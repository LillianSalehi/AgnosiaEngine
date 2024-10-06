#pragma once
#include <vulkan/vulkan_core.h>
namespace AgnosiaEngine {
  class DeviceLibrary {
    public:
      void pickPhysicalDevice(VkInstance& instance);
      void createLogicalDevice(VkDevice& devicvee); 
  };
}
