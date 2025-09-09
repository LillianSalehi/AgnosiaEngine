#pragma once

#include <string>
#include <vector>
#include "volk.h"

#include <filesystem>

class Shader {
  
  public:
    // Already-processed source constructor
    explicit Shader(VkShaderStageFlagBits stage, std::string_view source, std::string name);
    // Path constructor with glslang include handling
    explicit Shader(VkShaderStageFlagBits stage, const std::filesystem::path& path);
    Shader(const Shader&) = delete;
    Shader(Shader&& old) noexcept;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&& old) noexcept;
    ~Shader();

    [[nodiscard]] VkShaderModule GetShaderModule() const {
      return shaderModule_;
    }
    [[nodiscard]] VkShaderStageFlagBits GetPipelineStage() const {
      return stage_;
    }
  
  private:
    void Initialize(const std::vector<uint32_t> binarySpv);

    VkShaderStageFlagBits stage_{};
    VkShaderModule shaderModule_;
};
