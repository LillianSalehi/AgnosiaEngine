#include "../devicelibrary.h"
#include "../utils/types.h"
#include "../utils/helpers.h"
#include "buffers.h"
#include "graphicspipeline.h"
#include "../agnosiaimgui.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "render.h"
#include "texture.h"
#include "../utils/deletion.h"
 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

float lightPos[4] = {5.0f, 5.0f, 5.0f, 0.44f};
float lightColor[4] = {1.0f, 1.0f, 1.0f, 0.44f};
float camPos[4] = {3.0f, 3.0f, 3.0f, 0.44f};
float centerPos[4] = {0.0f, 0.0f, 0.0f, 0.44f};
float upDir[4] = {0.0f, 0.0f, 1.0f, 0.44f};
float depthField = 45.0f;
float distanceField[2] = {0.1f, 100.0f};

std::deque<Agnosia_T::Pipeline> graphicsHistory;
std::deque<Agnosia_T::Pipeline> fullscreenHistory;

void Graphics::createCommandPool() {
  // Commands in Vulkan are not executed using function calls, you have to
  // record the ops you wish to perform to command buffers, pools manage the
  // memory used by the buffer!
  DeviceControl::QueueFamilyIndices queueFamilyIndices = DeviceControl::findQueueFamilies(DeviceControl::getPhysicalDevice());

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  VK_CHECK(vkCreateCommandPool(DeviceControl::getDevice(), &poolInfo, nullptr, &Buffers::getCommandPool()));
  
  DeletionQueue::get().push_function([=](){vkDestroyCommandPool(DeviceControl::getDevice(), Buffers::getCommandPool(), nullptr);});
}
void Graphics::createCommandBuffer() {
  Buffers::getCommandBuffers().resize(Buffers::getMaxFramesInFlight());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = Buffers::getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)Buffers::getCommandBuffers().size();

  VK_CHECK(vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo, Buffers::getCommandBuffers().data()));
}
void Graphics::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, AssetCache& cache) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
  
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
  const VkRenderingAttachmentInfo colorAttachmentInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = Texture::getColorImage().imageView,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
      .resolveImageView = DeviceControl::getSwapChainImageViews()[imageIndex],
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
  };
  const VkRenderingAttachmentInfo depthAttachmentInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = Texture::getDepthImage().imageView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}},
  };

  const VkRenderingInfo renderInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = { .offset = {0, 0}, .extent = DeviceControl::getSwapChainExtent() },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
      .pDepthAttachment = &depthAttachmentInfo,
  };

  vkCmdBeginRendering(commandBuffer, &renderInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsHistory.front().pipeline);
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

  vkCmdSetLineWidth(commandBuffer, Gui::getLineWidth());

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsHistory.front().layout, 0, 1, &Buffers::getTextureDescriptorSets(), 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsHistory.front().layout, 1, 1, &Buffers::getSamplerDescriptorSet(), 0, nullptr);

  int modelID = 0;

  Agnosia_T::GPUBuffer gpuBuffer;
  
  
  gpuBuffer.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

  gpuBuffer.view = glm::lookAt(glm::vec3(camPos[0], camPos[1], camPos[2]),
                   glm::vec3(centerPos[0], centerPos[1], centerPos[2]),
                   glm::vec3(upDir[0], upDir[1], upDir[2]));

  gpuBuffer.proj = glm::perspective(glm::radians(depthField),
                    DeviceControl::getSwapChainExtent().width / (float)DeviceControl::getSwapChainExtent().height,
                    distanceField[0], distanceField[1]);
    
  // GLM was created for OpenGL, where the Y coordinate was inverted. This simply flips the sign.
  gpuBuffer.proj[1][1] *= -1;
  gpuBuffer.lightPos = glm::vec3(lightPos[0], lightPos[1], lightPos[2]);
  gpuBuffer.lightColor = glm::vec3(lightColor[0], lightColor[1], lightColor[2]);
  gpuBuffer.camPos = glm::vec3(camPos[0], camPos[1], camPos[2]);
  
  for (Model *model : cache.getModels()) {
    //printf("Model: %d\n", modelID);
    // Per model push constants
    gpuBuffer.vertexBuffer = model->getBuffers().vertexBufferAddress;
    gpuBuffer.objPosition = model->getPos();
    gpuBuffer.diffuseID = ((modelID+1)*4);
    gpuBuffer.metallicID = ((modelID+1)*4)+1;
    gpuBuffer.aoID = ((modelID+1)*4)+2;
    gpuBuffer.roughnessID = ((modelID+1)*4)+3;
    
      const size_t gpuBufferSize = sizeof(Agnosia_T::GPUBuffer);
      Agnosia_T::AllocatedBuffer gpuAllocatedBuffer = Buffers::createBuffer(gpuBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_AUTO);
      void *data = gpuAllocatedBuffer.info.pMappedData;
    
    
    // Copy the gpu buffer
    //memcpy((char*) data + (gpuBufferSize * modelID), &gpuBuffer, gpuBufferSize);
    memcpy(data, &gpuBuffer, gpuBufferSize);

    
    VkBufferDeviceAddressInfo gpuBufferDeviceAddressInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = gpuAllocatedBuffer.buffer,
    };
    Agnosia_T::GPUPushConstants pushConsts = {
      .gpuBufferAddress = vkGetBufferDeviceAddress(DeviceControl::getDevice(), &gpuBufferDeviceAddressInfo),
    };

    vkCmdPushConstants(commandBuffer, graphicsHistory.front().layout, VK_SHADER_STAGE_ALL, 0, sizeof(Agnosia_T::GPUPushConstants), &pushConsts);

    vkCmdBindIndexBuffer(commandBuffer, model->getBuffers().indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->getIndices()), 1, 0, 0, 0);
    modelID++;
    
    
  }
  

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fullscreenHistory.front().pipeline);
  
  if(cache.getModels().empty()) {
    // handle none
  }

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

  vkCmdPipelineBarrier2(Buffers::getCommandBuffers()[Render::getCurrentFrame()], &depInfo);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));
  
  //vmaDestroyBuffer(Buffers::getAllocator(), gpuAllocatedBuffer.buffer, gpuAllocatedBuffer.allocation);
  
}

float *Graphics::getCamPos() { return camPos; }
float *Graphics::getLightPos() { return lightPos; }
float *Graphics::getLightColor() { return lightColor; }
float *Graphics::getCenterPos() { return centerPos; }
float *Graphics::getUpDir() { return upDir; }
float &Graphics::getDepthField() { return depthField; }
float *Graphics::getDistanceField() { return distanceField; }


void Graphics::addGraphicsPipeline(Agnosia_T::Pipeline pipeline) {
    graphicsHistory.push_front(pipeline);
}
void Graphics::addFullscreenPipeline(Agnosia_T::Pipeline pipeline) {
    fullscreenHistory.push_front(pipeline);
}
