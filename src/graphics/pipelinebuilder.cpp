#include "pipelinebuilder.h"
#include <cstdio>
#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "buffers.h"
#include "../devicelibrary.h"
#include "../utils/helpers.h"
#include "../utils/deletion.h"
#include "shader.h"
#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_GLSL
#include <stb/stb_include.h>

Shader LoadShaderWithIncludes(VkShaderStageFlagBits stage, const std::filesystem::path& path)
{
  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
    printf("%s", path.c_str());
    throw std::runtime_error("Path does not refer to a file: " + path.string());
  }

  char error[256]{};
  auto processedSource = std::unique_ptr<char, decltype([](char* p) { free(p); })>(stb_include_file(path.string().data(), nullptr, path.parent_path().string().data(), error));
  if (!processedSource) {
    throw std::runtime_error("Failed to process includes");
  }
  return Shader(stage, std::string_view(processedSource.get()), path.filename().string().c_str());
}

PipelineBuilder::PipelineBuilder() : vertexShader("src/shaders/base.vert"),
                                     fragmentShader("src/shaders/base.frag"),
                                     iaTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
                                     iaPrimitiveRestartEnable(VK_FALSE),
                                     rDepthBiasClamp(0.0f),
                                     rDepthClampEnable(VK_FALSE),
                                     rRasterizerDiscardEnable(VK_FALSE),
                                     rPolygonMode(VK_POLYGON_MODE_FILL),
                                     rCullMode(VK_CULL_MODE_FRONT_BIT),
                                     rFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE),
                                     rDepthBiasEnable(VK_FALSE),
                                     cbBlendEnable(VK_FALSE),
                                     cbColorWriteMask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT),
                                     cbLogicOpEnable(VK_FALSE),
                                     cbLogicOp(VK_LOGIC_OP_COPY),
                                     dsDepthTestEnable(VK_TRUE),
                                     dsDepthWriteEnable(VK_TRUE),
                                     dsDepthCompareOp(VK_COMPARE_OP_LESS),
                                     dsDepthBoundsTestEnable(VK_FALSE),
                                     dsStencilTestEnable(VK_FALSE)
                                     {}
                        
  PipelineBuilder& PipelineBuilder::setVertexShader(const std::string& vertexShader) {
    this->vertexShader = vertexShader;
    return *this;
  }
    
  PipelineBuilder& PipelineBuilder::setFragmentShader(const std::string& fragmentShader) {
    this->fragmentShader = fragmentShader;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setTopology(VkPrimitiveTopology topology) {
    this->iaTopology = topology;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setPrimitiveRestart(VkBool32 primitiveRestart) {
    this->iaPrimitiveRestartEnable = primitiveRestart;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthClamp(VkBool32 depthClamp) {
    this->rDepthClampEnable = depthClamp;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDiscard(VkBool32 discard) {
    this->rRasterizerDiscardEnable= discard;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode polygonMode) {
    this->rPolygonMode = polygonMode;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode) {
    this->rCullMode = cullMode;
    return *this;
  }
    
  PipelineBuilder& PipelineBuilder::setFrontFace(VkFrontFace frontFace) {
    this->rFrontFace = frontFace;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthBias(VkBool32 depthBias) {
    this->rDepthBiasEnable = depthBias;
    return *this;
  }
    
  PipelineBuilder& PipelineBuilder::setDepthBiasConstantFactor(float constFactor) {
    this->rDepthBiasConstantFactor = constFactor;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthBiasClamp(float depthBiasClamp) {
    this->rDepthBiasClamp = depthBiasClamp;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthBiasSlopeFactor(float slopeFactor) {
    this->rDepthBiasSlopeFactor = slopeFactor;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setBlend(VkBool32 enableBlending) {
    this->cbBlendEnable = enableBlending;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setColorWriteMask(VkColorComponentFlags colorWriteMask) {
    this->cbColorWriteMask = colorWriteMask;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setLogicOp(VkBool32 logicOpEnable) {
    this->cbLogicOpEnable = logicOpEnable;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setLogicOpFlags(VkLogicOp logicOp) {
    this->cbLogicOp = logicOp;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthTest(VkBool32 depthTestEnable) {
    this->dsDepthTestEnable = depthTestEnable;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthWrite(VkBool32 depthWriteEnable) {
    this->dsDepthWriteEnable = depthWriteEnable;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthCompareOp(VkCompareOp depthCompareOp) {
    this->dsDepthCompareOp = depthCompareOp;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setDepthBoundsTest(VkBool32 depthBoundsTest) {
    this->dsDepthBoundsTestEnable = depthBoundsTest;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setStencilTest(VkBool32 stencilTest) {
    this->dsStencilTestEnable = stencilTest;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setFrontStencilState(VkStencilOpState frontState) {
    this->dsFront = frontState;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setBackStencilState(VkStencilOpState backState) {
    this->dsBack = backState;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setMinDepthBounds(float minDepth) {
    this->dsMinDepthBounds = minDepth;
    return *this;
  }
  PipelineBuilder& PipelineBuilder::setMaxDepthBounds(float maxDepth) {
    this->dsMaxDepthBounds = maxDepth;
    return *this;
  }
    
  Agnosia_T::Pipeline PipelineBuilder::Build() {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    
    const std::vector<VkDynamicState> DYNAMICSTATES = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};
    
    Shader vertexShader = LoadShaderWithIncludes(VK_SHADER_STAGE_VERTEX_BIT, this->vertexShader);
    Shader fragmentShader = LoadShaderWithIncludes(VK_SHADER_STAGE_FRAGMENT_BIT, this->fragmentShader);
      
    VkShaderModule vertShaderModule = vertexShader.GetShaderModule();
    VkShaderModule fragShaderModule = fragmentShader.GetShaderModule();
      
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .topology = this->iaTopology,
      .primitiveRestartEnable = this->iaPrimitiveRestartEnable
    };
    VkPipelineShaderStageCreateInfo vertShaderInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule,
      .pName = "main"
    };
    VkPipelineVertexInputStateCreateInfo vertexInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    VkPipelineRasterizationStateCreateInfo rasterizer {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .depthClampEnable = this->rDepthClampEnable,
      .rasterizerDiscardEnable = this->rRasterizerDiscardEnable,
      .polygonMode = this->rPolygonMode,
      .cullMode = this->rCullMode,
      .frontFace = this->rFrontFace,
      .depthBiasEnable = this->rDepthBiasEnable,
      .depthBiasConstantFactor = this->rDepthBiasConstantFactor,
      .depthBiasClamp = this->rDepthBiasClamp,
      .depthBiasSlopeFactor = this->rDepthBiasSlopeFactor,       
    };       
    VkPipelineShaderStageCreateInfo fragShaderInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule,
      .pName = "main"
    };
     
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderInfo,fragShaderInfo};

    VkPipelineColorBlendAttachmentState colorBlendAttachment {
      .blendEnable = this->cbBlendEnable,
      .colorWriteMask = this->cbColorWriteMask
    };
    VkPipelineColorBlendStateCreateInfo colorBlending {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .logicOpEnable = this->cbLogicOpEnable,
      .logicOp = this->cbLogicOp,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment
    };
    VkPipelineViewportStateCreateInfo viewportState {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .viewportCount = 1,
      .scissorCount = 1
    };
    VkPipelineMultisampleStateCreateInfo multisampling {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .rasterizationSamples = DeviceControl::getPerPixelSampleCount(),
      .sampleShadingEnable = VK_TRUE
    };
    VkPipelineDepthStencilStateCreateInfo depthStencil {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .depthTestEnable = this->dsDepthTestEnable,
      .depthWriteEnable = this->dsDepthWriteEnable,
      .depthCompareOp = this->dsDepthCompareOp,
      .depthBoundsTestEnable = this->dsDepthBoundsTestEnable,
      .stencilTestEnable = this->dsStencilTestEnable,
      .front = this->dsFront,
      .back = this->dsBack
    };
      
    VkPipelineDynamicStateCreateInfo dynamicState {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(DYNAMICSTATES.size()),
      .pDynamicStates = DYNAMICSTATES.data()
    };
      
    VkPushConstantRange pushConstant{
      .stageFlags = VK_SHADER_STAGE_ALL,
      .offset = 0,
      .size = sizeof(Agnosia_T::GPUPushConstants),
    };
    std::vector<VkDescriptorSetLayout> setLayouts = { Buffers::getTextureDescriptorSetLayouts(), Buffers::getSamplerDescriptorSetLayout() };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .setLayoutCount = 2,
      .pSetLayouts = setLayouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstant
    };
    VK_CHECK(vkCreatePipelineLayout(DeviceControl::getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));
      
    VkPipelineRenderingCreateInfo pipelineRenderingInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &DeviceControl::getImageFormat(),
      .depthAttachmentFormat = DeviceControl::getDepthFormat()
    };
    VkGraphicsPipelineCreateInfo pipelineInfo {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipelineRenderingInfo,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = pipelineLayout,
      .renderPass = nullptr,
      .subpass = 0,
    };

    VK_CHECK(vkCreateGraphicsPipelines(DeviceControl::getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    DeletionQueue::get().push_function([=](){vkDestroyPipeline(DeviceControl::getDevice(), pipeline, nullptr);});
    DeletionQueue::get().push_function([=](){vkDestroyPipelineLayout(DeviceControl::getDevice(), pipelineLayout, nullptr);});

    Agnosia_T::Pipeline finalPipeline = {pipeline, pipelineLayout};
      
    return finalPipeline;
  }


