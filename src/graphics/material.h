

#define VK_NO_PROTOTYPES
#include "volk.h"
#include "../types.h"
#include <string>
#include <glm/fwd.hpp>

class Material {
protected:
  std::string ID;
  std::string diffusePath;

  Agnosia_T::Texture diffuseTexture;

  float ambient;
  float specular;
  float shine;

public:
  Material(const std::string &matID, const std::string &texPath,
           const float ambient, const float spec, const float shine);
  std::string getID() const;

  std::string getDiffusePath() const;

  Agnosia_T::Texture &getDiffuseTexture();

  float getAmbient();
  float getSpecular();
  float getShininess();

  void setDiffuseImage(VkImage image);
  void setDiffuseView(VkImageView imageView);
  void setDiffuseSampler(VkSampler sampler);
};
