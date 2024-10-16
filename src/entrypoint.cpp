#include "entrypoint.h"
#include "global.h"

VkInstance vulkaninstance;

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
  debug_libs::Debug::checkUnavailableValidationLayers();        // Check if there is a mistake with our Validation Layers.

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

  debug_libs::Debug::vulkanDebugSetup(createInfo, vulkaninstance); // Handoff to the debug library to wrap the validation libs in! (And set the window up!)
}

void initVulkan() {
  // Initialize vulkan and set up pipeline.
  createInstance();
  debug_libs::Debug::setupDebugMessenger(vulkaninstance);        
  device_libs::DeviceControl::createSurface(vulkaninstance, Global::window);
  device_libs::DeviceControl::pickPhysicalDevice(vulkaninstance);
  device_libs::DeviceControl::createLogicalDevice();
  device_libs::DeviceControl::createSwapChain(Global::window);
  device_libs::DeviceControl::createImageViews();
  graphics_pipeline::Graphics::createRenderPass();
  buffers_libs::Buffers::createDescriptorSetLayout();
  graphics_pipeline::Graphics::createGraphicsPipeline();
  graphics_pipeline::Graphics::createCommandPool();
  texture_libs::Texture::createDepthResources();
  graphics_pipeline::Graphics::createFramebuffers();
  texture_libs::Texture::createTextureImage();
  texture_libs::Texture::createTextureImageView();
  texture_libs::Texture::createTextureSampler();
  modellib::Model::loadModel();
  buffers_libs::Buffers::createVertexBuffer();
  buffers_libs::Buffers::createIndexBuffer();
  buffers_libs::Buffers::createUniformBuffers();
  buffers_libs::Buffers::createDescriptorPool();
  buffers_libs::Buffers::createDescriptorSets();
  graphics_pipeline::Graphics::createCommandBuffer();
  render_present::Render::createSyncObject();
}

void mainLoop() {
  while (!glfwWindowShouldClose(Global::window)) {
    glfwPollEvents();
    render_present::Render::drawFrame();
  }
  vkDeviceWaitIdle(Global::device);
}

void cleanup() {
  render_present::Render::cleanupSwapChain();
  graphics_pipeline::Graphics::destroyGraphicsPipeline();
  graphics_pipeline::Graphics::destroyRenderPass();
  buffers_libs::Buffers::destroyUniformBuffer();
  buffers_libs::Buffers::destroyDescriptorPool();
  texture_libs::Texture::destroyTextureSampler();
  texture_libs::Texture::destroyTextureImage();
  vkDestroyDescriptorSetLayout(Global::device, Global::descriptorSetLayout, nullptr);
  buffers_libs::Buffers::destroyBuffers();
  render_present::Render::destroyFenceSemaphores();
  graphics_pipeline::Graphics::destroyCommandPool();

  vkDestroyDevice(Global::device, nullptr);
  if(Global::enableValidationLayers) {
    debug_libs::Debug::DestroyDebugUtilsMessengerEXT(vulkaninstance, nullptr);
  }
  device_libs::DeviceControl::destroySurface(vulkaninstance);
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

