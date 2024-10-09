#pragma once
#include "global.h"
#include <optional>
#include <algorithm>
#include <limits>
#include <ostream>
#include <set>
#include <string>
#include <vector>
namespace DeviceControl {
class devicelibrary {
    public:
      void pickPhysicalDevice(VkInstance& instance);
      void createLogicalDevice();
      void createSurface(VkInstance& instance, GLFWwindow* window);
      void destroySurface(VkInstance& instance);
      void createSwapChain(GLFWwindow* window);
      void destroySwapChain();
      void createImageViews();
      void destroyImageViews();
      void createCommandPool();
      void destroyCommandPool();

      // ---------- Getters & Setters ----------- //
      VkFormat getImageFormat();
      std::vector<VkImageView> getSwapChainImageViews();
      VkExtent2D getSwapChainExtent();
      std::vector<VkFramebuffer> getSwapChainFramebuffers();
  };
}


