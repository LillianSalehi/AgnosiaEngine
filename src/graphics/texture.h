#pragma once
#include "model.h"

#define VK_NO_PROTOTYPES

#include "volk.h"
#include <cstdint>
class Texture {
public:
  static const uint32_t TEXTURE_COUNT = 2;
  static void createMaterialTextures(std::vector<Model *> models);
  static void destroyTextureImage();
  static void destroyTextureSampler();
  static VkFormat findDepthFormat();
  static void createDepthResources();
  static void createColorResources();
  // ------------ Getters & Setters ------------ //
  static uint32_t getMipLevels();

  static VkImage &getColorImage();
  static VkImageView &getColorImageView();
  static VkDeviceMemory &getColorImageMemory();

  static VkImage &getDepthImage();
  static VkImageView &getDepthImageView();
  static VkDeviceMemory &getDepthImageMemory();
};
