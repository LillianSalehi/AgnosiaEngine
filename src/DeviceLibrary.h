#include <vulkan/vulkan_core.h>
namespace AgnosiaEngine {
  class DeviceLibrary {
    public:
      void pickPhysicalDevice(VkInstance& instance);
  };
}
