#include "buffers.h"
#include <stdexcept>
#include <string>
#define TINY_OBJ_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "model.h"
namespace ModelLib {
  BuffersLibraries::buffers buf;

  void model::loadModel() {
    tinyobj::ObjReaderConfig readerConfig;
    
    tinyobj::ObjReader reader;

    if(!reader.ParseFromFile(Global::MODEL_PATH, readerConfig)) {
      if(!reader.Error().empty()) {
        throw std::runtime_error(reader.Error());
      }
    }
    
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Global::Vertex vertex;

        vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };

        vertex.texCoord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };

        vertex.color = {1.0f, 1.0f, 1.0f};

        Global::vertices.push_back(vertex);
        Global::indices.push_back(Global::indices.size());       
      } 
    }
  }
}
