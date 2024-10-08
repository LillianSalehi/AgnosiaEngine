#pragma once
#include "../global.h"

namespace Graphics {
  class graphicspipeline {
    public:
      void createGraphicsPipeline();
      void destroyGraphicsPipeline();
      void createRenderPass();
      void destroyRenderPass();
      void createFramebuffers();
      void destroyFramebuffers();
      void createCommandPool();
      void destroyCommandPool();
      void createCommandBuffer();
      void recordCommandBuffer(VkCommandBuffer cmndBuffer, uint32_t imageIndex);
  };
}
