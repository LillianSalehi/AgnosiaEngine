
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <string>

class Material {
protected:
  std::string ID;
  std::string texturePath;

  VkImage textureImage;
  VkImageView textureImageView;
  VkSampler textureSampler;

public:
  Material(const std::string &matID, const std::string &texPath);
  std::string getID() const;
  std::string getTexturePath() const;
  VkImage &getTextureImage();
  VkImageView &getTextureView();
  VkSampler &getTextureSampler();

  void setTextureImage(VkImage image);
  void setTextureView(VkImageView imageView);
  void setTextureSampler(VkSampler sampler);
};
