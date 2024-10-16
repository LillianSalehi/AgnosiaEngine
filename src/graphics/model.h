#include "../global.h"

#define TINY_OBJ_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace modellib {
  class Model {
    public:
      static void loadModel();
  };
}
