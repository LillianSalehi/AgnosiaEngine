#include "entrypoint.h"
DeviceControl::devicelibrary deviceLibs;
Debug::vulkandebuglibs debugController;
Graphics::graphicspipeline graphicsPipeline;
RenderPresent::render renderPresentation;
BuffersLibraries::buffers buffers;
TextureLibraries::texture texture;
ModelLib::model model;
VkInstance vulkaninstance;

//TODO: add global instances?

// Getters and Setters!
void EntryApp::setFramebufferResized(bool setter) {
  framebufferResized = setter;
}
bool EntryApp::getFramebufferResized() const {
  return framebufferResized;
}
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto app = reinterpret_cast<EntryApp*>(glfwGetWindowUserPointer(window));
  app->setFramebufferResized(true);
}
    // Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for
    // now, and create window.
void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Settings for the window are set, create window reference.
  Global::window = glfwCreateWindow(Global::WIDTH, Global::HEIGHT, "Trimgles :o", nullptr, nullptr);
  glfwSetWindowUserPointer(Global::window, &EntryApp::getInstance());
  glfwSetFramebufferSizeCallback(Global::window, framebufferResizeCallback);
}

void createInstance() {
  debugController.checkUnavailableValidationLayers();           // Check if there is a mistake with our Validation Layers.

  // Set application info for the vulkan instance!
	VkApplicationInfo appInfo{};
 
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;           // Tell vulkan that appInfo is a Application Info structure
  appInfo.pApplicationName = "Triangle Test";                   // Give the struct a name to use
  appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);          // Create a Major Minor Patch version number for the application!
  appInfo.pEngineName = "Agnosia Engine";                       // Give an internal name for the engine running
  appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);               // Similar to the App version, give vulkan an *engine* version
  appInfo.apiVersion = VK_API_VERSION_1_3;                      // Tell vulkan what the highest API version we will allow this program to run on
  
  VkInstanceCreateInfo createInfo{};                            // Define parameters of new vulkan instance
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;    // Tell vulkan this is a info structure 
  createInfo.pApplicationInfo = &appInfo;                       // We just created a new appInfo structure, so we pass the pointer to it.

  debugController.vulkanDebugSetup(createInfo, vulkaninstance);       // Handoff to the debug library to wrap the validation libs in! (And set the window up!)
}

void initVulkan() {
  createInstance();
  debugController.setupDebugMessenger(vulkaninstance);                // The debug messenger is out holy grail, it gives us Vulkan related debug info when built with the -DDEBUG flag (as per the makefile)
  deviceLibs.createSurface(vulkaninstance, Global::window);
  deviceLibs.pickPhysicalDevice(vulkaninstance);
  deviceLibs.createLogicalDevice();
  deviceLibs.createSwapChain(Global::window);
  deviceLibs.createImageViews();
  graphicsPipeline.createRenderPass();
  buffers.createDescriptorSetLayout();
  graphicsPipeline.createGraphicsPipeline();
  graphicsPipeline.createCommandPool();
  texture.createDepthResources();
  graphicsPipeline.createFramebuffers();
  texture.createTextureImage();
  texture.createTextureImageView();
  texture.createTextureSampler();
  model.loadModel();
  buffers.createVertexBuffer();
  buffers.createIndexBuffer();
  buffers.createUniformBuffers();
  buffers.createDescriptorPool();
  buffers.createDescriptorSets();
  graphicsPipeline.createCommandBuffer();
  renderPresentation.createSyncObject();
}

void mainLoop() {                                               // This loop just updates the GLFW window.
  while (!glfwWindowShouldClose(Global::window)) {
    glfwPollEvents();
    renderPresentation.drawFrame();
  }
  vkDeviceWaitIdle(Global::device);
}

void cleanup() {                                                // Similar to the last handoff, destroy the utils in a safe manner in the library!
  renderPresentation.cleanupSwapChain();
  texture.createTextureSampler();
  texture.destroyTextureImage();
  buffers.destroyUniformBuffer();
  buffers.destroyDescriptorPool();
  vkDestroyDescriptorSetLayout(Global::device, Global::descriptorSetLayout, nullptr);
  graphicsPipeline.destroyGraphicsPipeline();
  graphicsPipeline.destroyRenderPass();

  buffers.destroyBuffers();
  renderPresentation.destroyFenceSemaphores();
  graphicsPipeline.destroyCommandPool();

  vkDestroyDevice(Global::device, nullptr);
  if(Global::enableValidationLayers) {
    debugController.DestroyDebugUtilsMessengerEXT(vulkaninstance, nullptr);
  }
  deviceLibs.destroySurface(vulkaninstance);
  vkDestroyInstance(vulkaninstance, nullptr);
  glfwDestroyWindow(Global::window);
  glfwTerminate();
}
// External Functions
EntryApp& EntryApp::getInstance() {
  static EntryApp instance;
  return instance;
}
EntryApp::EntryApp() : initialized(false), framebufferResized(false) {}

void EntryApp::initialize() {
  initialized = true;
}
bool EntryApp::isInitialized() const {
  return initialized;
}

void EntryApp::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

