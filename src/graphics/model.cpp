#include "buffers.h"
#include "model.h"
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <stdexcept>
#include "../devicelibrary.h"
#include "../utils.h"

#define TINY_OBJ_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// This is a container for ALL model instances alive
std::vector<Model *> Model::instances;
VmaAllocator _allocator;
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
void Model::createMemoryAllocator(VkInstance vkInstance) {
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
  vmaCreateAllocator(&allocInfo, &_allocator);
}
Agnosia_T::AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
  // Allocate the buffer we will use for Device Addresses
  VkBufferCreateInfo bufferInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .size = allocSize,
    .usage = usage
  };
  VmaAllocationCreateInfo vmaAllocInfo{
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage = memUsage
  };

  Agnosia_T::AllocatedBuffer newBuffer;
  VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));
  return newBuffer;
}
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

Model::Model(const std::string &modelID, const Material &material,
             const std::string &modelPath, const glm::vec3 &objPos)
    : ID(modelID), material(material), objPosition(objPos),
      modelPath(modelPath) {
  instances.push_back(this);
}

void Model::populateModels() {
  for (Model *model : getInstances()) {

    std::vector<Agnosia_T::Vertex> vertices;
    // Index buffer definition, showing which points to reuse.
    std::vector<uint32_t> indices;
    tinyobj::ObjReaderConfig readerConfig;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(model->modelPath, readerConfig)) {
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

    Agnosia_T::GPUMeshBuffers newSurface;

    // Create a Vertex Buffer here, infinitely easier than the old Vulkan method!
    newSurface.vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    // Find the address of the vertex buffer!
    VkBufferDeviceAddressInfo vertexDeviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = newSurface.vertexBuffer.buffer,
    };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(DeviceControl::getDevice(), &vertexDeviceAddressInfo);

    // Create the index buffer to iterate over and check for duplicate vertices
    newSurface.indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    // Find the address of the vertex buffer!
    VkBufferDeviceAddressInfo indexDeviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = newSurface.indexBuffer.buffer,
    };
    newSurface.indexBufferAddress = vkGetBufferDeviceAddress(DeviceControl::getDevice(), &indexDeviceAddressInfo);

    Agnosia_T::AllocatedBuffer stagingBuffer = createBuffer(
        vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data = stagingBuffer.allocation->GetMappedData();

    // Copy the vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // Copy the index buffer
    memcpy((char *)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd) {
      VkBufferCopy vertexCopy{0};
      vertexCopy.dstOffset = 0;
      vertexCopy.srcOffset = 0;
      vertexCopy.size = vertexBufferSize;

      vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.vertexBuffer.buffer,
                      1, &vertexCopy);

      VkBufferCopy indexCopy{0};
      indexCopy.dstOffset = 0;
      indexCopy.srcOffset = vertexBufferSize;
      indexCopy.size = indexBufferSize;

      vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.indexBuffer.buffer,
                      1, &indexCopy);
    });
    vmaDestroyBuffer(_allocator, stagingBuffer.buffer,
                     stagingBuffer.allocation);

    model->buffers = newSurface;
    model->verticeCount = vertices.size();
    model->indiceCount = indices.size();
  }
}

// TODO: Modify this and the material definition to allow for a fetch into all active textures
void Model::destroyTextures() {
  for (Model *model : Model::getInstances()) {
    vkDestroySampler(DeviceControl::getDevice(),
                     model->getMaterial().getDiffuseTexture().sampler, nullptr);
    vkDestroyImageView(DeviceControl::getDevice(),
                       model->getMaterial().getDiffuseTexture().imageView, nullptr);
    vkDestroyImage(DeviceControl::getDevice(),
                   model->getMaterial().getDiffuseTexture().image, nullptr);
  }
}
void Model::destroyModel(const std::string &modelID) {
  auto iterator = std::find_if(instances.begin(), instances.end(),
                               [&modelID](Model* model) { return model->ID == modelID; }
                              );
  if(iterator != instances.end()) {
    // Remove model from the instances array!
    instances.erase(iterator);
  }
}
std::string Model::getID() { return this->ID; }
glm::vec3 &Model::getPos() { return this->objPosition; }
Material &Model::getMaterial() { return this->material; }
Agnosia_T::GPUMeshBuffers Model::getBuffers() { return this->buffers; }
uint32_t Model::getIndices() { return this->indiceCount; }
uint32_t Model::getVertices() { return this->verticeCount; }
const std::vector<Model *> &Model::getInstances() { return instances; }
