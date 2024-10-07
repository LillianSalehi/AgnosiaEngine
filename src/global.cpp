#include "global.h"
namespace Global {

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };
  #ifdef DEBUG
    const bool enableValidationLayers = true;
  #else 
    const bool enableValidationLayers = false;
  #endif
}
