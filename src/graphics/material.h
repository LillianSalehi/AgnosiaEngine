#pragma once

#include "texture.h"
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <string>

class Material {
protected:
  std::string ID;
  Texture* diffuseTexture;
  Texture* metallicTexture;
  Texture* roughnessTexture;
  Texture* ambientOcclusionTexture;

public:
  Material(const std::string &matID, Texture* diffuseTexture, Texture* metallicTexture, Texture* roughnessTexture, Texture* ambientOcclusionTexture);
  
  std::string getID() const;
  
  Texture* getDiffuseTexture();
  Texture* getMetallicTexture();
  Texture* getRoughnessTexture();
  Texture* getAOTexture();
  
};
