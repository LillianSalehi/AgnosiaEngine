#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "../utils/types.h"
#include "material.h"
#include <glm/glm.hpp>
#include <string>

class Model {
protected:
  std::string ID;
  Agnosia_T::GPUMeshBuffers buffers;
  Material material;
  glm::vec3 objPosition;
  uint32_t verticeCount;
  uint32_t indiceCount;
  std::string modelPath;

public:
  Model(const std::string &modelID, const Material &material,
        const std::string &modelPath, const glm::vec3 &opjPos);

  Agnosia_T::GPUMeshBuffers getBuffers();
  std::string getID();
  glm::vec3 &getPos();
  Material &getMaterial();
  std::string getModelPath();
  uint32_t getIndices();
  uint32_t getVertices();
};
