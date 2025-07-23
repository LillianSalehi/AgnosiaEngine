#include "material.h"

Material::Material(const std::string &matID, Texture* diffuseTexture, const float ambient, const float spec, const float shine)
    : ID(matID), ambient(ambient), specular(spec), shine(shine), diffuseTexture(diffuseTexture) {}

std::string Material::getID() const { return ID; }

Texture* Material::getDiffuseTexture() { return this->diffuseTexture; }
float Material::getAmbient() { return this->ambient; }
float Material::getSpecular() { return this->specular; }
float Material::getShininess() { return this->shine; }


