#pragma once
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <optional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class DeviceControl {
public:
  struct QueueFamilyIndices {
    // We need to check that the Queue families support graphics operations and
    // window presentation, sometimes they can support one or the other,
    // therefore, we take into account both for completion.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };
  static void pickPhysicalDevice(VkInstance &instance);
  static void createLogicalDevice();
  static void createSurface(VkInstance &instance, GLFWwindow *window);
  static void destroySurface(VkInstance &instance);
  static void createSwapChain(GLFWwindow *window);
  static void destroySwapChain();
  static VkImageView createImageView(VkImage image, VkFormat format,
                                     VkImageAspectFlags flags,
                                     uint32_t mipLevels);
  static void createImageViews();
  static void destroyImageViews();
  static void createCommandPool();
  static void destroyCommandPool();
  static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  // ---------- Getters & Setters ----------- //
  static VkFormat &getImageFormat();
  static VkFormat getDepthFormat();
  static VkExtent2D &getSwapChainExtent();
  static std::vector<VkImage> &getSwapChainImages();
  static std::vector<VkFramebuffer> &getSwapChainFramebuffers();
  static VkDevice &getDevice();
  static VkSurfaceKHR &getSurface();
  static VkQueue &getGraphicsQueue();
  static VkQueue &getPresentQueue();
  static VkPhysicalDevice &getPhysicalDevice();
  static VkSampleCountFlagBits &getPerPixelSampleCount();
  static std::vector<VkImageView> &getSwapChainImageViews();
  static VkSwapchainKHR &getSwapChain();
};
