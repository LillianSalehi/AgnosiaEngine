#include "vulkandebuglibs.h"
using namespace Debug;

// This is our messenger object! It handles passing along debug messages to the debug callback we will also set.
VkDebugUtilsMessengerEXT debugMessenger;
// This is the set of "layers" to hook into. Basically, layers are used to tell the messenger what data we want, its a filter. *validation* is the general blanket layer to cover incorrect usage.

std::vector<const char*> getRequiredExtensions() {
  // This gets a little weird, Vulkan is platform agnostic, so you need to figure out what extensions to interface with the current system are needed
  // So, to figure out what extension codes and how many to use, feed the pointer into *glfwGetRequiredInstanceExtensions*, which will get the necessary extensions!
  // From there, we can send that over to our createInfo Vulkan info struct to make it fully platform agnostic!
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  
  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  
  if(Global::enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( 
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) { 
    // One hell of a function, this is using the *PFN_vkDestroyDebugUtilsMessengerEXT* prototype, the prototype for an, "Application-defined debug messenger callback function".
    // The VKAPI_CALL and VKAPI_ATTR ensure that the function has the right signature for vulkan to call it. The callback message can be anything from a diagnostic to error!
    // You can even sort by those diagnostics with their flags, since they are just integers, maybe TODO?
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    std::cout << "\n";

    return VK_FALSE;
}



void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)  { 
  // There is absolutely nothing about this i like, those long ass flags for messageType and Severity are just fucking hex values. Khronos should never cook again ToT
  // On a serious note, this is just a struct to define the parameters of the debug messenger, nothing super special.
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // Optional
}

void vulkandebuglibs::vulkanDebugSetup(VkInstanceCreateInfo& createInfo, VkInstance& instance) {
  // This function is quite useful, we first populate the debug create info structure, all the parameters dictating how the debug messenger will operate.
  // The reason we populate the debug messenger so late is actually on purpose, we need to set the createInfo, which depends on the debugMessenger info,
  // and if we set it before the creation of the instance, we cant debug vkCreateInstance or vkDestroyInstance! It's timed perfectly as of now.
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  auto extensions = getRequiredExtensions();  
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if(Global::enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(Global::validationLayers.size());
    createInfo.ppEnabledLayerNames = Global::validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

void vulkandebuglibs::checkUnavailableValidationLayers() {
  // Check if we are trying to hook validation layers in without support.
  if(Global::enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers request, but not available! Are your SDK path variables set?");
  }
}

bool vulkandebuglibs::checkValidationLayerSupport() {
  // This function is used to check Validation Layer Support, validation layers are the debug trace tools in the Vulkan SDK.
  // layerCount will be used as the var to keep track of the number of requested validation layer
  // VkLayerProperties is a structure with data on the layername, desc, versions and etc. 

  uint32_t layerCount;                                                    
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for(const char* layerName : Global::validationLayers) { 
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

VkResult CreateDebugUtilsMessengerEXT(
  VkInstance instance, 
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
  const VkAllocationCallbacks* pAllocator, 
  VkDebugUtilsMessengerEXT* pDebugMessenger) {
  // This function builds out debug messenger structure!
  // It's a little odd, we have to look up the address of the vkCreateDebugUtilsMessengerEXT ourselves because its an extension function, 
  // therefore, not auto-loaded.

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vulkandebuglibs::DestroyDebugUtilsMessengerEXT(VkInstance instance, 
  const VkAllocationCallbacks* pAllocator) {
  // We are doing kind of the same thing as before in the create function, find the address of the DestroyDebugUtils function, and call it.

  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if(func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void vulkandebuglibs::setupDebugMessenger(VkInstance& vulkanInstance) {
  // This is a pretty simple function! we just pass in the values to build the debug messenger, populate the structure with the data we want, 
  // and safely create it, covering for runtime errors as per usual, this is the first thing that will be called!
  if(!Global::enableValidationLayers) return;
  
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);
  
  if(CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up the Debug Messenger!");
  }
}

