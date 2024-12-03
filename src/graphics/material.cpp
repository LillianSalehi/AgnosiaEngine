#include "material.h"

Material::Material(const std::string &matID, const std::string &texPath)
    : ID(matID), texturePath(texPath) {}

std::string Material::getID() const { return ID; }
std::string Material::getTexturePath() const { return texturePath; }

VkImage &Material::getTextureImage() { return this->textureImage; }
VkImageView &Material::getTextureView() { return this->textureImageView; }
VkSampler &Material::getTextureSampler() { return this->textureSampler; }

void Material::setTextureImage(VkImage image) { this->textureImage = image; }
void Material::setTextureView(VkImageView imageView) {
  this->textureImageView = imageView;
}
void Material::setTextureSampler(VkSampler sampler) {
  this->textureSampler = sampler;
}
