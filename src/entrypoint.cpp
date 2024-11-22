#include "entrypoint.h"
#include "global.h"
#include "graphics/texture.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

VkInstance vulkaninstance;

// Getters and Setters!
void EntryApp::setFramebufferResized(bool setter) {
  framebufferResized = setter;
}
bool EntryApp::getFramebufferResized() const { return framebufferResized; }
static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  auto app = reinterpret_cast<EntryApp *>(glfwGetWindowUserPointer(window));
  app->setFramebufferResized(true);
}

// Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for
// now, and create window.
void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // Settings for the window are set, create window reference.
  Global::window = glfwCreateWindow(Global::WIDTH, Global::HEIGHT,
                                    "Trimgles :o", nullptr, nullptr);
  glfwSetWindowUserPointer(Global::window, &EntryApp::getInstance());
  glfwSetFramebufferSizeCallback(Global::window, framebufferResizeCallback);
}

void createInstance() {

  // Set application info for the vulkan instance!
  VkApplicationInfo appInfo{};

  appInfo.sType =
      VK_STRUCTURE_TYPE_APPLICATION_INFO;     // Tell vulkan that appInfo is a
                                              // Application Info structure
  appInfo.pApplicationName = "Triangle Test"; // Give the struct a name to use
  appInfo.applicationVersion = VK_MAKE_VERSION(
      1, 0,
      0); // Create a Major Minor Patch version number for the application!
  appInfo.pEngineName =
      "Agnosia Engine"; // Give an internal name for the engine running
  appInfo.engineVersion = VK_MAKE_VERSION(
      1, 0, 0); // Similar to the App version, give vulkan an *engine* version
  appInfo.apiVersion =
      VK_API_VERSION_1_3; // Tell vulkan what the highest API version we will
                          // allow this program to run on

  // This gets a little weird, Vulkan is platform agnostic, so you need to
  // figure out what extensions to interface with the current system are needed
  // So, to figure out what extension codes and how many to use, feed the
  // pointer into *glfwGetRequiredInstanceExtensions*, which will get the
  // necessary extensions! From there, we can send that over to our createInfo
  // Vulkan info struct to make it fully platform agnostic!
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  VkInstanceCreateInfo createInfo{}; // Define parameters of new vulkan instance
  createInfo.sType =
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Tell vulkan this is a info
                                              // structure
  createInfo.pApplicationInfo =
      &appInfo; // We just created a new appInfo structure, so we pass the
                // pointer to it.
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (vkCreateInstance(&createInfo, nullptr, &vulkaninstance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

void initVulkan() {
  // Initialize volk and continue if successful.
  volkInitialize();
  // Initialize vulkan and set up pipeline.
  createInstance();
  volkLoadInstance(vulkaninstance);
  device_libs::DeviceControl::createSurface(vulkaninstance, Global::window);
  device_libs::DeviceControl::pickPhysicalDevice(vulkaninstance);
  device_libs::DeviceControl::createLogicalDevice();
  volkLoadDevice(Global::device);
  device_libs::DeviceControl::createSwapChain(Global::window);
  device_libs::DeviceControl::createImageViews();
  buffers_libs::Buffers::createDescriptorSetLayout();
  graphics_pipeline::Graphics::createGraphicsPipeline();
  graphics_pipeline::Graphics::createCommandPool();
  texture_libs::Texture::createColorResources();
  texture_libs::Texture::createDepthResources();
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
  render_present::Render::init_imgui(vulkaninstance);
}

void mainLoop() {
  while (!glfwWindowShouldClose(Global::window)) {
    glfwPollEvents();

    render_present::Render::drawImGui();
    render_present::Render::drawFrame();
  }
  vkDeviceWaitIdle(Global::device);
}

void cleanup() {
  render_present::Render::cleanupSwapChain();
  graphics_pipeline::Graphics::destroyGraphicsPipeline();
  buffers_libs::Buffers::destroyUniformBuffer();
  buffers_libs::Buffers::destroyDescriptorPool();
  texture_libs::Texture::destroyTextureSampler();
  texture_libs::Texture::destroyTextureImage();
  vkDestroyDescriptorSetLayout(Global::device, Global::descriptorSetLayout,
                               nullptr);
  buffers_libs::Buffers::destroyBuffers();
  render_present::Render::destroyFenceSemaphores();
  graphics_pipeline::Graphics::destroyCommandPool();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  vkDestroyDevice(Global::device, nullptr);
  device_libs::DeviceControl::destroySurface(vulkaninstance);
  vkDestroyInstance(vulkaninstance, nullptr);
  glfwDestroyWindow(Global::window);
  glfwTerminate();
}

// External Functions
EntryApp &EntryApp::getInstance() {
  static EntryApp instance;
  return instance;
}
EntryApp::EntryApp() : initialized(false), framebufferResized(false) {}

void EntryApp::initialize() { initialized = true; }
bool EntryApp::isInitialized() const { return initialized; }

void EntryApp::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}
