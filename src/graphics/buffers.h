#pragma once
#include "../devicelibrary.h"
#include <chrono>
#include <cstring>

namespace buffers_libs {
class Buffers {
public:
  static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags props, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory);
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags);
  static void createIndexBuffer();
  static void createVertexBuffer();
  static void destroyBuffers();
  static VkBuffer getVertexBuffer();
  static VkBuffer getIndexBuffer();
  static void createDescriptorSetLayout();
  static void createUniformBuffers();
  static void updateUniformBuffer(uint32_t currentImage);
  static void destroyUniformBuffer();
  static void createDescriptorPool();
  static void createDescriptorSets();
  static void destroyDescriptorPool();
  static VkDescriptorPool getDescriptorPool();
};
} // namespace buffers_libs
