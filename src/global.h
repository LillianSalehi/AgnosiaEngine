#pragma once
#include "debug/VulkanDebugLibs.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Global {
  extern const std::vector<const char*> validationLayers;
  extern const bool enableValidationLayers;
}
