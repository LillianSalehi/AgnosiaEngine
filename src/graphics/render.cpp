#include "render.h"
#include "graphicspipeline.h"
#include "../devicelibrary.h"
#include "../entrypoint.h"
#include "texture.h"
#include <vulkan/vulkan_core.h>
namespace RenderPresent {

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  Graphics::graphicspipeline pipeline;
  DeviceControl::devicelibrary deviceLibs;
  BuffersLibraries::buffers buffers;
  TextureLibraries::texture tex;

  void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(Global::window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(Global::window, &width, &height);
      glfwWaitEvents();
    }
    vkDeviceWaitIdle(Global::device);
    // Don't really wanna do this but I also don't want to create an extra class instance just to call the cleanup function.
    for(auto framebuffer : pipeline.getSwapChainFramebuffers()) {
      vkDestroyFramebuffer(Global::device, framebuffer, nullptr);
    }
    for(auto imageView : deviceLibs.getSwapChainImageViews()) {
      vkDestroyImageView(Global::device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(Global::device, Global::swapChain, nullptr);

    deviceLibs.createSwapChain(Global::window);
    deviceLibs.createImageViews();
    tex.createDepthResources();
    pipeline.createFramebuffers();
  }
  // At a high level, rendering in Vulkan consists of 5 steps:
  // Wait for the previous frame, acquire a image from the swap chain
  // record a comman d buffer which draws the scene onto that image
  // submit the recorded command buffer and present the image!
  void render::drawFrame() {

    vkWaitForFences(Global::device, 1, &inFlightFences[Global::currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(Global::device, 1, &inFlightFences[Global::currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(Global::device, Global::swapChain, UINT64_MAX, imageAvailableSemaphores[Global::currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    buffers.updateUniformBuffer(Global::currentFrame);

    vkResetFences(Global::device, 1, &inFlightFences[Global::currentFrame]);

    vkResetCommandBuffer(Global::commandBuffers[Global::currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    pipeline.recordCommandBuffer(Global::commandBuffers[Global::currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[Global::currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &Global::commandBuffers[Global::currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[Global::currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(Global::graphicsQueue, 1, &submitInfo, inFlightFences[Global::currentFrame]) != VK_SUCCESS) {
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
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || EntryApp::getInstance().getFramebufferResized()) {
    EntryApp::getInstance().setFramebufferResized(false);
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }
  Global::currentFrame = (Global::currentFrame + 1) % Global::MAX_FRAMES_IN_FLIGHT;
  }
  #pragma info
  // SEMAPHORES
  // Synchronization of execution on the GPU in Vulkan is *explicit* The Order of ops is up to us to 
  // define the how we want things to run. 
  // Similarly, Semaphores are used to add order between queue ops. There are 2 kinds of Semaphores; binary, and timeline.
  // We are using Binary semaphores, which can be signaled or unsignaled.
  // Semaphores are initizalized unsignaled, the way we use them to order queue operations is by providing the same semaphore in one queue op and a wait in another.
  // For example:
  // VkCommandBuffer QueueOne, QueueTwo = ...
  // VkSemaphore semaphore = ...
  // enqueue QueueOne, Signal semaphore when done, start now.
  // vkQueueSubmit(work: QueueOne, signal: semaphore, wait: none)
  // enqueue QueueTwo, wait on semaphore to start
  // vkQueueSubmit(
// work: QueueTwo, signal: None, wait: semaphore)
  // FENCES
  // Fences are basically semaphores for the CPU! Otherwise known as the host. If the host needs to know when the GPU has finished a task, we use a fence. 
  // VkCommandBuffer cmndBuf = ...
  // VkFence fence = ...
  // Start work immediately, signal fence when done.
  // vkQueueSubmit(work: cmndBuf, fence: fence)
  // vkWaitForFence(fence)
  // doStuffOnceFenceDone()
  #pragma endinfo

  void render::createSyncObject() {
    imageAvailableSemaphores.resize(Global::MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(Global::MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(Global::MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < Global::MAX_FRAMES_IN_FLIGHT; i++) {
      if(vkCreateSemaphore(Global::device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(Global::device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(Global::device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores!");
      }
    }

  
  }
  void render::destroyFenceSemaphores() {
    for (size_t i = 0; i < Global::MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(Global::device, renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(Global::device, imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(Global::device, inFlightFences[i], nullptr);
    }
  }
  void render::cleanupSwapChain() {
    vkDestroyImageView(Global::device, Global::depthImageView, nullptr);
    vkDestroyImage(Global::device, Global::depthImage, nullptr);
    vkFreeMemory(Global::device, Global::depthImageMemory, nullptr);
    for(auto framebuffer : pipeline.getSwapChainFramebuffers()) {
      vkDestroyFramebuffer(Global::device, framebuffer, nullptr);
    }
    for(auto imageView : deviceLibs.getSwapChainImageViews()) {
      vkDestroyImageView(Global::device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(Global::device, Global::swapChain, nullptr);
  }

}
