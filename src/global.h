#pragma once
#include "debug/VulkanDebugLibs.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Global {
  // Global variables and includes we are going to use almost everywhere, validation layers hook into everything, and you need to check if they are enabled first,
  // so that's one obvious global, as well as the glfw includes!
  extern const std::vector<const char*> validationLayers;
  extern const bool enableValidationLayers;
}
