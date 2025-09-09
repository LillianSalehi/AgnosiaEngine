#pragma once

#include <functional>
#include <source_location>
#include "volk.h"
#include <stdexcept>
#include <string>
#include "../devicelibrary.h"
#include "../graphics/buffers.h"

static void VK_CHECK(VkResult result, std::source_location location = std::source_location::current()) {
  if(result != VK_SUCCESS) {
    std::string const fileName = location.file_name();
    std::string const line = std::to_string(location.line());
    std::string const function = location.function_name();
    
    throw std::runtime_error("VkResult was not VK_SUCCESS at: " + fileName + ":" + line + ", " + function + ", Result Code: " + std::to_string(result));    
  }
}
template<class T> [[nodiscard]] T* Address(T&& v) {
  return std::addressof(v);
}
inline void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = Buffers::getCommandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo,
                           &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  function(commandBuffer);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo,
                VK_NULL_HANDLE);
  vkQueueWaitIdle(DeviceControl::getGraphicsQueue());

  vkFreeCommandBuffers(DeviceControl::getDevice(), Buffers::getCommandPool(), 1,
                       &commandBuffer);
}
