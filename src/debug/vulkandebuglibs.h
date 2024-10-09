#pragma once
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <cstring>
#include "../global.h"

namespace Debug {
  class vulkandebuglibs {
    
    public:
      void vulkanDebugSetup(VkInstanceCreateInfo& createInfo, VkInstance& instance);
      bool checkValidationLayerSupport();
      void checkUnavailableValidationLayers();
      void setupDebugMessenger(VkInstance& vulkanInstance);
      void DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);
  };
}
