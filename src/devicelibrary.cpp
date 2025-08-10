#include "devicelibrary.h"
#include "utils/deletion.h"
#include "utils/helpers.h"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include "graphics/render.h"


VkPhysicalDeviceProperties deviceProperties;
VkDevice device;
VkSurfaceKHR surface;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkPhysicalDevice physicalDevice;
VkSampleCountFlagBits perPixelSampleCount;

VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
};

DeviceControl::QueueFamilyIndices
DeviceControl::findQueueFamilies(VkPhysicalDevice device) {
  // First we feed in a integer we want to use to hold the number of queued
  // items, that fills it, then we create that amount of default constructed
  // *VkQueueFamilyProperties* structs. These store the flags, the amount of
  // queued items in the family, and timestamp data. Queue families are simply
  // group collections of tasks we want to get done. Next, we check the flags of
  // the queueFamily item, use a bitwise and to see if they match, i.e. support
  // graphical operations, then return that to notify that we have at least one
  // family that supports VK_QUEUE_GRAPHICS_BIT. Which means this device
  // supports graphical operations! We also do the same thing for window
  // presentation, just check to see if its supported.
  DeviceControl::QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, DeviceControl::getSurface(),
                                         &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;
}
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

bool isDeviceSuitable(VkPhysicalDevice device) {
  // These two are simple, create a structure to hold the apiVersion,
  // driverVersion, vendorID, deviceID and type, name, and a few other settings.
  // Then populate it by passing in the device and the structure reference.
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  // Similarly, we can pass in the device and a deviceFeatures struct, this is
  // quite special, it holds a struct of optional features the GPU can perform.
  // Some, like a geometry shader, and stereoscopic rendering (multiViewport) we
  // want, so we dont return true without them.
  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
  // We need to find a device that supports graphical operations, or else we
  // cant do much with it! This function just runs over all the queueFamilies
  // and sees if there is a queue family with the VK_QUEUE_GRAPHICS_BIT flipped!
  DeviceControl::QueueFamilyIndices indices =
      DeviceControl::findQueueFamilies(device);
  bool extensionSupported = checkDeviceExtensionSupport(device);
  bool swapChainAdequate = false;

  if (extensionSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         supportedFeatures.samplerAnisotropy && indices.isComplete() &&
         extensionSupported && swapChainAdequate;
}
// -------------------------------------- Swap Chain Settings ----------------------------------------- //
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
  const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    // One of three settings we can set, Surface Format controls the color space and format.

    for (const auto &availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        // sRGB & 32bit BGRA
        return availableFormat;
      }
    }
  return availableFormats[0];
}
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // The second of the three settings, arguably the most important, the
  // presentation mode! This dictates how images are displayed. MAILBOX is
  // basically equivalent to triple buffering, it avoids screen tearing with
  // fairly low latency, However, it is not always supported, so in the case
  // that it isn't, currently we will default to FIFO, This is most similarly to
  // standard V-Sync.
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                            GLFWwindow *window) {
  // Swap Extent is just a fancy way of saying the resolution of the swap images
  // to display. This is almost always going to equal the resolution of the
  // window in pixels.

  // The max int32 value tells us that the window manager lets us change the
  // windth and height to what we wish!
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};
    // Clamp the image size to the minimum extent values specified by vulkan for
    // our window manager.
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

VkSampleCountFlagBits getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties physicalDeviceProps;
  VkSampleCountFlags maxCounts;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProps);

  VkSampleCountFlags counts =
      physicalDeviceProps.limits.framebufferColorSampleCounts &
      physicalDeviceProps.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}
// --------------------------------------- External Functions ----------------------------------------- //
void DeviceControl::pickPhysicalDevice(VkInstance &instance) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPU's with Vulkan Support! (DeviceLibrary:253)");
  }
  std::vector<VkPhysicalDevice> devices(
      deviceCount); // Direct Initialization is weird af, yo
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      // Once we have buttons or such, maybe ask the user or write a config file
      // for which GPU to use?
      physicalDevice = device;
      perPixelSampleCount = getMaxUsableSampleCount();
      break;
    }
  }
  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU! (DeviceLibrary.cpp:269)");
  }
}
void DeviceControl::createSurface(VkInstance &instance, GLFWwindow *window) {
  VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
  DeletionQueue::get().push_function([=](){vkDestroySurfaceKHR(instance, surface, nullptr);});
}
void DeviceControl::createLogicalDevice() {
  // Describe how many queues we want for a single family (1) here, right now we
  // are solely interested in graphics capabilites, but Compute Shaders,
  // transfer ops, decode and encode operations can also queued with setup! We
  // also assign each queue a priority. We do this by looping over all the
  // queueFamilies and sorting them by indices to fill the queue at the end!
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateSingularInfo = {};
    queueCreateSingularInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateSingularInfo.queueFamilyIndex = queueFamily;
    queueCreateSingularInfo.queueCount = 1;
    queueCreateSingularInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateSingularInfo);
  }
  
  
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingFeatures {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
    .pNext = nullptr,
  };

  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
    .pNext = &raytracingFeatures,
  };
  
  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &accelerationFeatures,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .shaderStorageBufferArrayNonUniformIndexing = true,
      .shaderStorageImageArrayNonUniformIndexing = true,
      .descriptorBindingSampledImageUpdateAfterBind = true,
      .descriptorBindingStorageImageUpdateAfterBind = true,
      .descriptorBindingStorageBufferUpdateAfterBind = true,
      .descriptorBindingUpdateUnusedWhilePending = true,
      .descriptorBindingPartiallyBound = true,
      .runtimeDescriptorArray = true,
      .scalarBlockLayout = true,
      .bufferDeviceAddress = true,

  };
  VkPhysicalDeviceVulkan13Features features13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &features12,
      .synchronization2 = true,
      .dynamicRendering = true,

  };
  VkPhysicalDeviceFeatures featuresBase{
      .robustBufferAccess = true,
      .sampleRateShading = true,
      .fillModeNonSolid = true,
      .wideLines = true,
      .largePoints = true,
      .samplerAnisotropy = true,
  };

  VkPhysicalDeviceFeatures2 deviceFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &features13,
      .features = featuresBase,
  };

  VkDeviceCreateInfo createDeviceInfo = {};
  createDeviceInfo.pNext = &deviceFeatures;
  createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  createDeviceInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createDeviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

  VK_CHECK(vkCreateDevice(physicalDevice, &createDeviceInfo, nullptr, &device));
  DeletionQueue::get().push_function([=](){vkDestroyDevice(device, nullptr);});
  
  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}
void DeviceControl::createSwapChain(GLFWwindow *window) {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

  // Number of images to hold in the swap chain, 1 over the minimum guarantees
  // we won't have to wait on the driver to complete internal operations before
  // acquiring another image. Absolutely a TODO to determine the best amount to
  // queue.
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  // Make sure not to queue more than the max! 0 indicates that there is no
  // maximum.
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createSwapChainInfo{};
  createSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createSwapChainInfo.surface = surface;
  createSwapChainInfo.minImageCount = imageCount;
  createSwapChainInfo.imageFormat = surfaceFormat.format;
  createSwapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
  createSwapChainInfo.imageExtent = extent;
  // Image array layers is always 1 unless we are developing for VR (Spoiler: we
  // are, we will use a build flag.) Image Usage specifies what operations you
  // use the images for, COLOR_ATTACH means we render directly to them, if you
  // wanted to render to separate images for things like post processing, you
  // can use TRANSFER_DST and use a memory operation to transfer the image to a
  // swap chain, this is also a TODO item eventually.
  createSwapChainInfo.imageArrayLayers = 1;
  createSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  // This handles swap chain images across multiple queue families, ie, if the
  // graphics queue family is different from the present queue
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};
  // Usage across multiple queue families without explicit transfer of ownership
  // if they are different queue families. Otherwise, no sharing without
  // explicit handoffs, faster, but not easily supported with multiple families.
  // Presentation and Graphics families are usually merged on most hardware.
  if (indices.graphicsFamily != indices.presentFamily) {
    createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createSwapChainInfo.queueFamilyIndexCount = 2;
    createSwapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  // Transformation of image support.
  createSwapChainInfo.preTransform =
      swapChainSupport.capabilities.currentTransform;
  // Do NOT blend with other windows on the system.
  createSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createSwapChainInfo.presentMode = presentMode;
  // This is interesting, clip pixels that are obscured for performance, but
  // that means you wont be able to read them reliably.. I am curious if this
  // would affect screen-space rendering techniques, may be something to note.
  createSwapChainInfo.clipped = VK_TRUE;
  // This is something that needs to be implemented later, operations like
  // resizing the window invalidate the swap chain and require you to recreate
  // it and reference the old one specified here, will revisit in a few days.
  // createSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

  VK_CHECK(vkCreateSwapchainKHR(device, &createSwapChainInfo, nullptr, &swapChain));

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

VkImageView DeviceControl::createImageView(VkImage image, VkFormat format,
                                           VkImageAspectFlags flags,
                                           uint32_t mipLevels) {
  // This defines the parameters of a newly created image object!
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = flags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;
  viewInfo.subresourceRange.levelCount = mipLevels;

  VkImageView imageView;
  VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

  return imageView;
}
void DeviceControl::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());

  for (uint32_t i = 0; i < swapChainImages.size(); i++) {
    swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    //DeletionQueue::get().push_function([=](){vkDestroyImageView(device, swapChainImageViews[i], nullptr);});
  }
}

// --------------------------------------- Getters & Setters ------------------------------------------ //
VkFormat &DeviceControl::getImageFormat() { return swapChainImageFormat; }
VkSwapchainKHR &DeviceControl::getSwapChain() { return swapChain; }
VkExtent2D &DeviceControl::getSwapChainExtent() { return swapChainExtent; }
std::vector<VkImage> &DeviceControl::getSwapChainImages() {
  return swapChainImages;
}

std::vector<VkImageView> &DeviceControl::getSwapChainImageViews() {
  return swapChainImageViews;
}
VkDevice &DeviceControl::getDevice() { return device; }
VkPhysicalDevice &DeviceControl::getPhysicalDevice() { return physicalDevice; }
VkSampleCountFlagBits &DeviceControl::getPerPixelSampleCount() {
  return perPixelSampleCount;
}
VkQueue &DeviceControl::getGraphicsQueue() { return graphicsQueue; }
VkQueue &DeviceControl::getPresentQueue() { return presentQueue; }
VkSurfaceKHR &DeviceControl::getSurface() { return surface; }

VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(DeviceControl::getPhysicalDevice(),
                                        format, &props);

    // Do we support linear tiling?
    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
      // Or do we support optimal tiling?
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported depth buffering format!");
}
VkFormat DeviceControl::getDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
