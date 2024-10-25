#include "global.h"

namespace Global {

  VkSurfaceKHR surface;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSwapchainKHR swapChain;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  GLFWwindow* window;
  VkDescriptorSetLayout descriptorSetLayout;
  std::vector<VkDescriptorSet> descriptorSets;
  uint32_t currentFrame = 0;
  VkImageView textureImageView;
  VkSampler textureSampler;
  VkImageView depthImageView;
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;

  std::vector<VkImageView> swapChainImageViews;
  std::vector<Vertex> vertices;
  // Index buffer definition, showing which points to reuse.
  std::vector<uint32_t> indices;

  Global::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    // First we feed in a integer we want to use to hold the number of queued items, that fills it, then we create that amount of default constructed *VkQueueFamilyProperties* structs. 
    // These store the flags, the amount of queued items in the family, and timestamp data. Queue families are simply group collections of tasks we want to get done. 
    // Next, we check the flags of the queueFamily item, use a bitwise and to see if they match, i.e. support graphical operations, then return that to notify that we have at least one family that supports VK_QUEUE_GRAPHICS_BIT.
    // Which means this device supports graphical operations!
    // We also do the same thing for window presentation, just check to see if its supported.
    Global::QueueFamilyIndices indices;
  
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
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, Global::surface, &presentSupport);
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
}
