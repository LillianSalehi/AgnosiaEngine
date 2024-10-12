
#include "graphicspipeline.h"
#include "buffers.h"
#include <vector>
namespace Graphics {
  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };


  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  DeviceControl::devicelibrary deviceLibs;
  Buffers::bufferslibrary buffers;
  
  std::vector<VkFramebuffer> swapChainFramebuffers;

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

  void graphicspipeline::destroyGraphicsPipeline() {
    vkDestroyPipeline(Global::device, graphicsPipeline, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed Graphics Pipeline safely\n" << std::endl; 
    vkDestroyPipelineLayout(Global::device, pipelineLayout, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed Layout Pipeline safely\n" << std::endl;
    
  }

  void graphicspipeline::createGraphicsPipeline() {
    // Note to self, for some reason the working directory is not where a read file is called from, but the project folder!
    auto vertShaderCode = readFile("src/shaders/vertex.spv");
    auto fragShaderCode = readFile("src/shaders/fragment.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, Global::device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, Global::device);

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

    auto bindingDescription = Global::Vertex::getBindingDescription();
    auto attributeDescriptions = Global::Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

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
    rasterizer.lineWidth = 2.0f;
      // How to cull the faces, right here we cull the back faces and tell the rasterizer front facing vertices are ordered clockwise.
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &Global::descriptorSetLayout;

    if (vkCreatePipelineLayout(Global::device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }
    // Here we combine all of the structures we created to make the final pipeline!
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(Global::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }
    vkDestroyShaderModule(Global::device, fragShaderModule, nullptr);
    vkDestroyShaderModule(Global::device, vertShaderModule, nullptr); 
    
    if(Global::enableValidationLayers) std::cout << "Pipeline Layout created successfully\n" << std::endl;
  }
  void graphicspipeline::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = deviceLibs.getImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
  
    if (vkCreateRenderPass(Global::device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
    }
    if(Global::enableValidationLayers) std::cout << "Render pass created successfully\n" << std::endl;
  }
  void graphicspipeline::destroyRenderPass() {
    vkDestroyRenderPass(Global::device, renderPass, nullptr);
    if(Global::enableValidationLayers) std::cout << "Destroyed Render Pass Safely\n" << std::endl;
  }
  void graphicspipeline::createFramebuffers() {
    // Resize the container to hold all the framebuffers.
    int framebuffersSize = deviceLibs.getSwapChainImageViews().size();
    swapChainFramebuffers.resize(framebuffersSize);

    for(size_t i = 0; i < framebuffersSize; i++) {
      VkImageView attachments[] = {
        deviceLibs.getSwapChainImageViews()[i]
      };
    
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = deviceLibs.getSwapChainExtent().width;
      framebufferInfo.height = deviceLibs.getSwapChainExtent().height;
      framebufferInfo.layers = 1;

      if(vkCreateFramebuffer(Global::device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer!");
      }
    }
  }
  void graphicspipeline::createCommandPool() {
    // Commands in Vulkan are not executed using function calls, you have to record the ops you wish to perform 
    // to command buffers, pools manage the memory used by the buffer!
    Global::QueueFamilyIndices queueFamilyIndices = Global::findQueueFamilies(Global::physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    
    if(vkCreateCommandPool(Global::device, &poolInfo, nullptr, &Global::commandPool) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create command pool!");
    }
    if(Global::enableValidationLayers) std::cout << "Command pool created successfully\n" << std::endl;
  }
  void graphicspipeline::destroyCommandPool() {
    vkDestroyCommandPool(Global::device, Global::commandPool, nullptr);
  }
  void graphicspipeline::createCommandBuffer() {
    Global::commandBuffers.resize(Global::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = Global::commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) Global::commandBuffers.size();

    if(vkAllocateCommandBuffers(Global::device, &allocInfo, Global::commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffers");
    }
    if(Global::enableValidationLayers) std::cout << "Allocated command buffers successfully\n" << std::endl;
  }

  void graphicspipeline::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = deviceLibs.getSwapChainExtent();

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) deviceLibs.getSwapChainExtent().width;
    viewport.height = (float) deviceLibs.getSwapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = deviceLibs.getSwapChainExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor); 

    VkBuffer vertexBuffers[] = {buffers.getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, buffers.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &Global::descriptorSets[Global::currentFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(buffers.getIndices().size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
  std::vector<VkFramebuffer> graphicspipeline::getSwapChainFramebuffers() {
    return swapChainFramebuffers;
  }  
}
