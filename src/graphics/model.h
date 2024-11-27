#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
class Model {
public:
  static void loadModel(glm::vec3 position);
};
