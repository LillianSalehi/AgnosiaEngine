#pragma once

#include <cstdint>

#define VK_NO_PROTOTYPES
#include "../types.h"
#include "model.h"
#include "volk.h"
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Buffers {
public:
  static Agnosia_T::AllocatedBuffer createBuffer(size_t allocSize,
                                                 VkBufferUsageFlags usage,
                                                 VmaMemoryUsage memUsage);
  static void createMemoryAllocator(VkInstance vkInstance);
  static void createDescriptorSetLayout();
  static void createDescriptorSet(std::vector<Model *> models);
  static void createDescriptorPool();
  static void destroyDescriptorPool();
  static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags props, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory);
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags);
  static void destroyBuffers();
  static VkDescriptorPool &getDescriptorPool();
  static VkDescriptorSet &getDescriptorSet();

  static VkDescriptorSetLayout &getDescriptorSetLayout();
  static uint32_t getMaxFramesInFlight();
  static std::vector<VkCommandBuffer> &getCommandBuffers();

  static VkCommandPool &getCommandPool();
  static uint32_t getIndicesSize();
};
