#include <volk.h>

#include "../devicelibrary.h"
#include "../entrypoint.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "render.h"
#include "texture.h"
#include "../utils/helpers.h"
#include "../utils/deletion.h"

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
void Render::drawFrame(AssetCache& cache) {
  VK_CHECK(vkWaitForFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
  uint32_t imageIndex;

  VkResult result = vkAcquireNextImageKHR(DeviceControl::getDevice(), DeviceControl::getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
    return;
  }
  VK_CHECK(result);
    
  VK_CHECK(vkResetFences(DeviceControl::getDevice(), 1, &inFlightFences[currentFrame]));
  VK_CHECK(vkResetCommandBuffer(Buffers::getCommandBuffers()[currentFrame], 0));
  Graphics::recordCommandBuffer(Buffers::getCommandBuffers()[currentFrame], imageIndex, cache);
  
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &imageAvailableSemaphores[currentFrame],
    .pWaitDstStageMask = waitStages,
    .commandBufferCount = 1,
    .pCommandBuffers = &Buffers::getCommandBuffers()[currentFrame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &renderFinishedSemaphores[imageIndex],
  };

  VK_CHECK(vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]));
  
  VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,
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
    DeletionQueue::get().push_function([=](){vkDestroySemaphore(DeviceControl::getDevice(), imageAvailableSemaphores[i], nullptr);});
    VK_CHECK(vkCreateFence(DeviceControl::getDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
    DeletionQueue::get().push_function([=](){vkDestroyFence(DeviceControl::getDevice(), inFlightFences[i], nullptr);});
  }
  for(size_t i = 0; i < DeviceControl::getSwapChainImages().size(); i++) {
    VK_CHECK(vkCreateSemaphore(DeviceControl::getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));    
    DeletionQueue::get().push_function([=](){vkDestroySemaphore(DeviceControl::getDevice(), renderFinishedSemaphores[i], nullptr);});
  }
}
void Render::cleanupSwapChain() {
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getColorImage().imageView, nullptr);
  vmaDestroyImage(Buffers::getAllocator(), Texture::getColorImage().image, Texture::getColorImage().alloc);
  vkDestroyImageView(DeviceControl::getDevice(), Texture::getDepthImage().imageView, nullptr);
  vmaDestroyImage(Buffers::getAllocator(), Texture::getDepthImage().image, Texture::getDepthImage().alloc);

  for (auto imageView : DeviceControl::getSwapChainImageViews()) {
    vkDestroyImageView(DeviceControl::getDevice(), imageView, nullptr);
  }
  vkDestroySwapchainKHR(DeviceControl::getDevice(), DeviceControl::getSwapChain(), nullptr);
}
uint32_t Render::getCurrentFrame() { return currentFrame; }
