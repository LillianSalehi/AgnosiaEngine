#include "buffers.h"
#include "model.h"
#include <stdexcept>

#define TINY_OBJ_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
template <> struct hash<Buffers::Vertex> {
  size_t operator()(Buffers::Vertex const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std

const std::string MODEL_PATH = "assets/models/viking_room.obj";

void Model::loadModel() {
  tinyobj::ObjReaderConfig readerConfig;

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(MODEL_PATH, readerConfig)) {
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
  std::unordered_map<Buffers::Vertex, uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Buffers::Vertex vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      // TODO: Small fix here, handle if there are no UV's unwrapped for the
      // model.
      //       As of now, if it is not unwrapped, it segfaults on texCoord
      //       assignment. Obviously we should always have UV's, but it
      //       shouldn't crash, just unwrap in a default method.
      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] =
            static_cast<uint32_t>(Buffers::getVertices().size());
        Buffers::getVertices().push_back(vertex);
      }
      Buffers::getIndices().push_back(uniqueVertices[vertex]);
    }
  }
}
