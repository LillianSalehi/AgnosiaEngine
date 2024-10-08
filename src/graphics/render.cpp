#include "render.h"
#include "graphicspipeline.h"
namespace RenderPresent {

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  Graphics::graphicspipeline pipeline;
  uint32_t currentFrame = 0;
  // At a high level, rendering in Vulkan consists of 5 steps:
  // Wait for the previous frame, acquire a image from the swap chain
  // record a comman d buffer which draws the scene onto that image
  // submit the recorded command buffer and present the image!
  void render::drawFrame() {

    vkWaitForFences(Global::device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(Global::device, 1, &inFlightFences[currentFrame]);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(Global::device, Global::swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(Global::commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    pipeline.recordCommandBuffer(Global::commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &Global::commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(Global::graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

    vkQueuePresentKHR(Global::presentQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % Global::MAX_FRAMES_IN_FLIGHT;
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
  // vkQueueSubmit(work: QueueTwo, signal: None, wait: semaphore)
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
  void destroyFenceSemaphore() {
    for (size_t i = 0; i < Global::MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(Global::device, imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(Global::device, renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(Global::device, inFlightFences[i], nullptr);
    }
  }
}
