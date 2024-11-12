#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>
#include <vector>

namespace Global {
// Global variables and includes we are going to use almost everywhere,
// validation layers hook into everything, and you need to check if they are
// enabled first, so that's one obvious global, as well as the glfw includes!

extern VkPhysicalDevice physicalDevice;
extern VkDevice device;

extern VkCommandPool commandPool;
extern std::vector<VkCommandBuffer> commandBuffers;

extern VkQueue graphicsQueue;
extern VkQueue presentQueue;

extern GLFWwindow *window;
extern VkSurfaceKHR surface;
extern uint32_t currentFrame;

extern std::vector<VkDescriptorSet> descriptorSets;
extern VkDescriptorSetLayout descriptorSetLayout;

extern VkImageView textureImageView;
extern VkSampler textureSampler;

extern VkImage depthImage;
extern VkImageView depthImageView;
extern VkDeviceMemory depthImageMemory;

extern VkSwapchainKHR swapChain;
extern std::vector<VkImageView> swapChainImageViews;

const std::string MODEL_PATH = "assets/models/teapot.obj";
const std::string TEXTURE_PATH = "assets/textures/checkermap.png";
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
  float time;
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};
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

extern std::vector<Vertex> vertices;
// Index buffer definition, showing which points to reuse.
extern std::vector<uint32_t> indices;
struct QueueFamilyIndices {
  // We need to check that the Queue families support graphics operations and
  // window presentation, sometimes they can support one or the other,
  // therefore, we take into account both for completion.
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
Global::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
} // namespace Global
