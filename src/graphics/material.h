#pragma once

#include "texture.h"
#include <vector>
#define VK_NO_PROTOTYPES
#include "volk.h"
#include "../types.h"
#include <string>

class Material {
protected:
  std::string ID;
  std::string diffusePath;

  Texture diffuseTexture;

  float ambient;
  float specular;
  float shine;

  static std::vector<Material *> instances;

public:
  Material(const std::string &matID, const std::string &texPath,
           const float ambient, const float spec, const float shine);
  
  std::string getID() const;
  
  static const std::vector<Material *> &getInstances();
  
  std::string getDiffusePath() const;

  Texture &getDiffuseTexture();

  float getAmbient();
  float getSpecular();
  float getShininess();

  static void destroyMaterial(Material* material);
  
};
