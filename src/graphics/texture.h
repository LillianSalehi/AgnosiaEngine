#pragma once
#include "../global.h"
#include "buffers.h"
#include "../devicelibrary.h"

namespace texture_libs {
  class Texture {
    public:
      static void createTextureImage();
      static void createTextureImageView();
      static void createTextureSampler();
      static void destroyTextureImage();
      static void destroyTextureSampler();
      static VkFormat findDepthFormat();
      static void createDepthResources();
      
      // ------------ Getters & Setters ------------ // 
      static uint32_t getMipLevels();
  };
}
