#include "DeviceLibrary.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace DeviceControl {


  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
  
  struct QueueFamilyIndices {
    // We need to check that the Queue families support graphics operations and window presentation, sometimes they can support one or the other,
    // therefore, we take into account both for completion.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    // First we feed in a integer we want to use to hold the number of queued items, that fills it, then we create that amount of default constructed *VkQueueFamilyProperties* structs. 
    // These store the flags, the amount of queued items in the family, and timestamp data. Queue families are simply group collections of tasks we want to get done. 
    // Next, we check the flags of the queueFamily item, use a bitwise and to see if they match, i.e. support graphical operations, then return that to notify that we have at least one family that supports VK_QUEUE_GRAPHICS_BIT.
    // Which means this device supports graphical operations!
    // We also do the same thing for window presentation, just check to see if its supported.
    QueueFamilyIndices indices;
  
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies) {
      if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if(presentSupport) {
        indices.presentFamily = i;
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
  void DeviceLibrary::destroySurface(VkInstance& instance) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    std::cout << "Destroyed surface safely\n" << std::endl;
  }
  void DeviceLibrary::createSurface(VkInstance& instance, GLFWwindow* window) {
    if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface!!");
    }
    std::cout << "GLFW Window Surface created successfully\n" << std::endl;
  }
  void DeviceLibrary::createLogicalDevice(VkDevice& device) {
    // Describe how many queues we want for a single family (1) here, right now we are solely interested in graphics capabilites,
    // but Compute Shaders, transfer ops, decode and encode operations can also queued with setup! We also assign each queue a priority.
    // We do this by looping over all the queueFamilies and sorting them by indices to fill the queue at the end!
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { 
      indices.graphicsFamily.value(), 
      indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateSingularInfo = {};
      queueCreateSingularInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateSingularInfo.queueFamilyIndex = queueFamily;
      queueCreateSingularInfo.queueCount = 1;
      queueCreateSingularInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateSingularInfo);
    }
    VkDeviceCreateInfo createDeviceInfo = {};
    createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
    createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    createDeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createDeviceInfo.pEnabledFeatures = &deviceFeatures;
    createDeviceInfo.enabledExtensionCount = 0;


    if(Global::enableValidationLayers) {
      createDeviceInfo.enabledLayerCount = static_cast<uint32_t>(Global::validationLayers.size());
      createDeviceInfo.ppEnabledLayerNames = Global::validationLayers.data();
    } else {
      createDeviceInfo.enabledLayerCount = 0;
    }
    if(vkCreateDevice(physicalDevice, &createDeviceInfo, nullptr, &device) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create logical device");
    }
    std::cout << "Created Logical device successfully!\n" << std::endl;

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);  
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
  }
}
