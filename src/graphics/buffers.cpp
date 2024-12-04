#include "../devicelibrary.h"

#include "buffers.h"
#include "model.h"

#include <cstdint>
#include <cstring>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <stdexcept>
#include <vulkan/vulkan_core.h>

std::vector<VkDescriptorSetLayout> modelSetLayouts;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

uint32_t indicesSize;

// Select a binding for each descriptor type
constexpr int STORAGE_BINDING = 0;
constexpr int SAMPLER_BINDING = 1;
constexpr int IMAGE_BINDING = 2;
// Max count of each descriptor type
// You can query the max values for these with
// physicalDevice.getProperties().limits.maxDescriptrorSet*******
constexpr int STORAGE_COUNT = 65536;
constexpr int SAMPLER_COUNT = 65536;
constexpr int IMAGE_COUNT = 65536;

// Create descriptor pool

VkDescriptorPool descriptorPool;
VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorSet descriptorSet;

VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

const int MAX_FRAMES_IN_FLIGHT = 2;

void Buffers::createDescriptorSetLayout() {
  // Create a table of pointers to data, a Descriptor Set!

  VkDescriptorSetLayoutBinding storageLayoutBinding = {
      .binding = STORAGE_BINDING,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = STORAGE_COUNT,
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr,
  };

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {
      .binding = SAMPLER_BINDING,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = SAMPLER_COUNT,
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr,
  };

  VkDescriptorSetLayoutBinding imageLayoutBinding = {
      .binding = IMAGE_BINDING,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .descriptorCount = IMAGE_COUNT,
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr,
  };

  std::vector<VkDescriptorSetLayoutBinding> bindings = {
      storageLayoutBinding, imageLayoutBinding, samplerLayoutBinding};

  std::vector<VkDescriptorBindingFlags> bindingFlags = {
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
          VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
          VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
          VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
  };
  VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingsFlags = {
      .sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount = 3,
      .pBindingFlags = bindingFlags.data(),
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,

      .pNext = &setLayoutBindingsFlags,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data(),

  };
  if (vkCreateDescriptorSetLayout(DeviceControl::getDevice(), &layoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }
}
void Buffers::createDescriptorPool() {

  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_COUNT},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMAGE_COUNT},
  };
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

  if (vkCreateDescriptorPool(DeviceControl::getDevice(), &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}
void Buffers::destroyDescriptorPool() {
  vkDestroyDescriptorPool(DeviceControl::getDevice(), descriptorPool, nullptr);
}
void Buffers::createDescriptorSet(std::vector<Model *> models) {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(DeviceControl::getDevice(), &allocInfo,
                               &descriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  std::vector<VkDescriptorImageInfo> imageInfoSet;
  imageInfoSet.resize(models.size());

  for (int i = 0; i < models.size(); i++) {
    imageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfoSet[i].imageView = models[i]->getMaterial().getTextureView();
    imageInfoSet[i].sampler = models[i]->getMaterial().getTextureSampler();
  }

  std::vector<VkWriteDescriptorSet> descriptorWrites{};
  descriptorWrites.resize(models.size());

  for (int i = 0; i < models.size(); i++) {
    descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i].dstSet = descriptorSet;
    descriptorWrites[i].dstBinding = SAMPLER_BINDING;
    descriptorWrites[i].dstArrayElement = i;
    descriptorWrites[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i].descriptorCount = 1;
    descriptorWrites[i].pImageInfo = &imageInfoSet[i];
  }

  vkUpdateDescriptorSets(DeviceControl::getDevice(),
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

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
VkDescriptorPool &Buffers::getDescriptorPool() { return descriptorPool; }
VkDescriptorSet &Buffers::getDescriptorSet() { return descriptorSet; }
VkDescriptorSetLayout &Buffers::getDescriptorSetLayout() {
  return descriptorSetLayout;
}

void Buffers::destroyBuffers() {
  vkDestroyBuffer(DeviceControl::getDevice(), indexBuffer, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), indexBufferMemory, nullptr);

  vkDestroyBuffer(DeviceControl::getDevice(), vertexBuffer, nullptr);
  vkFreeMemory(DeviceControl::getDevice(), vertexBufferMemory, nullptr);
}

uint32_t Buffers::getMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; }
std::vector<VkCommandBuffer> &Buffers::getCommandBuffers() {
  return commandBuffers;
}

VkCommandPool &Buffers::getCommandPool() { return commandPool; }
uint32_t Buffers::getIndicesSize() { return indicesSize; }
