#include "DeviceLibrary.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
using namespace AgnosiaEngine;

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkPhysicalDeviceProperties deviceProperties;
VkPhysicalDeviceFeatures deviceFeatures;

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;

  bool isComplete() {
    return graphicsFamily.has_value();
  }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
  // First we feed in a integer we want to use to hold the number of queued items, that fills it, then we create that amount of default constructed *VkQueueFamilyProperties* structs. 
  // These store the flags, the amount of queued items in the family, and timestamp data. Queue families are simply group collections of tasks we want to get done. 
  // Next, we check the flags of the queueFamily item, use a bitwise and to see if they match, i.e. support graphical operations, then return that to notify that we have at least one family that supports VK_QUEUE_GRAPHICS_BIT.
  // Which means this device supports graphical operations!
  QueueFamilyIndices indices;
  
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for(const auto& queueFamily : queueFamilies) {
    if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    if(indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
  // These two are simple, create a structure to hold the apiVersion, driverVersion, vendorID, deviceID and type, name, and a few other settings.
  // Then populate it by passing in the device and the structure reference.
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  // Similarly, we can pass in the device and a deviceFeatures struct, this is quite special, it holds a struct of optional features the GPU can perform.
  // Some, like a geometry shader, and stereoscopic rendering (multiViewport) we want, so we dont return true without them.
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
  // We need to find a device that supports graphical operations, or else we cant do much with it! This function just runs over all the queueFamilies and sees if there 
  // is a queue family with the VK_QUEUE_GRAPHICS_BIT flipped!
  QueueFamilyIndices indices = findQueueFamilies(device);

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.multiViewport && deviceFeatures.geometryShader && indices.isComplete();
}

void DeviceLibrary::pickPhysicalDevice(VkInstance& instance) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if(deviceCount == 0) {
    throw std::runtime_error("Failed to find GPU's with Vulkan Support!!");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount); // Direct Initialization is weird af, yo
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for(const auto& device : devices) {
    if(isDeviceSuitable(device)) {
      std::cout << "Using device: " << deviceProperties.deviceName << std::endl;
      //Once we have buttons or such, maybe ask the user or write a config file for which GPU to use?
      physicalDevice = device;
      break;
    }
  }
  
  if(physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU!");
  }
}


