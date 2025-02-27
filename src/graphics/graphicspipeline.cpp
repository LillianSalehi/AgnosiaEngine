#include "../devicelibrary.h"
#include "../types.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "pipelinebuilder.h"
#include "render.h"
#include "texture.h"
#include <fstream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "../agnosiaimgui.h"
#include <iostream>
#include <vulkan/vulkan_core.h>

float lightPos[4] = {5.0f, 5.0f, 5.0f, 0.44f};
float camPos[4] = {3.0f, 3.0f, 3.0f, 0.44f};
float centerPos[4] = {0.0f, 0.0f, 0.0f, 0.44f};
float upDir[4] = {0.0f, 0.0f, 1.0f, 0.44f};
float depthField = 45.0f;
float distanceField[2] = {0.1f, 100.0f};
float lineWidth = 1.0;

std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                             VK_DYNAMIC_STATE_SCISSOR};
                                         

Agnosia_T::Pipeline graphics;
Agnosia_T::Pipeline fullscreen;

                                         
void Graphics::destroyPipelines() {
  vkDestroyPipeline(DeviceControl::getDevice(), graphics.pipeline, nullptr);
  vkDestroyPipelineLayout(DeviceControl::getDevice(), graphics.layout, nullptr);

  vkDestroyPipeline(DeviceControl::getDevice(), fullscreen.pipeline, nullptr);
  vkDestroyPipelineLayout(DeviceControl::getDevice(), fullscreen.layout, nullptr);
}
void Graphics::createFullscreenPipeline() {
    PipelineBuilder builder;
    if(Gui::getWireframe()) {
        fullscreen = builder.setCullMode(VK_CULL_MODE_NONE)
                                            .setVertexShader("src/shaders/fullscreen.vert.spv")
                                            .setFragmentShader("src/shaders/fullscreen.frag.spv")
                                            .setLineWidth(lineWidth)
                                            .setPolygonMode(VK_POLYGON_MODE_LINE).Build();
    } else {
        fullscreen = builder.setCullMode(VK_CULL_MODE_NONE)
                                            .setVertexShader("src/shaders/fullscreen.vert.spv")
                                            .setFragmentShader("src/shaders/fullscreen.frag.spv")
                                            .Build();
    }
}
void Graphics::createGraphicsPipeline() {    
    PipelineBuilder builder;
    if(Gui::getWireframe()) {
        graphics = builder.setCullMode(VK_CULL_MODE_NONE)
                                      .setLineWidth(lineWidth)
                                      .setPolygonMode(VK_POLYGON_MODE_LINE)
                                      .Build();
    } else {
        graphics = builder.setCullMode(VK_CULL_MODE_NONE)
                                      .setLineWidth(lineWidth)
                                      .Build();
    }
}

void Graphics::createCommandPool() {
  // Commands in Vulkan are not executed using function calls, you have to
  // record the ops you wish to perform to command buffers, pools manage the
  // memory used by the buffer!
  DeviceControl::QueueFamilyIndices queueFamilyIndices =
      DeviceControl::findQueueFamilies(DeviceControl::getPhysicalDevice());

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(DeviceControl::getDevice(), &poolInfo, nullptr,
                          &Buffers::getCommandPool()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}
void Graphics::destroyCommandPool() {
  vkDestroyCommandPool(DeviceControl::getDevice(), Buffers::getCommandPool(),
                       nullptr);
}
void Graphics::createCommandBuffer() {
  Buffers::getCommandBuffers().resize(Buffers::getMaxFramesInFlight());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = Buffers::getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)Buffers::getCommandBuffers().size();

  if (vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo,
                               Buffers::getCommandBuffers().data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers");
  }
}
void Graphics::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
  const VkImageMemoryBarrier2 imageMemoryBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .pNext = nullptr,
      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = DeviceControl::getSwapChainImages()[imageIndex],
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  const VkDependencyInfo dependencyInfo{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .pNext = nullptr,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier,
  };
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  // ------------------- DYNAMIC RENDER INFO ---------------------- //

  const VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = Texture::getColorImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
      .resolveImageView = DeviceControl::getSwapChainImageViews()[imageIndex],
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
  };
  const VkRenderingAttachmentInfo depthAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = Texture::getDepthImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}},
  };

  const VkRenderingInfo renderInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {.offset = {0, 0},
                     .extent = DeviceControl::getSwapChainExtent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
      .pDepthAttachment = &depthAttachmentInfo,
  };

  vkCmdBeginRendering(commandBuffer, &renderInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphics.pipeline);
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)DeviceControl::getSwapChainExtent().width;
  viewport.height = (float)DeviceControl::getSwapChainExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = DeviceControl::getSwapChainExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  int texID = 0;
  for (Model *model : Model::getInstances()) {

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphics.layout, 0, 1, &Buffers::getDescriptorSet(),
                            0, nullptr);

    Agnosia_T::GPUPushConstants pushConsts;
    pushConsts.vertexBuffer = model->getBuffers().vertexBufferAddress;
    pushConsts.objPosition = model->getPos();
    pushConsts.lightPos = glm::vec3(lightPos[0], lightPos[1], lightPos[2]);
    pushConsts.camPos = glm::vec3(camPos[0], camPos[1], camPos[2]);
    pushConsts.textureID = texID;
    pushConsts.ambient = model->getMaterial().getAmbient();
    pushConsts.spec = model->getMaterial().getSpecular();
    pushConsts.shine = model->getMaterial().getShininess();

    pushConsts.model =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    pushConsts.view =
        glm::lookAt(glm::vec3(camPos[0], camPos[1], camPos[2]),
                    glm::vec3(centerPos[0], centerPos[1], centerPos[2]),
                    glm::vec3(upDir[0], upDir[1], upDir[2]));

    pushConsts.proj =
        glm::perspective(glm::radians(depthField),
                         DeviceControl::getSwapChainExtent().width /
                             (float)DeviceControl::getSwapChainExtent().height,
                         distanceField[0], distanceField[1]);
    // GLM was created for OpenGL, where the Y coordinate was inverted. This
    // simply flips the sign.
    pushConsts.proj[1][1] *= -1;

    vkCmdPushConstants(commandBuffer, graphics.layout, VK_SHADER_STAGE_ALL, 0,
                       sizeof(Agnosia_T::GPUPushConstants), &pushConsts);

    vkCmdBindIndexBuffer(commandBuffer, model->getBuffers().indexBuffer.buffer,
                         0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->getIndices()),
                     1, 0, 0, 0);
    texID++;
  }
  

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    fullscreen.pipeline);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
  
  vkCmdEndRendering(commandBuffer);

  const VkImageMemoryBarrier2 prePresentImageBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .pNext = nullptr,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = DeviceControl::getSwapChainImages()[imageIndex],
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  const VkDependencyInfo depInfo{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .pNext = nullptr,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &prePresentImageBarrier,
  };

  vkCmdPipelineBarrier2(Buffers::getCommandBuffers()[Render::getCurrentFrame()],
                        &depInfo);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

float *Graphics::getCamPos() { return camPos; }
float *Graphics::getLightPos() { return lightPos; }
float *Graphics::getCenterPos() { return centerPos; }
float *Graphics::getUpDir() { return upDir; }
float &Graphics::getDepthField() { return depthField; }
float *Graphics::getDistanceField() { return distanceField; }
float &Graphics::getLineWidth() { return lineWidth; }
