#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include "../utils/types.h"
#include "model.h"
#include <vector>
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Buffers {
public:
  static Agnosia_T::AllocatedBuffer createBuffer(size_t allocSize,
                                                 VmaAllocationCreateFlags vmaFlags,
                                                 VkBufferUsageFlags usageFlags,
                                                 VmaMemoryUsage memUsage);
  static void createMemoryAllocator(VkInstance vkInstance);
  static VmaAllocator getAllocator();
  static void createDescriptorSetLayout();
  static void createDescriptorSet(std::vector<Model *> models);
  static void createDescriptorPool();
  
  
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags);
  static VkDescriptorPool &getDescriptorPool();
  static VkDescriptorSet &getDescriptorSet();

  static VkDescriptorSetLayout &getDescriptorSetLayout();
  static uint32_t getMaxFramesInFlight();
  static std::vector<VkCommandBuffer> &getCommandBuffers();

  static VkCommandPool &getCommandPool();
  static uint32_t getIndicesSize();
};
