#define VK_NO_PROTOTYPES
#include <volk.h>

#include "../devicelibrary.h"
#include "../entrypoint.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "render.h"
#include "texture.h"
#include "../utils.h"

uint32_t currentFrame = 0;
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

  Render::cleanupSwapChain();
  
  DeviceControl::createSwapChain(EntryApp::getWindow());
  DeviceControl::createImageViews();
  Texture::createColorImage();
  Texture::createDepthImage();
}
// At a high level, rendering in Vulkan consists of 5 steps:
// Wait for the previous frame, acquire a image from the swap chain
// record a command buffer which draws the scene onto that image
// submit the recorded command buffer and present the image!
void Render::drawFrame() {
  vkWaitForFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame]);

  uint32_t imageIndex;
  VkSemaphore acquireSemaphore = imageAvailableSemaphores[currentFrame];
  VkResult result = vkAcquireNextImageKHR(DeviceControl::getDevice(), DeviceControl::getSwapChain(), UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
    return;
  }
  VK_CHECK(result);

  VkSemaphore submitSemaphore = renderFinishedSemaphores[imageIndex];
    
  vkResetFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame]);
  vkResetCommandBuffer(Buffers::getCommandBuffers()[currentFrame], 0);
  Graphics::recordCommandBuffer(Buffers::getCommandBuffers()[currentFrame], imageIndex);
  
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &acquireSemaphore,
    .pWaitDstStageMask = waitStages,
    .commandBufferCount = 1,
    .pCommandBuffers = &Buffers::getCommandBuffers()[currentFrame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &submitSemaphore,
  };

  VK_CHECK(vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]));
  
  VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderFinishedSemaphores[imageIndex],
    .swapchainCount = 1,
    .pSwapchains = &DeviceControl::getSwapChain(),
    .pImageIndices = &imageIndex,
  };

  result = vkQueuePresentKHR(DeviceControl::getPresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || EntryApp::getInstance().getFramebufferResized()) {
    EntryApp::getInstance().setFramebufferResized(false);
    recreateSwapChain();
  }
  VK_CHECK(result);
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
  renderFinishedSemaphores.resize(DeviceControl::getSwapChainImages().size());
  inFlightFences.resize(Buffers::getMaxFramesInFlight());

  VkSemaphoreCreateInfo semaphoreInfo = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  
  VkFenceCreateInfo fenceInfo = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  
  for (size_t i = 0; i < Buffers::getMaxFramesInFlight(); i++) {
    VK_CHECK(vkCreateSemaphore(DeviceControl::getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
    VK_CHECK(vkCreateFence(DeviceControl::getDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
  }
  for(size_t i = 0; i < DeviceControl::getSwapChainImages().size(); i++) {
    VK_CHECK(vkCreateSemaphore(DeviceControl::getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));    
  }
}
void Render::destroyFenceSemaphores() {
  for (size_t i = 0; i < Buffers::getMaxFramesInFlight(); i++) {
    vkDestroySemaphore(DeviceControl::getDevice(), renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(DeviceControl::getDevice(), imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(DeviceControl::getDevice(), inFlightFences[i], nullptr);
  }
}
void Render::cleanupSwapChain() {
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getColorImage().imageView, nullptr);
  vkDestroyImage(DeviceControl::getDevice(), Texture::getColorImage().image, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), Texture::getColorImage().memory, nullptr);
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getDepthImage().imageView, nullptr);
  vkDestroyImage(DeviceControl::getDevice(), Texture::getDepthImage().image, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), Texture::getDepthImage().memory, nullptr);

  for (auto imageView : DeviceControl::getSwapChainImageViews()) {
    vkDestroyImageView(DeviceControl::getDevice(), imageView, nullptr);
  }
  vkDestroySwapchainKHR(DeviceControl::getDevice(), DeviceControl::getSwapChain(), nullptr);
}
uint32_t Render::getCurrentFrame() { return currentFrame; }
