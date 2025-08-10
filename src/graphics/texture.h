#pragma once

#include <string>
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <cstdint>
#include "vk_mem_alloc.h"

class Texture {
protected:
  uint32_t mipLevels;
  VkImage image;
  VkImageView imageView;
  VkSampler sampler;

public:
  Texture(const std::string& ID, const std::string& texturePath);

  VkImage& getImage();
  VkImageView& getImageView();
  VkSampler& getSampler();
  uint32_t getMipLevels();
  
  static void createDepthImage();
  static void createColorImage();
  
  // ------------ Getters & Setters ------------ //
  struct Image {
    VkImage image;
    VkImageView imageView;
    VmaAllocation alloc;
  };
  static Image &getColorImage();
  static Image &getDepthImage();
};
