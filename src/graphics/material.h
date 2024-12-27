
#include <glm/fwd.hpp>
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <string>

class Material {
protected:
  std::string ID;
  std::string diffusePath;

  VkImage diffuseImage;
  VkImageView diffuseImageView;
  VkSampler diffuseSampler;

  float ambient;
  float specular;
  float shine;

public:
  Material(const std::string &matID, const std::string &texPath,
           const float ambient, const float spec, const float shine);
  std::string getID() const;

  std::string getDiffusePath() const;

  VkImage &getDiffuseImage();
  VkImageView &getDiffuseImgView();
  VkSampler &getDiffuseSampler();

  float getAmbient();
  float getSpecular();
  float getShininess();

  void setDiffuseImage(VkImage image);
  void setDiffuseView(VkImageView imageView);
  void setDiffuseSampler(VkSampler sampler);
};
