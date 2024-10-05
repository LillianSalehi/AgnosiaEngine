#include <cstdint>
#include <cstring>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Define a base class structure to handle public and private methods
class TriangleTestApplication {
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  #ifdef DEBUG
    const bool enableValidationLayers = true;
  #else 
    const bool enableValidationLayers = false;
  #endif

public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow *window;
  VkInstance instance;

  // Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for
  // now, and create window.
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Settings for the window are set, create window reference.
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  void initVulkan() {
    createInstance();
  }

  void createInstance() {
    if(enableValidationLayers && !checkValidationLayerSupport()) {
      throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Set application info for the vulkan instance!
	  VkApplicationInfo appInfo{};
 
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;                     // Tell vulkan that appInfo is a Application Info structure
    appInfo.pApplicationName = "Triangle Test";                             // Give the struct a name to use
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);                    // Create a Major Minor Patch version number for the application!
    appInfo.pEngineName = "No Engine";                                      // Give an internal name for the engine running
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);                         // Similar to the App version, give vulkan an *engine* version
    appInfo.apiVersion = VK_API_VERSION_1_0;                                // Tell vulkan what the highest API version we will allow this program to run on
  
    VkInstanceCreateInfo createInfo{};                                      // Define parameters of new vulkan instance
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;              // Tell vulkan this is a info structure 
    createInfo.pApplicationInfo = &appInfo;                                 // We just created a new appInfo structure, so we pass the pointer to it.
    
    // This gets a little weird, Vulkan is platform agnostic, so you need to figure out what extensions to interface with the current system are needed
    // So, to figure out what extension codes and how many to use, feed the pointer into *glfwGetRequiredInstanceExtensions*, which will get the necessary extensions!
    // From there, we can send that over to our createInfo Vulkan info struct to make it fully platform agnostic!
    uint32_t glfwExtensionCount = 0;                            
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    
    if(enableValidationLayers) {                                            // If we have validation layers, add them now, otherwise set it to 0
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
      createInfo.enabledLayerCount = 0;
    }
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);    // Finally create the Vulkan instance, passing in the info to create from, and the global instance to use!

    if(result != VK_SUCCESS) {                                              // vkCreateInstance returns a VkResult, if its anything but success, we exit immediately. (VK_SUCCESS == 0)
      throw std::runtime_error("Failed to create vulkan instance!");
    }
  }
 
  bool checkValidationLayerSupport() {                                      // This function is used to check Validation Layer Support, validation layers are the debug trace tools in the Vulkan SDK.
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
 
  void mainLoop() {
    // Update window whilst open
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    // Cleanup window when destroyed.
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

};

int main() {
  TriangleTestApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
