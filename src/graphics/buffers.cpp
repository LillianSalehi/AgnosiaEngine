#include "../devicelibrary.h"
#include "../types.h"
#include "buffers.h"
#include "texture.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<Agnosia_T::Vertex> vertices;
// Index buffer definition, showing which points to reuse.
std::vector<uint32_t> indices;

VmaAllocator _allocator;
Agnosia_T::GPUPushConstants pushConstants;

VkDescriptorPool descriptorPool;
VkDescriptorSetLayout descriptorSetLayout;
std::vector<VkDescriptorSet> descriptorSets;

VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;
std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void *> uniformBuffersMapped;

float objPos[4] = {0.0f, 0.0f, 0.0f, 0.44f};
float camPos[4] = {2.0f, 2.0f, 2.0f, 0.44f};
float centerPos[4] = {0.0f, 0.0f, 0.0f, 0.44f};
float upDir[4] = {0.0f, 0.0f, 1.0f, 0.44f};
float depthField = 45.0f;
float distanceField[2] = {0.1f, 100.0f};

const int MAX_FRAMES_IN_FLIGHT = 2;
struct UniformBufferObject {
  float time;
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

uint32_t Buffers::findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties) {
  // Graphics cards offer different types of memory to allocate from, here we
  // query to find the right type of memory for our needs. Query the available
  // types of memory to iterate over.
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(DeviceControl::getPhysicalDevice(),
                                      &memProperties);
  // iterate over and see if any of the memory types match our needs, in this
  // case, HOST_VISIBLE and HOST_COHERENT. These will be explained shortly.
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo,
                           &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  function(commandBuffer);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo,
                VK_NULL_HANDLE);
  vkQueueWaitIdle(DeviceControl::getGraphicsQueue());

  vkFreeCommandBuffers(DeviceControl::getDevice(), commandPool, 1,
                       &commandBuffer);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo,
                           &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo,
                VK_NULL_HANDLE);
  vkQueueWaitIdle(DeviceControl::getGraphicsQueue());

  vkFreeCommandBuffers(DeviceControl::getDevice(), commandPool, 1,
                       &commandBuffer);
}
void Buffers::createMemoryAllocator(VkInstance vkInstance) {
  VmaVulkanFunctions vulkanFuncs{
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };
  VmaAllocatorCreateInfo allocInfo{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = DeviceControl::getPhysicalDevice(),
      .device = DeviceControl::getDevice(),
      .pVulkanFunctions = &vulkanFuncs,
      .instance = vkInstance,
      .vulkanApiVersion = VK_API_VERSION_1_3,

  };
  vmaCreateAllocator(&allocInfo, &_allocator);
}
Agnosia_T::AllocatedBuffer Buffers::createBuffer(size_t allocSize,
                                                 VkBufferUsageFlags usage,
                                                 VmaMemoryUsage memUsage) {
  // Allocate the buffer we will use for Device Addresses
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .pNext = nullptr,
                                .size = allocSize,
                                .usage = usage};
  VmaAllocationCreateInfo vmaAllocInfo{
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = memUsage};

  Agnosia_T::AllocatedBuffer newBuffer;
  if (vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer,
                      &newBuffer.allocation, &newBuffer.info) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate a buffer using VMA!");
  }
  return newBuffer;
}

Agnosia_T::GPUMeshBuffers
Buffers::sendMesh(std::span<uint32_t> indices,
                  std::span<Agnosia_T::Vertex> vertices) {

  const size_t vertexBufferSize = vertices.size() * sizeof(Agnosia_T::Vertex);
  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  Agnosia_T::GPUMeshBuffers newSurface;

  // Create a Vertex Buffer here, infinitely easier than the old Vulkan method!
  newSurface.vertexBuffer = createBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);
  // Find the address of the vertex buffer!
  VkBufferDeviceAddressInfo deviceAddressInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = newSurface.vertexBuffer.buffer,
  };
  newSurface.vertexBufferAddress =
      vkGetBufferDeviceAddress(DeviceControl::getDevice(), &deviceAddressInfo);

  // Create the index buffer to iterate over and check for duplicate vertices
  newSurface.indexBuffer = createBuffer(indexBufferSize,
                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY);

  Agnosia_T::AllocatedBuffer stagingBuffer =
      createBuffer(vertexBufferSize + indexBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *data = stagingBuffer.allocation->GetMappedData();

  // Copy the vertex buffer
  memcpy(data, vertices.data(), vertexBufferSize);
  // Copy the index buffer
  memcpy((char *)data + vertexBufferSize, indices.data(), indexBufferSize);

  immediate_submit([&](VkCommandBuffer cmd) {
    VkBufferCopy vertexCopy{0};
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size = vertexBufferSize;

    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.vertexBuffer.buffer,
                    1, &vertexCopy);

    VkBufferCopy indexCopy{0};
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vertexBufferSize;
    indexCopy.size = indexBufferSize;

    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.indexBuffer.buffer, 1,
                    &indexCopy);
  });
  vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  return newSurface;
}

void Buffers::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(DeviceControl::getDevice(), &bufferInfo, nullptr,
                     &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(DeviceControl::getDevice(), buffer,
                                &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(DeviceControl::getDevice(), &allocInfo, nullptr,
                       &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(DeviceControl::getDevice(), buffer, bufferMemory, 0);
}

void Buffers::destroyBuffers() {
  vkDestroyBuffer(DeviceControl::getDevice(), indexBuffer, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), indexBufferMemory, nullptr);

  vkDestroyBuffer(DeviceControl::getDevice(), vertexBuffer, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), vertexBufferMemory, nullptr);
}

// ------------------------------ Uniform Buffer Setup
// -------------------------------- //
void Buffers::createDescriptorSetLayout() {
  // Create a table of pointers to data, a Descriptor Set!
  // --------------------- UBO Layout --------------------- //
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // Model-View-Projection matrix is in a single uniform buffer, so just 1
  // descriptor.
  uboLayoutBinding.descriptorCount = 1;
  // We are only using this buffer in the vertex shader, so set the flags thus.
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  // Immutable Samplers is relevant for image sampling.
  uboLayoutBinding.pImmutableSamplers = nullptr;

  // --------------- Texture Sampler Layout --------------- //
  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                          samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(DeviceControl::getDevice(), &layoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }
}
void Buffers::createUniformBuffers() {
  // Map the uniform buffer to memory as a pointer we can use to write data to
  // later. This stays mapped to memory for the applications lifetime. This
  // technique is called "persistent mapping", not having to map the buffer
  // every time we need to update it increases performance, though not free
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    vkMapMemory(DeviceControl::getDevice(), uniformBuffersMemory[i], 0,
                bufferSize, 0, &uniformBuffersMapped[i]);
  }
}
void Buffers::updateUniformBuffer(uint32_t currentImage) {
  // Update the uniform buffer every frame to change the position, but notably,
  // use chrono to know exactly how much to move so we aren't locked to the
  // framerate as the world time.

  static auto startTime = std::chrono::high_resolution_clock::now();
  // Calculate the time in seconds since rendering has began to floating point
  // precision.
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   currentTime - startTime)
                   .count();

  UniformBufferObject ubo{};
  ubo.time = time;
  // Modify the model projection transformation to rotate around the Z over
  // time.
  ubo.model = glm::translate(glm::mat4(1.0f),
                             glm::vec3(objPos[0], objPos[1], objPos[2]));
  // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f),
  //                         glm::vec3(0.0f, 0.0f, 1.0f));
  // Modify the view transformation to look at the object from above at a 45
  // degree angle. This takes the eye position, center position, and the up
  // direction.
  ubo.view = glm::lookAt(glm::vec3(camPos[0], camPos[1], camPos[2]),
                         glm::vec3(centerPos[0], centerPos[1], centerPos[2]),
                         glm::vec3(upDir[0], upDir[1], upDir[2]));
  // 45 degree field of view, set aspect ratio, and near and far clipping range.
  ubo.proj =
      glm::perspective(glm::radians(depthField),
                       DeviceControl::getSwapChainExtent().width /
                           (float)DeviceControl::getSwapChainExtent().height,
                       distanceField[0], distanceField[1]);

  // GLM was created for OpenGL, where the Y coordinate was inverted. This
  // simply flips the sign.
  ubo.proj[1][1] *= -1;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
void Buffers::destroyUniformBuffer() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(DeviceControl::getDevice(), uniformBuffers[i], nullptr);
    vkFreeMemory(DeviceControl::getDevice(), uniformBuffersMemory[i], nullptr);
  }
}
void Buffers::createDescriptorPool() {
  // Create a pool to be used to allocate descriptor sets.
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(DeviceControl::getDevice(), &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}
void Buffers::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(DeviceControl::getDevice(), &allocInfo,
                               descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = Texture::getTextureImageView();
    imageInfo.sampler = Texture::getTextureSampler();

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(DeviceControl::getDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}
void Buffers::destroyDescriptorPool() {
  vkDestroyDescriptorPool(DeviceControl::getDevice(), descriptorPool, nullptr);
}
VkDescriptorPool &Buffers::getDescriptorPool() { return descriptorPool; }
std::vector<VkDescriptorSet> &Buffers::getDescriptorSets() {
  return descriptorSets;
}
float *Buffers::getObjPos() { return objPos; }
float *Buffers::getCamPos() { return camPos; }
float *Buffers::getCenterPos() { return centerPos; }
float *Buffers::getUpDir() { return upDir; }
float &Buffers::getDepthField() { return depthField; }
float *Buffers::getDistanceField() { return distanceField; }
uint32_t Buffers::getMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; }
std::vector<VkCommandBuffer> &Buffers::getCommandBuffers() {
  return commandBuffers;
}
std::vector<VkBuffer> &Buffers::getUniformBuffers() { return uniformBuffers; }
std::vector<VkDeviceMemory> &Buffers::getUniformBuffersMemory() {
  return uniformBuffersMemory;
}
VkCommandPool &Buffers::getCommandPool() { return commandPool; }
VkDescriptorSetLayout &Buffers::getDescriptorSetLayout() {
  return descriptorSetLayout;
}
std::vector<Agnosia_T::Vertex> &Buffers::getVertices() { return vertices; }
std::vector<uint32_t> &Buffers::getIndices() { return indices; }
