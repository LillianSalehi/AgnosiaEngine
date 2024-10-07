#pragma once
#include "global.h"
namespace DeviceControl {
  class DeviceLibrary {
    public:

      void pickPhysicalDevice(VkInstance& instance);
      void createLogicalDevice(VkDevice& device);
      void createSurface(VkInstance& instance, GLFWwindow* window);
      void destroySurface(VkInstance& instance);
      void createSwapChain(GLFWwindow* window, VkDevice& device);
      void destroySwapChain(VkDevice& device);
  };
}


