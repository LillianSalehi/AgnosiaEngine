#pragma once
#include <cstring>
#include "../global.h"

namespace debug_libs {
  class Debug {
    public:
      static void vulkanDebugSetup(VkInstanceCreateInfo& createInfo, VkInstance& instance);
      static bool checkValidationLayerSupport();
      static void checkUnavailableValidationLayers();
      static void setupDebugMessenger(VkInstance& vulkanInstance);
      static void DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);
  };
}
