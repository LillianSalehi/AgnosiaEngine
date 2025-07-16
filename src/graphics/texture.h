#pragma once

#include <string>
#define VK_NO_PROTOTYPES
#include "volk.h"


#include <cstdint>
class Texture {
protected:
  uint32_t mipLevels;
  VkImage image;
  VkImageView imageView;
  VkSampler sampler;
  
public:
  Texture(const std::string& texturePath);

  VkImage& getImage();
  VkImageView& getImageView();
  VkSampler& getSampler();
  uint32_t getMipLevels();
  
  static void createDepthResources();
  static void createColorResources();
  // ------------ Getters & Setters ------------ //
  

  static VkImage &getColorImage();
  static VkImageView &getColorImageView();
  static VkDeviceMemory &getColorImageMemory();

  static VkImage &getDepthImage();
  static VkImageView &getDepthImageView();
  static VkDeviceMemory &getDepthImageMemory();
};
