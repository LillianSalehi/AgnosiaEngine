#include "agnosiaimgui.h"
#include "graphics/buffers.h"

namespace agnosia_imgui {

VkDescriptorPool imGuiDescriptorPool;

void initWindow() {

  ImGui::DragFloat3("Object Position", buffers_libs::Buffers::getObjPos());
  ImGui::DragFloat3("Camera Position", buffers_libs::Buffers::getCamPos());
  ImGui::DragFloat3("Center Position", buffers_libs::Buffers::getCenterPos());
  ImGui::DragFloat3("Up Direction", buffers_libs::Buffers::getUpDir());
  ImGui::DragFloat("Depth of Field", buffers_libs::Buffers::getDepthField(),
                   0.1f, 1.0f, 180.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
  ImGui::DragFloat2("Near and Far fields",
                    buffers_libs::Buffers::getDistanceField());
}

void drawTabs() {
  if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_Reorderable)) {
    if (ImGui::BeginTabItem("Transforms Control")) {
      initWindow();
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
    if (auto proc = vkGetDeviceProcAddr(Global::device, fn))
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

  ImGui_ImplGlfw_InitForVulkan(Global::window, true);

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
  if (vkCreateDescriptorPool(Global::device, &ImGuiPoolInfo, nullptr,
                             &imGuiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create ImGui descriptor pool!");
  }

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = device_libs::DeviceControl::getImageFormat(),
      .depthAttachmentFormat = texture_libs::Texture::findDepthFormat(),
  };

  ImGui_ImplVulkan_InitInfo initInfo{
      .Instance = instance,
      .PhysicalDevice = Global::physicalDevice,
      .Device = Global::device,
      .QueueFamily = Global::findQueueFamilies(Global::physicalDevice)
                         .graphicsFamily.value(),
      .Queue = Global::graphicsQueue,
      .DescriptorPool = imGuiDescriptorPool,
      .MinImageCount = Global::MAX_FRAMES_IN_FLIGHT,
      .ImageCount = Global::MAX_FRAMES_IN_FLIGHT,
      .MSAASamples = Global::perPixelSampleCount,
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
  };

  ImGui_ImplVulkan_Init(&initInfo);
}
} // namespace agnosia_imgui
