#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>

class Agnosia_T {
public:
  struct Vertex {
    // This defines what a vertex is!
    // We control the position, color and texture coordinate here!
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    bool operator==(const Vertex &other) const {
      return pos == other.pos && normal == other.normal &&
             color == other.color && uv == other.uv;
    }
  };
  struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
  };
  struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
  };
  struct GPUMeshBuffers {
    AllocatedBuffer indexBuffer;
    VkDeviceAddress indexBufferAddress;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
  };

  struct GPUPushConstants {
    VkDeviceAddress vertexBuffer;
    glm::vec3 objPosition;
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    glm::vec3 camPos;
    int textureID;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

  enum PipelineStage  {
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    COMPUTE_SHADER,
    RAYGEN_SHADER,
    MISS_SHADER,
    CLOSEST_HIT_SHADER,
    ANY_HIT_SHADER,
    INTERSECTION_SHADER,
  };
  
};

