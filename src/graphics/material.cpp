#include "material.h"

Material::Material(const std::string &matID, const std::string &difPath,
                   const float ambient, const float spec, const float shine)
    : ID(matID), diffusePath(difPath), ambient(ambient), specular(spec),
      shine(shine) {}

std::string Material::getID() const { return ID; }
std::string Material::getDiffusePath() const { return diffusePath; }

Agnosia_T::Texture &Material::getDiffuseTexture() {
  return this->diffuseTexture;
}

float Material::getAmbient() { return this->ambient; }
float Material::getSpecular() { return this->specular; }
float Material::getShininess() { return this->shine; }

void Material::setDiffuseImage(VkImage image) { this->diffuseTexture.image = image; }
void Material::setDiffuseView(VkImageView imageView) {
  this->diffuseTexture.imageView = imageView;
}
void Material::setDiffuseSampler(VkSampler sampler) {
  this->diffuseTexture.sampler = sampler;
}
