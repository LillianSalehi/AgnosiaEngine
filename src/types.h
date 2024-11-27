#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "vk_mem_alloc.h"
#include <array>
#include <glm/glm.hpp>

class Agnosia_T {
public:
  struct Vertex {
    // This defines what a vertex is!
    // We control the position, color and texture coordinate here!
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(8) glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription{};
      bindingDescription.binding = 0;
      bindingDescription.stride = sizeof(Vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
      std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[0].offset = offsetof(Vertex, pos);

      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset = offsetof(Vertex, color);

      attributeDescriptions[2].binding = 0;
      attributeDescriptions[2].location = 2;
      attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
      return attributeDescriptions;
    }
    bool operator==(const Vertex &other) const {
      return pos == other.pos && color == other.color &&
             texCoord == other.texCoord;
    }
  };
  struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
  };
  struct GPUMeshBuffers {
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
  };
  struct GPUPushConstants {
    VkDeviceAddress vertexBuffer;
  };
};
