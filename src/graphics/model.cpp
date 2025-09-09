#include "buffers.h"
#include "model.h"
#include <stdexcept>
#include "../devicelibrary.h"

#define TINY_OBJ_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/hash.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"
#include <cstring>
#include "../utils/deletion.h"

// chatgpt did this and the haters can WEEP fuck hash functions.
namespace std {
template <> struct hash<Agnosia_T::Vertex> {
  size_t operator()(Agnosia_T::Vertex const &vertex) const {
    size_t hashPos = hash<glm::vec3>()(vertex.pos);
    size_t hashColor = hash<glm::vec3>()(vertex.color);
    size_t hashUV = hash<glm::vec2>()(vertex.uv);
    size_t hashNormal = hash<glm::vec3>()(vertex.normal);

    // Combine all hashes
    return ((hashPos ^ (hashColor << 1)) >> 1) ^ (hashUV << 1) ^ (hashNormal << 2);
  }
};

} // namespace std
void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = Buffers::getCommandPool();
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

  vkFreeCommandBuffers(DeviceControl::getDevice(), Buffers::getCommandPool(), 1,
                       &commandBuffer);
}

Model::Model(const std::string &modelID, const Material &material, const std::string &modelPath, const glm::vec3 &objPos)
  : ID(modelID), material(material), objPosition(objPos), modelPath(modelPath) {

  std::vector<Agnosia_T::Vertex> vertices;
  // Index buffer definition, showing which points to reuse.
  std::vector<uint32_t> indices;
  tinyobj::ObjReaderConfig readerConfig;
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(this->modelPath, readerConfig)) {
    if (!reader.Error().empty()) {
      throw std::runtime_error(reader.Error());
    }
    if (!reader.Warning().empty()) {
      throw std::runtime_error(reader.Warning());
    }
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();
  auto &materials = reader.GetMaterials();

  std::unordered_map<Agnosia_T::Vertex, uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Agnosia_T::Vertex vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]};
      // TODO: Small fix here, handle if there are no UV's unwrapped for the
      // model.
      //       As of now, if it is not unwrapped, it segfaults on texCoord
      //       assignment. Obviously we should always have UV's, but it
      //       shouldn't crash, just unwrap in a default method.
      vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }

  const size_t vertexBufferSize = vertices.size() * sizeof(Agnosia_T::Vertex);
  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  this->buffers.vertexBuffer = Buffers::createBuffer(vertexBufferSize,
                                                  VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                  VMA_MEMORY_USAGE_AUTO);
    
  // Find the address of the vertex buffer!
  VkBufferDeviceAddressInfo vertexDeviceAddressInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = this->buffers.vertexBuffer.buffer,
  };
  this->buffers.vertexBufferAddress = vkGetBufferDeviceAddress(DeviceControl::getDevice(), &vertexDeviceAddressInfo);

  // Create the index buffer to iterate over and check for duplicate vertices
  this->buffers.indexBuffer = Buffers::createBuffer(indexBufferSize,
                                                 VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VMA_MEMORY_USAGE_AUTO);
  // Find the address of the vertex buffer!
  VkBufferDeviceAddressInfo indexDeviceAddressInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = this->buffers.indexBuffer.buffer,
  };
  this->buffers.indexBufferAddress = vkGetBufferDeviceAddress(DeviceControl::getDevice(), &indexDeviceAddressInfo);

  // Allocate a buffer to use memory that will first, request the ability to *be* mapped, then persistently mapped and fetched.
  Agnosia_T::AllocatedBuffer stagingBuffer = Buffers::createBuffer(
      vertexBufferSize + indexBufferSize,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_AUTO);

  void *data = stagingBuffer.info.pMappedData;

  // Copy the vertex buffer
  memcpy(data, vertices.data(), vertexBufferSize);
  // Copy the index buffer
  memcpy((char *)data + vertexBufferSize, indices.data(), indexBufferSize);

  immediate_submit([&](VkCommandBuffer cmd) {
    VkBufferCopy vertexCopy{0};
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size = vertexBufferSize;

    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, this->buffers.vertexBuffer.buffer, 1, &vertexCopy);

    VkBufferCopy indexCopy{0};
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vertexBufferSize;
    indexCopy.size = indexBufferSize;

    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, this->buffers.indexBuffer.buffer, 1, &indexCopy);
  });
  
  vmaDestroyBuffer(Buffers::getAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
  
  this->verticeCount = vertices.size();
  this->indiceCount = indices.size();
  
  Agnosia_T::AllocatedBuffer vertexBuffer = this->buffers.vertexBuffer;
  Agnosia_T::AllocatedBuffer indexBuffer = this->buffers.indexBuffer;
  DeletionQueue::get().push_function([=](){vmaDestroyBuffer(Buffers::getAllocator(), indexBuffer.buffer, indexBuffer.allocation);});
  DeletionQueue::get().push_function([=](){vmaDestroyBuffer(Buffers::getAllocator(), vertexBuffer.buffer, vertexBuffer.allocation);});
}

std::string Model::getID() { return this->ID; }
glm::vec3 &Model::getPos() { return this->objPosition; }
Material &Model::getMaterial() { return this->material; }
Agnosia_T::GPUMeshBuffers Model::getBuffers() { return this->buffers; }
uint32_t Model::getIndices() { return this->indiceCount; }
uint32_t Model::getVertices() { return this->verticeCount; }

