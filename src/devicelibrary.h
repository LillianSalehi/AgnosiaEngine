#pragma once
#include "global.h"
#include <algorithm>
#include <limits>
#include <set>
#include <string>

namespace device_libs {
class DeviceControl {
    public:
      static void pickPhysicalDevice(VkInstance& instance);
      static void createLogicalDevice();
      static void createSurface(VkInstance& instance, GLFWwindow* window);
      static void destroySurface(VkInstance& instance);
      static void createSwapChain(GLFWwindow* window);
      static void destroySwapChain();
      static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, uint32_t mipLevels);
      static void createImageViews();
      static void destroyImageViews();
      static void createCommandPool();
      static void destroyCommandPool();

      // ---------- Getters & Setters ----------- //
      static VkFormat getImageFormat();
      static std::vector<VkImageView> getSwapChainImageViews();
      static VkExtent2D getSwapChainExtent();
      static std::vector<VkFramebuffer> getSwapChainFramebuffers();
  };
}


