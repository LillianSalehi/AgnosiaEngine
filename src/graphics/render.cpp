#include "../devicelibrary.h"
#include "../entrypoint.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "render.h"
#include "texture.h"
#include <stdexcept>

#include "../agnosiaimgui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace render_present {

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
VkDescriptorPool imGuiDescriptorPool;

static float floatBar = 0.0f;

void recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(Global::window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Global::window, &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(Global::device);
  // Don't really wanna do this but I also don't want to create an extra class
  // instance just to call the cleanup function.

  for (auto imageView : Global::swapChainImageViews) {
    vkDestroyImageView(Global::device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(Global::device, Global::swapChain, nullptr);

  device_libs::DeviceControl::createSwapChain(Global::window);
  device_libs::DeviceControl::createImageViews();
  texture_libs::Texture::createColorResources();
  texture_libs::Texture::createDepthResources();
}
// At a high level, rendering in Vulkan consists of 5 steps:
// Wait for the previous frame, acquire a image from the swap chain
// record a comman d buffer which draws the scene onto that image
// submit the recorded command buffer and present the image!
void Render::drawFrame() {
  vkWaitForFences(Global::device, 1, &inFlightFences[Global::currentFrame],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(Global::device, 1, &inFlightFences[Global::currentFrame]);

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(Global::device, Global::swapChain, UINT64_MAX,
                            imageAvailableSemaphores[Global::currentFrame],
                            VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  buffers_libs::Buffers::updateUniformBuffer(Global::currentFrame);

  vkResetFences(Global::device, 1, &inFlightFences[Global::currentFrame]);

  vkResetCommandBuffer(Global::commandBuffers[Global::currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);
  graphics_pipeline::Graphics::recordCommandBuffer(
      Global::commandBuffers[Global::currentFrame], imageIndex);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  Global::commandBuffers[Global::currentFrame]);
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      imageAvailableSemaphores[Global::currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &Global::commandBuffers[Global::currentFrame];

  VkSemaphore signalSemaphores[] = {
      renderFinishedSemaphores[Global::currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(Global::graphicsQueue, 1, &submitInfo,
                    inFlightFences[Global::currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {Global::swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(Global::presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      EntryApp::getInstance().getFramebufferResized()) {
    EntryApp::getInstance().setFramebufferResized(false);
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  Global::currentFrame =
      (Global::currentFrame + 1) % Global::MAX_FRAMES_IN_FLIGHT;
}
void Render::drawImGui() {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  // 2. Show a simple window that we create ourselves. We use a Begin/End pair
  // to create a named window.

  static int counter = 0;

  ImGui::Begin("Agnosia Debug"); // Create a window called "Hello, world!" and
                                 // append into it.

  ImGui::Text("This is some useful text."); // Display some text (you can use
                                            // a format strings too)
  ImGui::SliderFloat("float", &floatBar, 0.0f,
                     1.0f); // Edit 1 float using a slider from 0.0f to 1.0f

  agnosia_imgui::Gui::drawTabs();

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();

  ImGui::Render();
}
#pragma info
// SEMAPHORES
// Synchronization of execution on the GPU in Vulkan is *explicit* The Order of
// ops is up to us to define the how we want things to run. Similarly,
// Semaphores are used to add order between queue ops. There are 2 kinds of
// Semaphores; binary, and timeline. We are using Binary semaphores, which can
// be signaled or unsignaled. Semaphores are initizalized unsignaled, the way we
// use them to order queue operations is by providing the same semaphore in one
// queue op and a wait in another. For example: VkCommandBuffer QueueOne,
// QueueTwo = ... VkSemaphore semaphore = ... enqueue QueueOne, Signal semaphore
// when done, start now. vkQueueSubmit(work: QueueOne, signal: semaphore, wait:
// none) enqueue QueueTwo, wait on semaphore to start vkQueueSubmit(
// work: QueueTwo, signal: None, wait: semaphore)
// FENCES
// Fences are basically semaphores for the CPU! Otherwise known as the host. If
// the host needs to know when the GPU has finished a task, we use a fence.
// VkCommandBuffer cmndBuf = ...
// VkFence fence = ...
// Start work immediately, signal fence when done.
// vkQueueSubmit(work: cmndBuf, fence: fence)
// vkWaitForFence(fence)
// doStuffOnceFenceDone()
#pragma endinfo

void Render::createSyncObject() {
  imageAvailableSemaphores.resize(Global::MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(Global::MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(Global::MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < Global::MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(Global::device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(Global::device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(Global::device, &fenceInfo, nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create semaphores!");
    }
  }
}
void Render::destroyFenceSemaphores() {
  for (size_t i = 0; i < Global::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(Global::device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(Global::device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(Global::device, inFlightFences[i], nullptr);
  }
}
void Render::cleanupSwapChain() {
  vkDestroyImageView(Global::device, Global::colorImageView, nullptr);
  vkDestroyImage(Global::device, Global::colorImage, nullptr);
  vkFreeMemory(Global::device, Global::colorImageMemory, nullptr);
  vkDestroyImageView(Global::device, Global::depthImageView, nullptr);
  vkDestroyImage(Global::device, Global::depthImage, nullptr);
  vkFreeMemory(Global::device, Global::depthImageMemory, nullptr);

  for (auto imageView : Global::swapChainImageViews) {
    vkDestroyImageView(Global::device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(Global::device, Global::swapChain, nullptr);
}

void Render::init_imgui(VkInstance instance) {
  auto load_vk_func = [&](const char *fn) {
    if (auto proc = vkGetDeviceProcAddr(Global::device, fn))
      return proc;
    return vkGetInstanceProcAddr(instance, fn);
  };
  ImGui_ImplVulkan_LoadFunctions(
      [](const char *fn, void *data) {
        return (*(decltype(load_vk_func) *)data)(fn);
      },
      &load_vk_func);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // TODO
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(Global::window, true);

  VkDescriptorPoolSize ImGuiPoolSizes[]{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };
  VkDescriptorPoolCreateInfo ImGuiPoolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = ImGuiPoolSizes,
  };
  if (vkCreateDescriptorPool(Global::device, &ImGuiPoolInfo, nullptr,
                             &imGuiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create ImGui descriptor pool!");
  }

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = device_libs::DeviceControl::getImageFormat(),
      .depthAttachmentFormat = texture_libs::Texture::findDepthFormat(),
  };

  ImGui_ImplVulkan_InitInfo initInfo{
      .Instance = instance,
      .PhysicalDevice = Global::physicalDevice,
      .Device = Global::device,
      .QueueFamily = Global::findQueueFamilies(Global::physicalDevice)
                         .graphicsFamily.value(),
      .Queue = Global::graphicsQueue,
      .DescriptorPool = imGuiDescriptorPool,
      .MinImageCount = Global::MAX_FRAMES_IN_FLIGHT,
      .ImageCount = Global::MAX_FRAMES_IN_FLIGHT,
      .MSAASamples = Global::perPixelSampleCount,
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
  };

  ImGui_ImplVulkan_Init(&initInfo);
}
} // namespace render_present
