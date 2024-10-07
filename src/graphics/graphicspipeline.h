#pragma once
#include "../global.h"

namespace Graphics {
  class graphicspipeline {
    public:
      void createGraphicsPipeline(VkDevice& device);
      void destroyGraphicsPipeline(VkDevice& device);
  };
}
