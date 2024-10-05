#include <vulkan/vulkan_core.h>

namespace AgnosiaEngine {
  class VulkanDebugLibs {
    public:
      void vulkanDebugSetup(VkInstanceCreateInfo& createInfo);
      bool checkValidationLayerSupport();
  };
}

