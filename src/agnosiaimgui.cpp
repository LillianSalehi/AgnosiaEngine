#include "agnosiaimgui.h"
#include "devicelibrary.h"
#include "entrypoint.h"
#include "graphics/buffers.h"
#include "graphics/texture.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdexcept>

VkDescriptorPool imGuiDescriptorPool;

void initImGuiWindow() {

  ImGui::DragFloat3("Object Position", Buffers::getObjPos());
  ImGui::DragFloat3("Camera Position", Buffers::getCamPos());
  ImGui::DragFloat3("Center Position", Buffers::getCenterPos());
  ImGui::DragFloat3("Up Direction", Buffers::getUpDir());
  ImGui::DragFloat("Depth of Field", &Buffers::getDepthField(), 0.1f, 1.0f,
                   180.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
  ImGui::DragFloat2("Near and Far fields", Buffers::getDistanceField());
}

void drawTabs() {
  if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_Reorderable)) {
    if (ImGui::BeginTabItem("Transforms Control")) {
      initImGuiWindow();
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}

void Gui::drawImGui() {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  // 2. Show a simple window that we create ourselves. We use a Begin/End pair
  // to create a named window.

  ImGui::Begin("Agnosia Debug");

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

  drawTabs();

  ImGui::End();

  ImGui::Render();
}

void Gui::initImgui(VkInstance instance) {
  auto load_vk_func = [&](const char *fn) {
    if (auto proc = vkGetDeviceProcAddr(DeviceControl::getDevice(), fn))
      return proc;
    return vkGetInstanceProcAddr(instance, fn);
  };
  ImGui_ImplVulkan_LoadFunctions(
      [](const char *fn, void *data) {
        return (*(decltype(load_vk_func) *)data)(fn);
      },
      &load_vk_func);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // TODO
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(EntryApp::getWindow(), true);

  VkDescriptorPoolSize ImGuiPoolSizes[]{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };
  VkDescriptorPoolCreateInfo ImGuiPoolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = ImGuiPoolSizes,
  };
  if (vkCreateDescriptorPool(DeviceControl::getDevice(), &ImGuiPoolInfo,
                             nullptr, &imGuiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create ImGui descriptor pool!");
  }

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &DeviceControl::getImageFormat(),
      .depthAttachmentFormat = Texture::findDepthFormat(),
  };

  ImGui_ImplVulkan_InitInfo initInfo{
      .Instance = instance,
      .PhysicalDevice = DeviceControl::getPhysicalDevice(),
      .Device = DeviceControl::getDevice(),
      .QueueFamily =
          DeviceControl::findQueueFamilies(DeviceControl::getPhysicalDevice())
              .graphicsFamily.value(),
      .Queue = DeviceControl::getGraphicsQueue(),
      .DescriptorPool = imGuiDescriptorPool,
      .MinImageCount = Buffers::getMaxFramesInFlight(),
      .ImageCount = Buffers::getMaxFramesInFlight(),
      .MSAASamples = DeviceControl::getPerPixelSampleCount(),
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
  };

  ImGui_ImplVulkan_Init(&initInfo);
}
