#pragma once
#include "../devicelibrary.h"
#include "../global.h"
#include "buffers.h"
#include <cstdint>

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
  static void createColorResources();
  // ------------ Getters & Setters ------------ //
  static uint32_t getMipLevels();
};
} // namespace texture_libs
