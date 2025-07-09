#include "agnosiaimgui.h"
#include "devicelibrary.h"
#include "entrypoint.h"
#include "graphics/buffers.h"
#include "graphics/graphicspipeline.h"

#include "graphics/model.h"
#include "graphics/pipelinebuilder.h"
#include "graphics/render.h"
#include "graphics/texture.h"
#include "types.h"

#define VK_NO_PROTOTYPES
#include "volk.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <stdexcept>
VkInstance vulkaninstance;
GLFWwindow *window;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

// Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for now, and create window.
void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // Settings for the window are set, create window reference.
  window = glfwCreateWindow(WIDTH, HEIGHT, "Trimgles :o", nullptr, nullptr);
  glfwSetWindowUserPointer(window, &EntryApp::getInstance());
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void createInstance() {
  // Set application info for the vulkan instance!
  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Agnosia",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Agnosia Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_4,
  };

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
  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data(),
  };

  if (vkCreateInstance(&createInfo, nullptr, &vulkaninstance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance! (Entrypoint.cpp:87)");
  }
}
void initAgnosia() {
  Material *sphereMaterial = new Material("sphereMaterial", "assets/textures/checkermap.png", 0.1f, 1.0f, 32);
  Material *stanfordDragonMaterial = new Material("stanfordDragonMaterial", "assets/textures/checkermap.png", 0.1f, 1.0f, 256);
  Material *teapotMaterial = new Material("teapotMaterial", "assets/textures/checkermap.png", 0.1f, 1.0f, 128);

  Model *uvSphere = new Model("uvSphere", *sphereMaterial, "assets/models/UVSphere.obj", glm::vec3(0.0f, 0.0f, 0.0f));
  Model *stanfordDragon = new Model("stanfordDragon", *stanfordDragonMaterial, "assets/models/StanfordDragon800k.obj", glm::vec3(0.0f, 2.0f, 0.0f));
  Model *teapot = new Model("teapot", *teapotMaterial, "assets/models/teapot.obj", glm::vec3(1.0f, -3.0f, -1.0f));
}
void initVulkan() {
  
  // Initialize volk and continue if successful.
  volkInitialize();
  // Initialize vulkan and set up pipeline.
  createInstance();
  volkLoadInstance(vulkaninstance);
  DeviceControl::createSurface(vulkaninstance, window);
  DeviceControl::pickPhysicalDevice(vulkaninstance);
  DeviceControl::createLogicalDevice();
  volkLoadDevice(DeviceControl::getDevice());
  DeviceControl::createSwapChain(window);
  Model::createMemoryAllocator(vulkaninstance);
  DeviceControl::createImageViews();
  Buffers::createDescriptorSetLayout();
  
  PipelineBuilder builder;
    
  Agnosia_T::Pipeline graphics = builder.setCullMode(VK_CULL_MODE_BACK_BIT)
                                        .Build();

  Agnosia_T::Pipeline fullscreen = builder.setCullMode(VK_CULL_MODE_NONE)
                                    .setVertexShader("src/shaders/fullscreen.vert")
                                    .setFragmentShader("src/shaders/fullscreen.frag")
                                    .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                                    .Build();
                                      
  Graphics::addGraphicsPipeline(graphics);
  Graphics::addFullscreenPipeline(fullscreen);
  Graphics::createCommandPool();
  // Image creation MUST be after command pool, because command
  // buffers are utilized.
  Model::populateModels();
  Texture::createMaterialTextures(Model::getInstances());
  Texture::createColorResources();
  Texture::createDepthResources();
  Buffers::createDescriptorPool();
  Buffers::createDescriptorSet(Model::getInstances());
  Graphics::createCommandBuffer();
  Render::createSyncObject();

  Gui::initImgui(vulkaninstance);
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    Gui::drawImGui();
    Render::drawFrame();
  }
  vkDeviceWaitIdle(DeviceControl::getDevice());
}

void cleanup() {
  Render::cleanupSwapChain();
  Graphics::destroyPipelines();
  Buffers::destroyDescriptorPool();
  Model::destroyTextures();

  vkDestroyDescriptorSetLayout(DeviceControl::getDevice(),
                               Buffers::getDescriptorSetLayout(), nullptr);

  Buffers::destroyBuffers();
  Render::destroyFenceSemaphores();
  Graphics::destroyCommandPool();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  vkDestroyDevice(DeviceControl::getDevice(), nullptr);
  DeviceControl::destroySurface(vulkaninstance);
  vkDestroyInstance(vulkaninstance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

// Thread safe singleton fetching with mutex
EntryApp& EntryApp::getInstance() {
  static EntryApp instance;
  return instance;
}

EntryApp::EntryApp() : initialized(false), framebufferResized(false) {}

void EntryApp::initialize() { initialized = true; }
bool EntryApp::isInitialized() const { return initialized; }
GLFWwindow *EntryApp::getWindow() { return window; }

void EntryApp::run() {
  initWindow();
  initAgnosia();
  initVulkan();
  mainLoop();
  cleanup();
}
