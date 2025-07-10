#include "../devicelibrary.h"
#include "../utils.h"
#include "buffers.h"
#include "model.h"
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>

std::vector<VkDescriptorSetLayout> modelSetLayouts;

uint32_t indicesSize;

// Select a binding for each descriptor type
constexpr int STORAGE_BINDING = 0;
constexpr int SAMPLER_BINDING = 1;
constexpr int IMAGE_BINDING = 2;
// Max count of each descriptor type
// You can query the max values for these with
// physicalDevice.getProperties().limits.maxDescriptorSet*******
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

VmaAllocator allocator;

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
      .vulkanApiVersion = VK_API_VERSION_1_4,

  };
  vmaCreateAllocator(&allocInfo, &allocator);
}

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
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
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
  VK_CHECK(vkCreateDescriptorSetLayout(DeviceControl::getDevice(), &layoutInfo, nullptr, &descriptorSetLayout));
}
void Buffers::createDescriptorPool() {

  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_COUNT},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMAGE_COUNT},
  };
  VkDescriptorPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
    .maxSets = 1,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data(),
  };
  
  VK_CHECK(vkCreateDescriptorPool(DeviceControl::getDevice(), &poolInfo, nullptr, &descriptorPool));
}
void Buffers::destroyDescriptorPool() {
  vkDestroyDescriptorPool(DeviceControl::getDevice(), descriptorPool, nullptr);
}
void Buffers::createDescriptorSet(std::vector<Model *> models) {
  VkDescriptorSetAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptorSetLayout,
  };
  VK_CHECK(vkAllocateDescriptorSets(DeviceControl::getDevice(), &allocInfo, &descriptorSet));

  std::vector<VkDescriptorImageInfo> imageInfoSet;
  imageInfoSet.resize(models.size());

  for (int i = 0; i < models.size(); i++) {
    imageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfoSet[i].imageView = models[i]->getMaterial().getDiffuseTexture().imageView;
    imageInfoSet[i].sampler = models[i]->getMaterial().getDiffuseTexture().sampler;
  }

  std::vector<VkWriteDescriptorSet> descriptorWrites{};
  descriptorWrites.resize(models.size());

  for (int i = 0; i < models.size(); i++) {
    descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i].dstSet = descriptorSet;
    descriptorWrites[i].dstBinding = SAMPLER_BINDING;
    descriptorWrites[i].dstArrayElement = i;
    descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i].descriptorCount = 1;
    descriptorWrites[i].pImageInfo = &imageInfoSet[i];
  }

  vkUpdateDescriptorSets(DeviceControl::getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type! (Buffers.cpp:164)");
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion = {
    .size = size,
  };
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
  };
  vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(DeviceControl::getGraphicsQueue());
  vkFreeCommandBuffers(DeviceControl::getDevice(), commandPool, 1, &commandBuffer);
}

Agnosia_T::AllocatedBuffer Buffers::createBuffer(size_t allocSize, VmaAllocationCreateFlags vmaFlags, VkBufferUsageFlags usageFlags, VmaMemoryUsage memUsage) {
  // Allocate a VMA Buffer and return it back.
  VkBufferCreateInfo bufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .size = allocSize,
    .usage = usageFlags,
  };
  VmaAllocationCreateInfo vmaAllocInfo = {
    .flags = vmaFlags,
    .usage = memUsage,
  };

  Agnosia_T::AllocatedBuffer buffer;
  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
  return buffer;
}

VkDescriptorPool &Buffers::getDescriptorPool() { return descriptorPool; }
VkDescriptorSet &Buffers::getDescriptorSet() { return descriptorSet; }
VkDescriptorSetLayout &Buffers::getDescriptorSetLayout() { return descriptorSetLayout; }

uint32_t Buffers::getMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; }
std::vector<VkCommandBuffer> &Buffers::getCommandBuffers() { return commandBuffers; }
VkCommandPool &Buffers::getCommandPool() { return commandPool; }
uint32_t Buffers::getIndicesSize() { return indicesSize; }

VmaAllocator Buffers::getAllocator() {
  return allocator;
}
