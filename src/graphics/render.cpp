
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "../devicelibrary.h"
#include "../entrypoint.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "render.h"
#include "texture.h"

uint32_t currentFrame;
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;

void recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(EntryApp::getWindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(EntryApp::getWindow(), &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(DeviceControl::getDevice());
  // Don't really wanna do this but I also don't want to create an extra class
  // instance just to call the cleanup function.

  for (auto imageView : DeviceControl::getSwapChainImageViews()) {
    vkDestroyImageView(DeviceControl::getDevice(), imageView, nullptr);
  }
  vkDestroySwapchainKHR(DeviceControl::getDevice(),
                        DeviceControl::getSwapChain(), nullptr);

  DeviceControl::createSwapChain(EntryApp::getWindow());
  DeviceControl::createImageViews();
  Texture::createColorResources();
  Texture::createDepthResources();
}
// At a high level, rendering in Vulkan consists of 5 steps:
// Wait for the previous frame, acquire a image from the swap chain
// record a comman d buffer which draws the scene onto that image
// submit the recorded command buffer and present the image!
void Render::drawFrame() {
  vkWaitForFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame],
                  VK_TRUE, UINT64_MAX);

  vkResetFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame]);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      DeviceControl::getDevice(), DeviceControl::getSwapChain(), UINT64_MAX,
      imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  vkResetFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame]);

  vkResetCommandBuffer(Buffers::getCommandBuffers()[currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);
  Graphics::recordCommandBuffer(Buffers::getCommandBuffers()[currentFrame],
                                imageIndex);
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &Buffers::getCommandBuffers()[currentFrame];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {DeviceControl::getSwapChain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(DeviceControl::getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      EntryApp::getInstance().getFramebufferResized()) {
    EntryApp::getInstance().setFramebufferResized(false);
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  currentFrame = (currentFrame + 1) % Buffers::getMaxFramesInFlight();
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
  imageAvailableSemaphores.resize(Buffers::getMaxFramesInFlight());
  renderFinishedSemaphores.resize(Buffers::getMaxFramesInFlight());
  inFlightFences.resize(Buffers::getMaxFramesInFlight());

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < Buffers::getMaxFramesInFlight(); i++) {
    if (vkCreateSemaphore(DeviceControl::getDevice(), &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(DeviceControl::getDevice(), &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(DeviceControl::getDevice(), &fenceInfo, nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create semaphores!");
    }
  }
}
void Render::destroyFenceSemaphores() {
  for (size_t i = 0; i < Buffers::getMaxFramesInFlight(); i++) {
    vkDestroySemaphore(DeviceControl::getDevice(), renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(DeviceControl::getDevice(), imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(DeviceControl::getDevice(), inFlightFences[i], nullptr);
  }
}
void Render::cleanupSwapChain() {
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getColorImageView(),
                     nullptr);
  vkDestroyImage(DeviceControl::getDevice(), Texture::getColorImage(), nullptr);
  vkFreeMemory(DeviceControl::getDevice(), Texture::getColorImageMemory(),
               nullptr);
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getDepthImageView(),
                     nullptr);
  vkDestroyImage(DeviceControl::getDevice(), Texture::getDepthImage(), nullptr);
  vkFreeMemory(DeviceControl::getDevice(), Texture::getDepthImageMemory(),
               nullptr);

  for (auto imageView : DeviceControl::getSwapChainImageViews()) {
    vkDestroyImageView(DeviceControl::getDevice(), imageView, nullptr);
  }
  vkDestroySwapchainKHR(DeviceControl::getDevice(),
                        DeviceControl::getSwapChain(), nullptr);
}
uint32_t Render::getCurrentFrame() { return currentFrame; }
