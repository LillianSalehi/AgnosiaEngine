#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

class Buffers {

public:
  struct Vertex {
    // This defines what a vertex is!
    // We control the position, color and texture coordinate here!
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

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
  static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags props, VkBuffer &buffer,
                           VkDeviceMemory &bufferMemory);
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags);
  static void createIndexBuffer();
  static void createVertexBuffer();
  static void destroyBuffers();
  static VkBuffer &getVertexBuffer();
  static VkBuffer &getIndexBuffer();
  static void createDescriptorSetLayout();
  static void createUniformBuffers();
  static void updateUniformBuffer(uint32_t currentImage);
  static void destroyUniformBuffer();
  static void createDescriptorPool();
  static void createDescriptorSets();
  static void destroyDescriptorPool();
  static VkDescriptorPool &getDescriptorPool();
  static VkDescriptorSetLayout &getDescriptorSetLayout();
  static std::vector<VkDescriptorSet> &getDescriptorSets();

  static float *getObjPos();
  static float *getCamPos();
  static float *getCenterPos();
  static float *getUpDir();
  static float &getDepthField();
  static float *getDistanceField();
  static uint32_t getMaxFramesInFlight();
  static std::vector<VkCommandBuffer> &getCommandBuffers();
  static std::vector<VkBuffer> &getUniformBuffers();
  static std::vector<VkDeviceMemory> &getUniformBuffersMemory();
  static VkCommandPool &getCommandPool();
  static std::vector<Vertex> &getVertices();
  static std::vector<uint32_t> &getIndices();
};
