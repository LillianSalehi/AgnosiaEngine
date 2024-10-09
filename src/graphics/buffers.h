#pragma once
#include "../global.h"

namespace Buffers {
  class bufferslibrary {
    public:
      void createVertexBuffer();
      void destroyVertexBuffer();
      VkBuffer getVertexBuffer();
      std::vector<Global::Vertex> getVertices();
  };
}
