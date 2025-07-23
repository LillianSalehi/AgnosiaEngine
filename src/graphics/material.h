#pragma once

#include "texture.h"
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <string>

class Material {
protected:
  std::string ID;
  Texture* diffuseTexture;

  float ambient;
  float specular;
  float shine;

public:
  Material(const std::string &matID, Texture* texture,
           const float ambient, const float spec, const float shine);
  
  std::string getID() const;
  
  Texture* getDiffuseTexture();

  float getAmbient();
  float getSpecular();
  float getShininess();
};
