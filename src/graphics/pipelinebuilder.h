#pragma once
#define VK_NO_PROTOTYPES
#include "volk.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../devicelibrary.h"
#include "../types.h"
#include "buffers.h"
#include "texture.h"

class PipelineBuilder {
  private:
    std::string vertexShader;
    std::string fragmentShader;
    // Input Assembly //
    VkPrimitiveTopology iaTopology;
    VkBool32 iaPrimitiveRestartEnable;
    // Rasterizer //
    VkBool32 rDepthClampEnable;
    VkBool32 rRasterizerDiscardEnable;
    VkPolygonMode rPolygonMode;
    VkCullModeFlags rCullMode;
    VkFrontFace rFrontFace;
    VkBool32 rDepthBiasEnable;
    float rDepthBiasConstantFactor;
    float rDepthBiasClamp;
    float rDepthBiasSlopeFactor;
    // Color Blending //
    VkBool32 cbBlendEnable;
    VkColorComponentFlags cbColorWriteMask;
    VkBool32 cbLogicOpEnable;
    VkLogicOp cbLogicOp;
    // Depth Stencil //
    VkBool32 dsDepthTestEnable;
    VkBool32 dsDepthWriteEnable;
    VkCompareOp dsDepthCompareOp;
    VkBool32 dsDepthBoundsTestEnable;
    VkBool32 dsStencilTestEnable;
    VkStencilOpState dsFront;
    VkStencilOpState dsBack;
    float dsMinDepthBounds;
    float dsMaxDepthBounds;
  public:
    PipelineBuilder();

    PipelineBuilder& setVertexShader(const std::string& vertexShader);
    PipelineBuilder& setFragmentShader(const std::string& fragmentShader);
    PipelineBuilder& setTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPrimitiveRestart(VkBool32 primitiveRestart);
    PipelineBuilder& setDepthClamp(VkBool32 depthClamp);
    PipelineBuilder& setDiscard(VkBool32 discard);
    PipelineBuilder& setPolygonMode(VkPolygonMode polygonMode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode);
    PipelineBuilder& setFrontFace(VkFrontFace frontFace);
    PipelineBuilder& setDepthBias(VkBool32 depthBias);
    PipelineBuilder& setDepthBiasConstantFactor(float constFactor);
    PipelineBuilder& setDepthBiasClamp(float depthBiasClamp);
    PipelineBuilder& setDepthBiasSlopeFactor(float slopeFactor);
    PipelineBuilder& setBlend(VkBool32 enableBlending);
    PipelineBuilder& setColorWriteMask(VkColorComponentFlags colorWriteMask);
    PipelineBuilder& setLogicOp(VkBool32 logicOpEnable);
    PipelineBuilder& setLogicOpFlags(VkLogicOp logicOp);
    PipelineBuilder& setDepthTest(VkBool32 depthTestEnable);
    PipelineBuilder& setDepthWrite(VkBool32 depthWriteEnable);
    PipelineBuilder& setDepthCompareOp(VkCompareOp depthCompareOp);
    PipelineBuilder& setDepthBoundsTest(VkBool32 depthBoundsTest);
    PipelineBuilder& setStencilTest(VkBool32 stencilTest);
    PipelineBuilder& setFrontStencilState(VkStencilOpState frontState);
    PipelineBuilder& setBackStencilState(VkStencilOpState backState);
    PipelineBuilder& setMinDepthBounds(float minDepth);
    PipelineBuilder& setMaxDepthBounds(float maxDepth);

    Agnosia_T::Pipeline Build();
};
