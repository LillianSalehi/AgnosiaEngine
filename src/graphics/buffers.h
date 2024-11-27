#pragma once

#include <cstdint>
#include <span>
#define VK_NO_PROTOTYPES
#include "../types.h"
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
  static Agnosia_T::GPUMeshBuffers
  sendMesh(std::span<uint32_t> indices, std::span<Agnosia_T::Vertex> vertices);
  static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags props, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory);
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags);
  static void destroyBuffers();
  static void createDescriptorSetLayout();
  static void createUniformBuffers();
  static void updateUniformBuffer(uint32_t currentImage);
  static void destroyUniformBuffer();
  static void createDescriptorPool();
  static void createDescriptorSets();
  static void destroyDescriptorPool();
  static VkDescriptorPool &getDescriptorPool();
  static VkDescriptorSetLayout &getDescriptorSetLayout();
  static std::vector<VkDescriptorSet> &getDescriptorSets();

  static float *getObjPos();
  static float *getCamPos();
  static float *getCenterPos();
  static float *getUpDir();
  static float &getDepthField();
  static float *getDistanceField();
  static uint32_t getMaxFramesInFlight();
  static std::vector<VkCommandBuffer> &getCommandBuffers();
  static std::vector<VkBuffer> &getUniformBuffers();
  static std::vector<VkDeviceMemory> &getUniformBuffersMemory();
  static VkCommandPool &getCommandPool();
  static std::vector<Agnosia_T::Vertex> &getVertices();
  static std::vector<uint32_t> &getIndices();
};
