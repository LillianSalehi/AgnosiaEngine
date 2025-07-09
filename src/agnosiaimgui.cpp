#include "agnosiaimgui.h"
#include "devicelibrary.h"
#include "entrypoint.h"
#include "graphics/buffers.h"
#include "graphics/graphicspipeline.h"
#include "graphics/pipelinebuilder.h"
#include "graphics/texture.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "types.h"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

PipelineBuilder builder;
Agnosia_T::Pipeline graphicsSolid;
Agnosia_T::Pipeline graphicsWireframe;
Agnosia_T::Pipeline fullscreenSolid;
Agnosia_T::Pipeline fullscreenWireframe;

VkDescriptorPool imGuiDescriptorPool;
static bool wireframe = false;
float lineWidth = 1.0f;

void initTransformsWindow() {
  if (ImGui::TreeNode("Model Transforms")) {
    for (Model *model : Model::getInstances()) {

      ImGui::DragFloat3(model->getID().c_str(), const_cast<float *>(glm::value_ptr(model->getPos())));
    }
    ImGui::TreePop();
  }
  if(ImGui::TreeNode("Camera Transforms")) {
    ImGui::DragFloat3("Camera Position", Graphics::getCamPos());
    ImGui::DragFloat3("Light Position", Graphics::getLightPos());
    ImGui::DragFloat3("Center Position", Graphics::getCenterPos());
    ImGui::DragFloat("Depth of Field", &Graphics::getDepthField(), 0.1f, 1.0f,
                     180.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat2("Near and Far fields", Graphics::getDistanceField());
    ImGui::TreePop();
  }
  
}
void initRenderWindow() {
  if(ImGui::Checkbox("Wireframe?", &wireframe)) {
    // Rebuild graphics pipeline if setting is changed.

    if(wireframe) {
      Graphics::addGraphicsPipeline(graphicsWireframe);
      Graphics::addFullscreenPipeline(fullscreenWireframe);
      
    } else {                                  
      Graphics::addGraphicsPipeline(graphicsSolid);
      Graphics::addFullscreenPipeline(fullscreenSolid);
    }    
  }
  ImGui::DragFloat("Line Width", &lineWidth, 1.0f, 1.0f, 64.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
  
  for(Model *model : Model::getInstances()) {
    
    if(ImGui::Button(("Kill " + model->getID()).c_str())) {
      Model::destroyModel(model->getID());
    }
    
    int polycount =  model->getIndices()/3;
    ImGui::Text("Polycount: %d", polycount);
  }
  
}

void drawTabs() {
  if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_Reorderable)) {
    if (ImGui::BeginTabItem("Transforms Control")) {
      initTransformsWindow();
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Render Control")) {
      initRenderWindow();
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}

void Gui::drawImGui() {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

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
  ImGui_ImplVulkan_LoadFunctions([](const char *fn, void *data) {
    return (*(decltype(load_vk_func) *)data)(fn);
  }, &load_vk_func);

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
  if (vkCreateDescriptorPool(DeviceControl::getDevice(), &ImGuiPoolInfo, nullptr, &imGuiDescriptorPool) != VK_SUCCESS) {
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
      .QueueFamily = DeviceControl::findQueueFamilies(DeviceControl::getPhysicalDevice()).graphicsFamily.value(),
      .Queue = DeviceControl::getGraphicsQueue(),
      .DescriptorPool = imGuiDescriptorPool,
      .MinImageCount = Buffers::getMaxFramesInFlight(),
      .ImageCount = Buffers::getMaxFramesInFlight(),
      .MSAASamples = DeviceControl::getPerPixelSampleCount(),
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
  };

  ImGui_ImplVulkan_Init(&initInfo);

  
  graphicsSolid = builder.setCullMode(VK_CULL_MODE_NONE)
                         .setPolygonMode(VK_POLYGON_MODE_FILL)
                         .Build();
  graphicsWireframe = builder.setCullMode(VK_CULL_MODE_NONE)
                             .setPolygonMode(VK_POLYGON_MODE_LINE)
                             .Build();

  fullscreenSolid = builder.setCullMode(VK_CULL_MODE_NONE)
                           .setVertexShader("src/shaders/fullscreen.vert")
                           .setFragmentShader("src/shaders/fullscreen.frag")
                           .setPolygonMode(VK_POLYGON_MODE_FILL)
                           .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                           .Build();
  
  fullscreenWireframe = builder.setCullMode(VK_CULL_MODE_NONE)
                               .setVertexShader("src/shaders/fullscreen.vert")
                               .setFragmentShader("src/shaders/fullscreen.frag")
                               .setPolygonMode(VK_POLYGON_MODE_LINE)
                               .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                               .Build();
}
bool Gui::getWireframe() {
  return wireframe;
}
float Gui::getLineWidth() {
  return lineWidth;
}
