#include "shader.h"
#include <cassert>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/Logger.h>
#include <iostream>
#include <fstream>
#include "../utils/helpers.h"
#include "../devicelibrary.h"

constexpr EShLanguage VkShaderStageToGlslang(VkShaderStageFlagBits stage) {
  switch (stage) {
    case VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT: return EShLanguage::EShLangVertex;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT: return EShLanguage::EShLangFragment;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT: return EShLanguage::EShLangCompute;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR: return EShLanguage::EShLangRayGen;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR: return EShLanguage::EShLangMiss;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return EShLanguage::EShLangClosestHit;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return EShLanguage::EShLangAnyHit;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return EShLanguage::EShLangIntersect;
  }
  return static_cast<EShLanguage>(-1);
}
size_t NumberOfPathComponents(std::filesystem::path path) {
  size_t parents = 0;
  while (!path.empty()) {
    parents++;
    path = path.parent_path();
  }
  return parents > 0 ? parents - 1 : 0; // The path will contain a filename, which we will ignore
}

std::string LoadFile(const std::filesystem::path& path) {
  std::ifstream file{path};
  if (!file) {
    throw std::runtime_error("File not found");
  }
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

class IncludeHandler final : public glslang::TShader::Includer {
  public:
    IncludeHandler(const std::filesystem::path& sourcePath) {
      // Seed the "stack" with just the parent directory of the top-level source
      currentIncluderDir_ /= sourcePath.parent_path();
    }

    glslang::TShader::Includer::IncludeResult* includeLocal(
      const char* requested_source,
      [[maybe_unused]] const char* requesting_source,
      [[maybe_unused]] size_t include_depth) override {
      // Everything will explode if this is not relative
      assert(std::filesystem::path(requested_source).is_relative());
      auto fullRequestedSource = currentIncluderDir_ / requested_source;
      currentIncluderDir_ = fullRequestedSource.parent_path();

      auto contentPtr = std::make_unique<std::string>(LoadFile(fullRequestedSource));
      auto content = contentPtr.get();
      auto sourcePathPtr = std::make_unique<std::string>(requested_source);
      //auto sourcePath = sourcePathPtr.get();

      contentStrings_.emplace_back(std::move(contentPtr));
      sourcePathStrings_.emplace_back(std::move(sourcePathPtr));

      return new glslang::TShader::Includer::IncludeResult(requested_source, content->c_str(), content->size(), nullptr);
    }

    void releaseInclude(glslang::TShader::Includer::IncludeResult* data) override {
      for (size_t i = 0; i < NumberOfPathComponents(data->headerName); i++) {
        currentIncluderDir_ = currentIncluderDir_.parent_path();
      }  
      delete data;
    }

  private:
    // Acts like a stack that we "push" path components to when include{Local, System} are invoked, and "pop" when releaseInclude is invoked
    std::filesystem::path currentIncluderDir_;
    std::vector<std::unique_ptr<std::string>> contentStrings_;
    std::vector<std::unique_ptr<std::string>> sourcePathStrings_;
};

void Shader::Initialize(const std::vector<uint32_t> binarySpv) {
  glslang::InitializeProcess();
  
  VK_CHECK(vkCreateShaderModule(DeviceControl::getDevice(),
                                Address(VkShaderModuleCreateInfo{
                                  .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                  .pNext = nullptr,
                                  .codeSize = binarySpv.size() * sizeof(uint32_t),
                                  .pCode = binarySpv.data()}),
                                nullptr,
                                &shaderModule_
                                ));
}

std::vector<uint32_t> CompileShaderToSpirv(VkShaderStageFlagBits stageFlag, std::string_view source, glslang::TShader::Includer* includer) {
  const EShLanguage stage = VkShaderStageToGlslang(stageFlag);
  glslang::TShader shader(stage);

  int length = static_cast<int>(source.size());
  const char* data = source.data();
  
  shader.setStringsWithLengths(&data, &length, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
  shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_6);

  std::string preamble = "#extension GL_GOOGLE_include_directive : enable\n";
  shader.setPreamble(preamble.c_str());
  shader.setOverrideVersion(460);
  const EShMessages compilerMessages = EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo | EShMsgEnhanced | EShMsgAbsolutePath | EShMsgDisplayErrorColumn);
  
  bool parseResult;
  if (includer) {
    parseResult = shader.parse(GetDefaultResources(), 460, EProfile::ECoreProfile, false, false, compilerMessages, *includer);
  } else {
    parseResult = shader.parse(GetDefaultResources(), 460, EProfile::ECoreProfile, false, false, compilerMessages);
  }

  if (!parseResult) {
    printf("Info log: %s\nDebug log: %s\n", shader.getInfoLog(), shader.getInfoDebugLog());
    // TODO: throw shader compile error
    throw std::runtime_error("Shader compilation failed");
  }
  
  glslang::TProgram program;
  program.addShader(&shader);
  program.link(EShMsgDefault);
  program.buildReflection();
  
  auto options = glslang::SpvOptions{
    .generateDebugInfo = true,
    .stripDebugInfo = false,
    .disableOptimizer = true,
  };
  
  std::vector<uint32_t> spirv;
  spv::SpvBuildLogger logger;  
  glslang::GlslangToSpv(*shader.getIntermediate(), spirv, &logger, &options);
  
  auto loggerMessages = logger.getAllMessages();
  if (!loggerMessages.empty()) {
    printf("spv logger messages: %s", loggerMessages.c_str());
  }
  
  return spirv;
}
Shader::Shader(VkShaderStageFlagBits stage, std::string_view source, std::string name)
: stage_(stage) {
  Initialize(CompileShaderToSpirv(stage, source, nullptr));
}
  
Shader::Shader(VkShaderStageFlagBits stage, const std::filesystem::path& path)
: stage_(stage) {
  Initialize(CompileShaderToSpirv(stage, LoadFile(path), Address(IncludeHandler(path))));
}
Shader::Shader(Shader&& old) noexcept
: stage_(old.stage_),
  shaderModule_(std::exchange(old.shaderModule_, VK_NULL_HANDLE)) {}

Shader& Shader::operator=(Shader&& old) noexcept {
  if (&old == this) return *this;
  this->~Shader();
  return *new (this) Shader(std::move(old));
}

Shader::~Shader() {
  if(shaderModule_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(DeviceControl::getDevice(), shaderModule_, nullptr);
    glslang::FinalizeProcess();
  }
}
