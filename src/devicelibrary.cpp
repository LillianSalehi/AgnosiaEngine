#include "devicelibrary.h"

namespace device_libs {

  VkPhysicalDeviceProperties deviceProperties;

  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };
  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {

    // Swap chains are weird ngl, it's another one of those Vulkan platform agnosticity. The swapchain is basically a wrapper for GDI+, DXGI, X11, Wayland, etc.
    // It lets us use the swap chain rather than create a different framebuffer handler for every targeted platform. 
    // Swap chains handle the ownership of buffers before sending them to the presentation engine.
    // (still no fucking clue how it works though)
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Global::surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, Global::surface, &formatCount, nullptr);

    if(formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, Global::surface, &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, Global::surface, &presentModeCount, details.presentModes.data());

    if(presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, Global::surface, &presentModeCount, details.presentModes.data());
    }
  
    return details;
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
       requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  bool isDeviceSuitable(VkPhysicalDevice device) {
    // These two are simple, create a structure to hold the apiVersion, driverVersion, vendorID, deviceID and type, name, and a few other settings.
    // Then populate it by passing in the device and the structure reference.
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // Similarly, we can pass in the device and a deviceFeatures struct, this is quite special, it holds a struct of optional features the GPU can perform.
    // Some, like a geometry shader, and stereoscopic rendering (multiViewport) we want, so we dont return true without them.
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    // We need to find a device that supports graphical operations, or else we cant do much with it! This function just runs over all the queueFamilies and sees if there 
    // is a queue family with the VK_QUEUE_GRAPHICS_BIT flipped!
    Global::QueueFamilyIndices indices = Global::findQueueFamilies(device);
    bool extensionSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    if(extensionSupported) {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
  
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU 
      && supportedFeatures.samplerAnisotropy 
      && indices.isComplete() 
      && extensionSupported
      && swapChainAdequate;
  }
// -------------------------------------- Swap Chain Settings ----------------------------------------- //
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // One of three settings we can set, Surface Format controls the color space and format.
    
    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        // sRGB & 32bit BGRA
        return availableFormat;
      }
    }
    return availableFormats[0];
  }
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // The second of the three settings, arguably the most important, the presentation mode! This dictates how images are displayed.
    // MAILBOX is basically equivalent to triple buffering, it avoids screen tearing with fairly low latency,
    // However, it is not always supported, so in the case that it isn't, currently we will default to FIFO, 
    // This is most similarly to standard V-Sync.
    for(const auto& availablePresentMode : availablePresentModes) {
      if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        if(Global::enableValidationLayers) std::cout << "Using Triple Buffering\n" << std::endl;
        return availablePresentMode;
      }
    }

    if(Global::enableValidationLayers) std::cout << "Using FIFO (V-Sync)\n" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
  }
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    // Swap Extent is just a fancy way of saying the resolution of the swap images to display.
    // This is almost always going to equal the resolution of the window in pixels.
  
    // The max int32 value tells us that the window manager lets us change the windth and height to what we wish!
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
      };
        // Clamp the image size to the minimum extent values specified by vulkan for our window manager.
       actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
       actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

       return actualExtent;
    }
  }
// --------------------------------------- External Functions ----------------------------------------- //
  void DeviceControl::pickPhysicalDevice(VkInstance& instance) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if(deviceCount == 0) {
      throw std::runtime_error("Failed to find GPU's with Vulkan Support!!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount); // Direct Initialization is weird af, yo
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for(const auto& device : devices) {
      if(isDeviceSuitable(device)) {
        if(Global::enableValidationLayers) std::cout << "Using device: " << deviceProperties.deviceName << std::endl;
        //Once we have buttons or such, maybe ask the user or write a config file for which GPU to use?
        Global::physicalDevice = device;
        break;
      }
    }
    if(Global::physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("Failed to find a suitable GPU!");
    }
  }
  void DeviceControl::destroySurface(VkInstance& instance) {
    vkDestroySurfaceKHR(instance, Global::surface, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed surface safely\n" << std::endl;
  }
  void DeviceControl::createSurface(VkInstance& instance, GLFWwindow* window) {
    if(glfwCreateWindowSurface(instance, window, nullptr, &Global::surface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface!!");
    }
    if(Global::enableValidationLayers) std::cout << "GLFW Window Surface created successfully\n" << std::endl;
  }
  void DeviceControl::createLogicalDevice() {
    // Describe how many queues we want for a single family (1) here, right now we are solely interested in graphics capabilites,
    // but Compute Shaders, transfer ops, decode and encode operations can also queued with setup! We also assign each queue a priority.
    // We do this by looping over all the queueFamilies and sorting them by indices to fill the queue at the end!
    Global::QueueFamilyIndices indices = Global::findQueueFamilies(Global::physicalDevice);

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
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createDeviceInfo = {};
    createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
    createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    createDeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createDeviceInfo.pEnabledFeatures = &deviceFeatures;
    createDeviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if(Global::enableValidationLayers) {
      createDeviceInfo.enabledLayerCount = static_cast<uint32_t>(Global::validationLayers.size());
      createDeviceInfo.ppEnabledLayerNames = Global::validationLayers.data();
    } else {
      createDeviceInfo.enabledLayerCount = 0;
    }
    if(vkCreateDevice(Global::physicalDevice, &createDeviceInfo, nullptr, &Global::device) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create logical device");
    }
    if(Global::enableValidationLayers) std::cout << "Created Logical device successfully!\n" << std::endl;

    vkGetDeviceQueue(Global::device, indices.graphicsFamily.value(), 0, &Global::graphicsQueue);  
    vkGetDeviceQueue(Global::device, indices.presentFamily.value(), 0, &Global::presentQueue);
  }
  void DeviceControl::createSwapChain(GLFWwindow* window) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(Global::physicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
    
    // Number of images to hold in the swap chain, 1 over the minimum guarantees we won't have to wait on the driver to complete 
    // internal operations before acquiring another image. Absolutely a TODO to determine the best amount to queue.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // Make sure not to queue more than the max! 0 indicates that there is no maximum.
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createSwapChainInfo{};
    createSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createSwapChainInfo.surface = Global::surface;
    createSwapChainInfo.minImageCount = imageCount;
    createSwapChainInfo.imageFormat = surfaceFormat.format;
    createSwapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    createSwapChainInfo.imageExtent = extent;
    // Image array layers is always 1 unless we are developing for VR (Spoiler: we are, we will use a build flag.) 
    // Image Usage specifies what operations you use the images for, COLOR_ATTACH means we render directly to them, 
    // if you wanted to render to separate images for things like post processing, you can use TRANSFER_DST and use a 
    // memory operation to transfer the image to a swap chain, this is also a TODO item eventually.
    createSwapChainInfo.imageArrayLayers = 1;
    createSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // This handles swap chain images across multiple queue families, ie, if the graphics queue family is different from the present queue
    Global::QueueFamilyIndices indices = Global::findQueueFamilies(Global::physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    // Usage across multiple queue families without explicit transfer of ownership if they are different queue families.
    // Otherwise, no sharing without explicit handoffs, faster, but not easily supported with multiple families.
    // Presentation and Graphics families are usually merged on most hardware.
    if (indices.graphicsFamily != indices.presentFamily) {
     createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
     createSwapChainInfo.queueFamilyIndexCount = 2;
     createSwapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
       createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    // Transformation of image support.
    createSwapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    // Do NOT blend with other windows on the system.
    createSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createSwapChainInfo.presentMode = presentMode;
    // This is interesting, clip pixels that are obscured for performance, but that means you wont be able to read them reliably..
    // I am curious if this would affect screen-space rendering techniques, may be something to note.
    createSwapChainInfo.clipped = VK_TRUE;
    // This is something that needs to be implemented later, operations like resizing the window invalidate the swap chain and 
    // require you to recreate it and reference the old one specified here, will revisit in a few days.
    //createSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(Global::device, &createSwapChainInfo, nullptr, &Global::swapChain) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create the swap chain!!");
    }
    if(Global::enableValidationLayers) std::cout << "Swap Chain created successfully\n" << std::endl;

    vkGetSwapchainImagesKHR(Global::device, Global::swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(Global::device, Global::swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }
  void DeviceControl::destroySwapChain() {
    vkDestroySwapchainKHR(Global::device, Global::swapChain, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed Swap Chain safely\n" << std::endl;   
  }
  VkImageView DeviceControl::createImageView(VkImage image, VkFormat format, VkImageAspectFlags flags) {
    // This defines the parameters of a newly created image object!
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = flags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(Global::device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
  }
  void DeviceControl::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
  }
  void DeviceControl::destroyImageViews() {
    for (auto imageView : swapChainImageViews) {  
      vkDestroyImageView(Global::device, imageView, nullptr);
    }
    if(Global::enableValidationLayers) std::cout << "Image destroyed safely\n" << std::endl;
  }
// --------------------------------------- Getters & Setters ------------------------------------------ //
  VkFormat DeviceControl::getImageFormat() {
    return swapChainImageFormat;
  }
  std::vector<VkImageView> DeviceControl::getSwapChainImageViews() {
    return swapChainImageViews;
  }
  VkExtent2D DeviceControl::getSwapChainExtent() {
    return swapChainExtent;
  }
}

