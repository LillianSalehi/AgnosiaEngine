#pragma once
#include "../global.h"

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
  };
}
