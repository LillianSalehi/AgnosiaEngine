
#include "debug/VulkanDebugLibs.h"
#include "DeviceLibrary.h"
#include "debug/VulkanDebugLibs.h"
#include "global.h"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// Define a base class structure to handle public and private methods
class TriangleTestApplication {

public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  DeviceControl::DeviceLibrary deviceLibs;
  Debug::VulkanDebugLibs debugController;

  GLFWwindow* window;
  VkInstance instance;
  VkDevice device;

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
    debugController.setupDebugMessenger(instance);                          // The debug messenger is out holy grail, it gives us Vulkan related debug info when built with the -DNDEBUG flag (as per the makefile)
    deviceLibs.createSurface(instance, window);
    deviceLibs.pickPhysicalDevice(instance);
    deviceLibs.createLogicalDevice(device);
  }

  void createInstance() {
    debugController.checkUnavailableValidationLayers();                     // Check if there is a mistake with our Validation Layers.

    // Set application info for the vulkan instance!
	  VkApplicationInfo appInfo{};
 
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;           // Tell vulkan that appInfo is a Application Info structure
    appInfo.pApplicationName = "Triangle Test";                   // Give the struct a name to use
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);          // Create a Major Minor Patch version number for the application!
    appInfo.pEngineName = "Agnosia Engine";                       // Give an internal name for the engine running
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);               // Similar to the App version, give vulkan an *engine* version
    appInfo.apiVersion = VK_API_VERSION_1_1;                      // Tell vulkan what the highest API version we will allow this program to run on
  
    VkInstanceCreateInfo createInfo{};                            // Define parameters of new vulkan instance
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;    // Tell vulkan this is a info structure 
    createInfo.pApplicationInfo = &appInfo;                       // We just created a new appInfo structure, so we pass the pointer to it.

    debugController.vulkanDebugSetup(createInfo, instance);                 // Handoff to the debug library to wrap the validation libs in! (And set the window up!)
  }

  void mainLoop() {                                               // This loop just updates the GLFW window.
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {                                                // Similar to the last handoff, destroy the debug util in a safe manner in the library!
    vkDestroyDevice(device, nullptr);
    if(Global::enableValidationLayers) {
      debugController.DestroyDebugUtilsMessengerEXT(instance, nullptr);
    }
 
    deviceLibs.destroySurface(instance);
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

