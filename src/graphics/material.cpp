#include "material.h"

Material::Material(const std::string &matID, const std::string &difPath,
                   const float ambient, const float spec, const float shine)
    : ID(matID), diffusePath(difPath), ambient(ambient), specular(spec),
      shine(shine) {}

std::string Material::getID() const { return ID; }
std::string Material::getDiffusePath() const { return diffusePath; }

VkImage &Material::getDiffuseImage() { return this->diffuseImage; }
VkImageView &Material::getDiffuseImgView() { return this->diffuseImageView; }
VkSampler &Material::getDiffuseSampler() { return this->diffuseSampler; }

float Material::getAmbient() { return this->ambient; }
float Material::getSpecular() { return this->specular; }
float Material::getShininess() { return this->shine; }
void Material::setDiffuseImage(VkImage image) { this->diffuseImage = image; }
void Material::setDiffuseView(VkImageView imageView) {
  this->diffuseImageView = imageView;
}
void Material::setDiffuseSampler(VkSampler sampler) {
  this->diffuseSampler = sampler;
}
