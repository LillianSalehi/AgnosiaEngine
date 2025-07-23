
#include "assetcache.h"

Texture* AssetCache::fetchLoadTexture(const std::string& ID, const std::string& path) {
  auto it = textureRegistry.find(ID);
  if(it != textureRegistry.end()) {
    return &it->second;
  } else {
    textureRegistry.insert_or_assign(ID, Texture(ID, path));
    return &textureRegistry.at(ID);
  }
}
Material* AssetCache::findMaterial(const std::string& ID) {
  auto it = materialRegistry.find(ID);
  return it != materialRegistry.end() ? it->second.get() : nullptr;
}
Model* AssetCache::findModel(const std::string& ID) {
  auto it = modelRegistry.find(ID);
  return it != modelRegistry.end() ? it->second.get() : nullptr;
}

void AssetCache::store(std::unique_ptr<Material>&& material) {
  materialRegistry.insert_or_assign(material->getID(), std::move(material));
}
void AssetCache::store(std::unique_ptr<Model>&& model) {
  modelRegistry.insert_or_assign(model->getID(), std::move(model));
}
void AssetCache::remove(const std::string& ID) {
  textureRegistry.erase(ID);
  materialRegistry.erase(ID);
  modelRegistry.erase(ID);
}

std::vector<Model*> AssetCache::getModels() {
  std::vector<Model*> models;
  for(auto& it : modelRegistry) {
    models.push_back(it.second.get());
  }
  return models;
}

