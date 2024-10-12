#pragma once
#include "../global.h"
#include <cstdint>

namespace Buffers {
  class bufferslibrary {
    public:
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
