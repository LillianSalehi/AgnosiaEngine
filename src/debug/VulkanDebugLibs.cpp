#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanDebugLibs.h"
using namespace AgnosiaEngine;

#include <vector>
#include <cstring>
#include <vulkan/vulkan_core.h>

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

void VulkanDebugLibs::vulkanDebugSetup(VkInstanceCreateInfo& createInfo) {
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();
}
 
bool VulkanDebugLibs::checkValidationLayerSupport() {                     // This function is used to check Validation Layer Support, validation layers are the debug trace tools in the Vulkan SDK.
  uint32_t layerCount;                                                    // layerCount will be used as the var to keep track of the number of requested validation layerk
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);               // Set layerCount to the number of validation layers requested when pProperties is NULLPTR

  std::vector<VkLayerProperties> availableLayers(layerCount);             // VkLayerProperties is a structure with data on the layername, desc, versions and etc. 
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());// Now that we have a VkLayerProperties fed in, as well as the num. of properties, we can fill layerCount with the VkResult

  for(const char* layerName : validationLayers) {                         // Pretty straightforward from here, just enumerate over all the VkResult data and see if we have any validationLayers
    bool layerFound = false;

    for(const auto& layerProperties : availableLayers) {
      if(strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if(!layerFound) {
      return false;
    }
  }
  return true;
}



