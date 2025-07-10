#include "material.h"
#include "../devicelibrary.h"
#include <algorithm>


// This is a container for ALL material instances alive
std::vector<Material *> Material::instances;

Material::Material(const std::string &matID, const std::string &difPath, const float ambient, const float spec, const float shine)
    : ID(matID), diffusePath(difPath), ambient(ambient), specular(spec), shine(shine) {
  instances.push_back(this);
}

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
void Material::destroyMaterial(Material* material) {
  instances.erase(std::remove(instances.begin(), instances.end(), material), instances.end());
  
  vkDestroySampler(DeviceControl::getDevice(), material->getDiffuseTexture().sampler, nullptr);
  vkDestroyImageView(DeviceControl::getDevice(), material->getDiffuseTexture().imageView, nullptr);
  vkDestroyImage(DeviceControl::getDevice(), material->getDiffuseTexture().image, nullptr);

  delete material;
}
void Material::destroyMaterials() {
  for(Material* material : instances) {
    instances.erase(std::remove(instances.begin(), instances.end(), material), instances.end());

    vkDestroySampler(DeviceControl::getDevice(), material->getDiffuseTexture().sampler, nullptr);
    vkDestroyImageView(DeviceControl::getDevice(), material->getDiffuseTexture().imageView, nullptr);
    vkDestroyImage(DeviceControl::getDevice(), material->getDiffuseTexture().image, nullptr);

    delete material;
  }
}
const std::vector<Material *> &Material::getInstances() { return instances; }
