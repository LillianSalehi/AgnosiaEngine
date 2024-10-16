#include "model.h"

namespace std {
  template<> struct hash<Global::Vertex> {
    size_t operator()(Global::Vertex const& vertex) const {
      return ((hash<glm::vec3>()(vertex.pos) ^
              (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
              (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

namespace modellib {

  std::unordered_map<Global::Vertex, uint32_t> uniqueVertices{};

  void Model::loadModel() {
    tinyobj::ObjReaderConfig readerConfig;
    
    tinyobj::ObjReader reader;

    if(!reader.ParseFromFile(Global::MODEL_PATH, readerConfig)) {
      if(!reader.Error().empty()) {
        throw std::runtime_error(reader.Error());
      }
      if(!reader.Warning().empty()) {
        throw std::runtime_error(reader.Warning());
      }
    }
    
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    //auto& materials = reader.GetMaterials();

    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Global::Vertex vertex{};

        vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };
        
        //TODO: Major fix here, running anything but the viking room OBJ crashes on texcoord assignment!
        vertex.texCoord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };
        vertex.color = {1.0f, 1.0f, 1.0f};

        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] = static_cast<uint32_t>(Global::vertices.size());
          Global::vertices.push_back(vertex);
        }
        
        Global::indices.push_back(uniqueVertices[vertex]);
      }
    }
  }
}
