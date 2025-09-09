#include "agnosiaimgui.h"
#include "assetcache.h"
#include "devicelibrary.h"
#include "entrypoint.h"
#include "graphics/buffers.h"
#include "graphics/graphicspipeline.h"

#include "graphics/model.h"
#include "graphics/pipelinebuilder.h"
#include "graphics/render.h"
#include "graphics/texture.h"
#include "utils/helpers.h"
#include "utils/types.h"
#include <memory>
#include "utils/deletion.h"

#include "volk.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VkInstance vulkaninstance;
GLFWwindow *window;
AssetCache cache;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

DeletionQueue* DeletionQueue::instance = nullptr;

// Getters and Setters!
void EntryApp::setFramebufferResized(bool setter) {
  framebufferResized = setter;
}

bool EntryApp::getFramebufferResized() const { return framebufferResized; }

static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto app = reinterpret_cast<EntryApp *>(glfwGetWindowUserPointer(window));
  app->setFramebufferResized(true);
}

// Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for now, and create window.
void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // Settings for the window are set, create window reference.
  window = glfwCreateWindow(WIDTH, HEIGHT, "Agnosia", nullptr, nullptr);
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

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vulkaninstance));
  DeletionQueue::get().push_function([=](){vkDestroyInstance(vulkaninstance, nullptr);});
}
void initAgnosia() {
  Texture* checkermap = cache.fetchLoadTexture("checkermap", "assets/textures/checkermap.png");
  Texture* metallicPlaceholder = cache.fetchLoadTexture("metallicPlaceholder", "assets/textures/placeholderMetallic.jpg");
  Texture* roughnessPlaceholder = cache.fetchLoadTexture("roughnessPlaceholder", "assets/textures/placeholderRoughness.jpg");
  Texture* ambientOcclusionPlaceholder = cache.fetchLoadTexture("ambientOcclusionPlaceholder", "assets/textures/placeholderAO.jpg");
  
  auto sphereMaterial = std::make_unique<Material>("sphereMaterial", checkermap, metallicPlaceholder, roughnessPlaceholder, ambientOcclusionPlaceholder);
  auto stanfordDragonMaterial = std::make_unique<Material>("stanfordDragonMaterial", checkermap, metallicPlaceholder, roughnessPlaceholder, ambientOcclusionPlaceholder);
  auto teapotMaterial = std::make_unique<Material>("teapotMaterial", checkermap, metallicPlaceholder, roughnessPlaceholder, ambientOcclusionPlaceholder);
  cache.store(std::move(sphereMaterial));
  cache.store(std::move(stanfordDragonMaterial));
  cache.store(std::move(teapotMaterial));

  auto uvSphere = std::make_unique<Model>("uvSphere", *cache.findMaterial("sphereMaterial"), "assets/models/UVSphere.obj", glm::vec3(0.0f, 0.0f, 0.0f));
  auto stanfordDragon = std::make_unique<Model>("stanfordDragon", *cache.findMaterial("stanfordDragonMaterial"), "assets/models/StanfordDragon800k.obj", glm::vec3(0.0f, 2.0f, 0.0f));
  auto teapot = std::make_unique<Model>("teapot", *cache.findMaterial("teapotMaterial"), "assets/models/teapot.obj", glm::vec3(1.0f, -3.0f, -1.0f));
  cache.store(std::move(uvSphere));
  cache.store(std::move(stanfordDragon));
  cache.store(std::move(teapot));

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
  Buffers::createMemoryAllocator(vulkaninstance);
  DeviceControl::createImageViews();
  Buffers::createDescriptorSetLayout();
  PipelineBuilder builder;
    
  Agnosia_T::Pipeline graphics = builder.setCullMode(VK_CULL_MODE_BACK_BIT).Build();

  Agnosia_T::Pipeline fullscreen = builder.setCullMode(VK_CULL_MODE_NONE)
                                          .setVertexShader("src/shaders/fullscreen.vert")
                                          .setFragmentShader("src/shaders/fullscreen.frag")
                                          .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                                          .Build();
                                      
  Graphics::addGraphicsPipeline(graphics);
  Graphics::addFullscreenPipeline(fullscreen);
  Buffers::createDescriptorPool();
  Graphics::createCommandPool();
  initAgnosia();
  // Image creation MUST be after command pool, because command buffers are utilized.
  Texture::createColorImage();
  Texture::createDepthImage();
  Buffers::createDescriptorSet(cache.getModels());
  Graphics::createCommandBuffer();
  Render::createSyncObject();
  

  Gui::initImgui(vulkaninstance);
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    Gui::drawImGui(cache);
    Render::drawFrame(cache);
  }
  vkDeviceWaitIdle(DeviceControl::getDevice());
}

void cleanup() {
  DeletionQueue::get().push_function([=](){Render::cleanupSwapChain();});
  DeletionQueue::get().flush();
  
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
  initVulkan();
  mainLoop();
  cleanup();
}
