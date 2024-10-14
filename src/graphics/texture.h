#pragma once
#include "../global.h"
#include <vulkan/vulkan_core.h>

namespace TextureLibraries {
  class texture {
    public:
      void createTextureImage();
      void createTextureImageView();
      void createTextureSampler();
      void destroyTextureImage();
      void destroyTextureSampler();
      VkFormat findDepthFormat();
      void createDepthResources();
  };
}
