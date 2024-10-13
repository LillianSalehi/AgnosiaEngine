#pragma once
#include "../global.h"

namespace BuffersLibraries {
  class buffers {
    public:
      void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
      uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);
      void createIndexBuffer();
      void createVertexBuffer();
      void destroyBuffers();
      VkBuffer getVertexBuffer();
      VkBuffer getIndexBuffer();
      std::vector<Global::Vertex> getVertices();
      std::vector<uint16_t> getIndices();
      void createDescriptorSetLayout();
      void createUniformBuffers();
      void updateUniformBuffer(uint32_t currentImage);
      void destroyUniformBuffer();
      void createDescriptorPool();
      void createDescriptorSets();
      void destroyDescriptorPool();
  };
}
