#pragma once

#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/texture.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#define VK_NO_PROTOTYPES
#include "volk.h"

class AssetCache {
  private:
    std::unordered_map<std::string, Texture> textureRegistry;
    std::unordered_map<std::string, std::unique_ptr<Material>> materialRegistry;
    std::unordered_map<std::string, std::unique_ptr<Model>> modelRegistry;
    
  public:
    Texture* fetchLoadTexture(const std::string& ID, const std::string& path);
    Material* findMaterial(const std::string& ID);
    Model* findModel(const std::string& ID);

    void store(std::unique_ptr<Material>&& material);
    void store(std::unique_ptr<Model>&& model);

    void remove(const std::string& ID);

    std::vector<Model*> getModels();
};
