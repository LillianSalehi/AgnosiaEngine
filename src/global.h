#pragma once
#include "debug/vulkandebuglibs.h"
#include <iostream>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Global {
  // Global variables and includes we are going to use almost everywhere, validation layers hook into everything, and you need to check if they are enabled first,
  // so that's one obvious global, as well as the glfw includes!
  extern const std::vector<const char*> validationLayers;
  extern const bool enableValidationLayers;
  extern VkDevice device;  
  struct QueueFamilyIndices {
    // We need to check that the Queue families support graphics operations and window presentation, sometimes they can support one or the other,
    // therefore, we take into account both for completion.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  extern VkSurfaceKHR surface;
  extern VkPhysicalDevice physicalDevice;
  Global::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
}
