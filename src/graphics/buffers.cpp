#include "buffers.h"
#include <iostream>
#include <vulkan/vulkan_core.h>

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
namespace Buffers {

  const std::vector<Global::Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} 
  };
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Graphics cards offer different types of memory to allocate from, here we query to find the right type of memory for our needs.
    // Query the available types of memory to iterate over.
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(Global::physicalDevice, &memProperties);
    // iterate over and see if any of the memory types match our needs, in this case, HOST_VISIBLE and HOST_COHERENT. These will be explained shortly.
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    throw std::runtime_error("failed to find suitable memory type!");
  }

  void bufferslibrary::createVertexBuffer() {
    // Create a Vertex Buffer to hold the vertex information in memory so it doesn't have to be hardcoded!
    // Size denotes the size of the buffer in bytes, usage in this case is the buffer behaviour, using a bitwise OR.
    // Sharing mode denostes the same as the images in the swap chain! in this case, only the graphics queue uses this buffer, so we make it exclusive.
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.pNext = NULL;
    std::cerr << &bufferInfo << " " << &vertexBuffer << "\n";
    if (vkCreateBuffer(Global::device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to create vertex buffer!");
    }
    // Query the memory requirements of the buffer.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Global::device, vertexBuffer, &memRequirements);
  
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(Global::device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    vkBindBufferMemory(Global::device, vertexBuffer, vertexBufferMemory, 0);

    void* data;
    vkMapMemory(Global::device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferInfo.size);
    vkUnmapMemory(Global::device, vertexBufferMemory);
  }
  void bufferslibrary::destroyVertexBuffer() {
    vkDestroyBuffer(Global::device, vertexBuffer, nullptr);
    vkFreeMemory(Global::device, vertexBufferMemory, nullptr);
  }
  VkBuffer bufferslibrary::getVertexBuffer() {
    return vertexBuffer;
  }
  std::vector<Global::Vertex> bufferslibrary::getVertices() {
    return vertices;
  }
}
