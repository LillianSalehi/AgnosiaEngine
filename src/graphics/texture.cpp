#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "texture.h"
#include "buffers.h"
#include "../devicelibrary.h"

BuffersLibraries::buffers buffer;
DeviceControl::devicelibrary deviceLibraries;
VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkPipelineStageFlags sourceStage;
VkPipelineStageFlags destinationStage;


namespace TextureLibraries {
  void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    // This function specifies all the data in an image object, this is called directly after the creation of an image object.
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Global::device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(Global::device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = buffer.findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(Global::device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(Global::device, image, imageMemory, 0);
  }

  VkCommandBuffer beginSingleTimeCommands() {
    // This is a neat function! This sets up a command buffer using our previously set command pool 
    // to return a command buffer to execute commands!
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Global::commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Global::device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
  }
  void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // This function takes a command buffer with the data we wish to execute and submits it to the graphics queue.
    // Afterwards, it purges the command buffer.
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(Global::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(Global::graphicsQueue);

    vkFreeCommandBuffers(Global::device, Global::commandPool, 1, &commandBuffer);
  }
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Copy 1 buffer to another.
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
  }
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // This function handles transitioning image layout data from one layout to another.
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
  }
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    //This handles copying from the buffer to the image, specifically what *parts* to copy to the image.
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
      width,
      height,
      1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
  }
// -------------------------------- Image Libraries ------------------------------- //
  void texture::createTextureImage() {
    // Import pixels from image with data on color channels, width and height, and colorspace!
    // Its a lot of kind of complicated memory calls to bring it from a file -> to a buffer -> to a image object.
    int textureWidth, textureHeight, textureChannels;
    stbi_uc* pixels = stbi_load("assets/textures/test.png", &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
    
    VkDeviceSize imageSize = textureWidth * textureHeight * 4;

    if(!pixels) {
      throw std::runtime_error("Failed to load texture!");
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    buffer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(Global::device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(Global::device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(Global::device, stagingBuffer, nullptr);
    vkFreeMemory(Global::device, stagingBufferMemory, nullptr);
  }
  void texture::createTextureImageView() {
    // Create a texture image view, which is a struct of information about the image.
    Global::textureImageView = deviceLibraries.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
  }
  void texture::createTextureSampler() {
    // Create a sampler to access and parse the texture object.
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // These two options define the filtering method when sampling the texture. 
    // It also handles zooming in versus out, min vs mag!
    samplerInfo.magFilter = VK_FILTER_LINEAR; // TODO: CUBIC
    samplerInfo.minFilter = VK_FILTER_LINEAR; // TODO: CUBIC
    
    // These options define UVW edge modes, ClampToEdge extends the last pixels to the edges when larger than the UVW.
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(Global::physicalDevice, &properties);
    // Enable or Disable Anisotropy, and set the amount.
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    
    // When sampling with Clamp to Border, the border color is defined here.
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Normalizing coordinates changes texCoords from [0, texWidth] to [0, 1].
    // This is what should ALWAYS be used, because it means you can use varying texture sizes.
    // Another TODO: Normalizing
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Compare texels to a value and use the output in filtering!
    // This is mainly used in percentage-closer filtering on shadow maps, this will be revisted eventually...
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    
    // Mipmaps are basically LoD's for textures, different resolutions to load based on distance. 
    // These settings simply describe how to apply mipmapping.
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(Global::device, &samplerInfo, nullptr, &Global::textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }  
  }
  void texture::destroyTextureSampler() {
    vkDestroySampler(Global::device, Global::textureSampler, nullptr);
    vkDestroyImageView(Global::device, Global::textureImageView, nullptr);
  }
  void texture::destroyTextureImage() {
    vkDestroyImage(Global::device, textureImage, nullptr);
    vkFreeMemory(Global::device, textureImageMemory, nullptr);
  }
  
}
