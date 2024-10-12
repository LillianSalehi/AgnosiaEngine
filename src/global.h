#pragma once
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>
#include "debug/vulkandebuglibs.h"
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Global {
  // Global variables and includes we are going to use almost everywhere, validation layers hook into everything, and you need to check if they are enabled first,
  // so that's one obvious global, as well as the glfw includes!
  extern const std::vector<const char*> validationLayers;
  extern const bool enableValidationLayers;
  extern VkDevice device;  
  extern VkCommandPool commandPool;
  extern std::vector<VkCommandBuffer> commandBuffers;  
  extern VkQueue graphicsQueue;
  extern VkQueue presentQueue;
  const int MAX_FRAMES_IN_FLIGHT = 2;
  extern GLFWwindow* window;
  extern VkDescriptorSetLayout descriptorSetLayout;
  extern uint32_t currentFrame;
  extern std::vector<VkDescriptorSet> descriptorSets;
  struct UniformBufferObject {
    float time;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
  
    static VkVertexInputBindingDescription getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription{};
      bindingDescription.binding = 0;
      bindingDescription.stride = sizeof(Vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
      std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[0].offset = offsetof(Vertex, pos);

      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset = offsetof(Vertex, color);
      return attributeDescriptions;
    }
  };
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  struct QueueFamilyIndices {
    // We need to check that the Queue families support graphics operations and window presentation, sometimes they can support one or the other,
    // therefore, we take into account both for completion.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };
  extern VkSwapchainKHR swapChain;
  extern VkSurfaceKHR surface;
  extern VkPhysicalDevice physicalDevice;
  Global::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
}
