#pragma once
#include "../global.h"
#include "../devicelibrary.h"
#include "buffers.h"
#include "texture.h"
#include <fstream>

namespace graphics_pipeline {
  class Graphics {
    public:
      static void createGraphicsPipeline();
      static void destroyGraphicsPipeline();
      static void createRenderPass();
      static void destroyRenderPass();
      static void createFramebuffers();
      static void destroyFramebuffers();
      static void createCommandPool();
      static void destroyCommandPool();
      static void createCommandBuffer();
      static void recordCommandBuffer(VkCommandBuffer cmndBuffer, uint32_t imageIndex);
      static std::vector<VkFramebuffer> getSwapChainFramebuffers();
  };
}
