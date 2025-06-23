#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "../types.h"
#include "material.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Model {
protected:
  std::string ID;
  Agnosia_T::GPUMeshBuffers buffers;
  Material material;
  glm::vec3 objPosition;
  uint32_t indiceCount;
  std::string modelPath;
  static std::vector<Model *> instances;

public:
  Model(const std::string &modelID, const Material &material,
        const std::string &modelPath, const glm::vec3 &opjPos);

  static void createMemoryAllocator(VkInstance instance);
  static const std::vector<Model *> &getInstances();

  static void populateModels();
  static void destroyTextures();
  static void destroyModel(const std::string &modelID);

  Agnosia_T::GPUMeshBuffers getBuffers();
  std::string getID();
  glm::vec3 &getPos();
  Material &getMaterial();
  std::string getModelPath();
  uint32_t getIndices();
};
