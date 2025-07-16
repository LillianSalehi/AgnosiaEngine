#include "material.h"

// This is a container for ALL material instances alive
std::vector<Material *> Material::instances;

Material::Material(const std::string &matID, const std::string &difPath, const float ambient, const float spec, const float shine)
    : ID(matID), diffusePath(difPath), ambient(ambient), specular(spec), shine(shine), diffuseTexture(diffusePath) {
  instances.push_back(this);
}

std::string Material::getID() const { return ID; }
std::string Material::getDiffusePath() const { return diffusePath; }

Texture &Material::getDiffuseTexture() { return this->diffuseTexture; }
float Material::getAmbient() { return this->ambient; }
float Material::getSpecular() { return this->specular; }
float Material::getShininess() { return this->shine; }

void Material::destroyMaterial(Material* material) {
  std::erase(instances, material);
  
  //vkDestroySampler(DeviceControl::getDevice(), material->getDiffuseTexture().sampler, nullptr);
  //vkDestroyImageView(DeviceControl::getDevice(), material->getDiffuseTexture().imageView, nullptr);
  //vkDestroyImage(DeviceControl::getDevice(), material->getDiffuseTexture().image, nullptr);
}
const std::vector<Material *> &Material::getInstances() { return instances; }
