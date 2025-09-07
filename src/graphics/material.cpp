#include "material.h"

Material::Material(const std::string &matID, Texture* diffuseTexture, Texture* metallicTexture, Texture* roughnessTexture, Texture* ambientOcclusionTexture)
    : ID(matID), diffuseTexture(diffuseTexture), metallicTexture(metallicTexture), roughnessTexture(roughnessTexture), ambientOcclusionTexture(ambientOcclusionTexture) {}

std::string Material::getID() const { return ID; }

Texture* Material::getDiffuseTexture() { return this->diffuseTexture; }
Texture* Material::getMetallicTexture() { return this->metallicTexture; }
Texture* Material::getRoughnessTexture() { return this->roughnessTexture; }
Texture* Material::getAOTexture() { return this->ambientOcclusionTexture; }


