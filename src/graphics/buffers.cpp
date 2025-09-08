#include "../devicelibrary.h"
#include "../utils/helpers.h"
#include "buffers.h"
#include "model.h"
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "../utils/deletion.h"

std::vector<VkDescriptorSetLayout> modelSetLayouts;

uint32_t indicesSize;

// Select a binding for each descriptor type
constexpr int STORAGE_BINDING = 0;
constexpr int IMAGE_BINDING = 1;
constexpr int SAMPLER_BINDING = 2;

// Max count of each descriptor type
// You can query the max values for these with
// physicalDevice.getProperties().limits.maxDescriptorSet*******
constexpr int STORAGE_COUNT = 65536;
constexpr int SAMPLER_COUNT = 65536;
constexpr int IMAGE_COUNT = 65536;

// Create descriptor pool
VkDescriptorPool descriptorPool;

VkDescriptorSetLayout texturesSetLayouts;
VkDescriptorSet texturesSets;

VkSampler sampler;
VkDescriptorSetLayout samplerDescriptorSetLayout;
VkDescriptorSet samplerDescriptorSet;

VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

const int MAX_FRAMES_IN_FLIGHT = 2;

VmaAllocator allocator;

void Buffers::createMemoryAllocator(VkInstance vkInstance) {
  VmaVulkanFunctions vulkanFuncs = {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };
  VmaAllocatorCreateInfo allocInfo = {
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = DeviceControl::getPhysicalDevice(),
      .device = DeviceControl::getDevice(),
      .pVulkanFunctions = &vulkanFuncs,
      .instance = vkInstance,
      .vulkanApiVersion = VK_API_VERSION_1_4,

  };
  vmaCreateAllocator(&allocInfo, &allocator);
  DeletionQueue::get().push_function([=](){vmaDestroyAllocator(allocator);});
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
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
      .descriptorCount = SAMPLER_COUNT,
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr,
  };

  VkDescriptorSetLayoutBinding imageLayoutBinding = {
      .binding = IMAGE_BINDING,
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      .descriptorCount = IMAGE_COUNT,
      .stageFlags = VK_SHADER_STAGE_ALL,
      .pImmutableSamplers = nullptr,
  };

  std::vector<VkDescriptorSetLayoutBinding> bindings = {
      storageLayoutBinding, imageLayoutBinding, samplerLayoutBinding};

  std::vector<VkDescriptorBindingFlags> bindingFlags = {
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
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
  
    VK_CHECK(vkCreateDescriptorSetLayout(DeviceControl::getDevice(), &layoutInfo, nullptr, &texturesSetLayouts));
    DeletionQueue::get().push_function([=](){vkDestroyDescriptorSetLayout(DeviceControl::getDevice(), texturesSetLayouts, nullptr);});
    
  
    VK_CHECK(vkCreateDescriptorSetLayout(DeviceControl::getDevice(), &layoutInfo, nullptr, &samplerDescriptorSetLayout));
    DeletionQueue::get().push_function([=](){vkDestroyDescriptorSetLayout(DeviceControl::getDevice(), samplerDescriptorSetLayout, nullptr);});
}
void Buffers::createDescriptorPool() {

  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_COUNT},
      {VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_COUNT},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMAGE_COUNT},
  };
  VkDescriptorPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
    .maxSets = 5,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data(),
  };
  
  VK_CHECK(vkCreateDescriptorPool(DeviceControl::getDevice(), &poolInfo, nullptr, &descriptorPool));
  DeletionQueue::get().push_function([=](){vkDestroyDescriptorPool(DeviceControl::getDevice(), descriptorPool, nullptr);});
}
void Buffers::createDescriptorSet(std::vector<Model *> models) {
  // Create the allocater struct for the textures.
  VkDescriptorSetAllocateInfo textureAllocInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &texturesSetLayouts,
  };
  VK_CHECK(vkAllocateDescriptorSets(DeviceControl::getDevice(), &textureAllocInfo, &texturesSets));
  // Create the allocater struct for the samplers.
  VkDescriptorSetAllocateInfo samplerAllocInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &samplerDescriptorSetLayout,
  };
  VK_CHECK(vkAllocateDescriptorSets(DeviceControl::getDevice(), &samplerAllocInfo, &samplerDescriptorSet));
  
  for(int model = 0; model < models.size(); model++) {
    // Textures for each model
    VkDescriptorImageInfo modelTexInfo[4];
    for(int i = 0; i < 4; i++) {
      modelTexInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    modelTexInfo[0].imageView = models[model]->getMaterial().getDiffuseTexture()->getImageView();
    modelTexInfo[1].imageView = models[model]->getMaterial().getMetallicTexture()->getImageView();
    modelTexInfo[2].imageView = models[model]->getMaterial().getAOTexture()->getImageView();
    modelTexInfo[3].imageView = models[model]->getMaterial().getRoughnessTexture()->getImageView();

    VkWriteDescriptorSet modelTexWriter = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = texturesSets,
      .dstBinding = IMAGE_BINDING,
      .dstArrayElement = static_cast<uint32_t>(4*(model+1)),
      .descriptorCount = 4,
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      .pImageInfo = modelTexInfo,
    };
    vkUpdateDescriptorSets(DeviceControl::getDevice(), 1, &modelTexWriter, 0, nullptr);
  }  

  // Now we create the one sampler we are going to use right now.
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(DeviceControl::getPhysicalDevice(), &properties);

  VkSamplerCreateInfo samplerInfo = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.0f,
    .maxLod = VK_LOD_CLAMP_NONE,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
  };

  VK_CHECK(vkCreateSampler(DeviceControl::getDevice(), &samplerInfo, nullptr, &sampler));
  DeletionQueue::get().push_function([=](){vkDestroySampler(DeviceControl::getDevice(), sampler, nullptr);});
  
  VkDescriptorImageInfo samplerInfoSet = {
    .sampler = sampler,
  };
  
  VkWriteDescriptorSet samplerWriteSet = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = samplerDescriptorSet,
    .dstBinding = SAMPLER_BINDING,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
    .pImageInfo = &samplerInfoSet,
  };

  vkUpdateDescriptorSets(DeviceControl::getDevice(), 1, &samplerWriteSet, 0, nullptr);
}

uint32_t Buffers::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  // Graphics cards offer different types of memory to allocate from, here we
  // query to find the right type of memory for our needs. Query the available
  // types of memory to iterate over.
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(DeviceControl::getPhysicalDevice(), &memProperties);
  // iterate over and see if any of the memory types match our needs, in this
  // case, HOST_VISIBLE and HOST_COHERENT. These will be explained shortly.
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type!");
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
VkDescriptorSet &Buffers::getTextureDescriptorSets() { return texturesSets; }
VkDescriptorSetLayout &Buffers::getTextureDescriptorSetLayouts() { return texturesSetLayouts; }

VkDescriptorSet &Buffers::getSamplerDescriptorSet() { return samplerDescriptorSet; }
VkDescriptorSetLayout &Buffers::getSamplerDescriptorSetLayout() { return samplerDescriptorSetLayout; }

uint32_t Buffers::getMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; }
std::vector<VkCommandBuffer> &Buffers::getCommandBuffers() { return commandBuffers; }
VkCommandPool &Buffers::getCommandPool() { return commandPool; }
uint32_t Buffers::getIndicesSize() { return indicesSize; }

VmaAllocator Buffers::getAllocator() {
  return allocator;
}
