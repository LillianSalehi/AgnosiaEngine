#pragma once
#include "../global.h"

namespace TextureLibraries {
  class texture {
    public:
      void createTextureImage();
      void createTextureImageView();
      void createTextureSampler();
      void destroyTextureImage();
      void destroyTextureSampler();
  };
}
