#include "graphicspipeline.h"
#include <fstream>
namespace Graphics {
  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineLayout pipelineLayout;

  static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }
  
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }
  VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice& device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
  }

  void graphicspipeline::destroyGraphicsPipeline(VkDevice& device) {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed Layout Pipeline safely\n" << std::endl;
  }

  void graphicspipeline::createGraphicsPipeline(VkDevice& device) {
    // Note to self, for some reason the working directory is not where a read file is called from, but the project folder!
    auto vertShaderCode = readFile("src/shaders/vert.spv");
    auto fragShaderCode = readFile("src/shaders/frag.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

    // ------------------ STAGE 1 - INPUT ASSEMBLER ---------------- //
    // This can get a little complicated, normally, vertices are loaded in sequential order, with an element buffer however, you can specify the indices yourself!
    // Using an element buffer means you can reuse vertices, which can lead to optimizations. If you set PrimRestart to TRUE, you can utilize the _STRIP modes with special indices
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;   

    // ------------------ STAGE 2 - VERTEX SHADER ------------------ //
    // this will be revisited, right now we are hardcoding shader data, so we tell it to not load anything, but that will change.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

    // ------------------- STAGE 5 - RASTERIZATION ----------------- //
    // Take Vertex shader vertices and fragmentize them for the frament shader. The rasterizer also can perform depth testing, face culling, and scissor testing.
    // In addition, it can also be configured for wireframe rendering.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Render regardless of the near and far planes, useful for shadow maps, requires GPU feature *depthClamp*
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // MODE_FILL, fill polygons, MODE_LINE, draw wireframe, MODE_POINT, draw vertices. Anything other than fill requires GPU feature *fillModeNonSolid*
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
      // How to cull the faces, right here we cull the back faces and tell the rasterizer front facing vertices are ordered clockwise.
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Whether or not to add depth values. e.x. for shadow maps.
    rasterizer.depthBiasEnable = VK_FALSE;

    // ------------------ STAGE 6 - FRAGMENT SHADER ---------------- //
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};   
    
    // ------------------ STAGE 7 - COLOR BLENDING ----------------- //
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // ---------------------- STATE CONTROLS ----------------------  //
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    // Again, this will be revisited, multisampling can be very useful for anti-aliasing, since it is fast, but we won't implement it for now.
    // Requires GPU feature UNKNOWN eanbled.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // Most of the graphics pipeline is set in stone, some of the pipeline state can be modified without recreating it at runtime though!
    // There are TONS of settings, this would be another TODO to see what else we can mess with dynamically, but right now we just allow dynamic size of the viewport 
    // and dynamic scissor states. Scissors are pretty straightforward, they are basically pixel masks for the rasterizer.
    // Scissors describe what regions pixels will be stored, it doesnt cut them after being rendered, it stops them from ever being rendered in that area in the first place.
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr); 
    
    if(Global::enableValidationLayers) std::cout << "Pipeline Layout created successfully\n" << std::endl;
  }
}
