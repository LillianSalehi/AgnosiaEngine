#include "../devicelibrary.h"
#include "buffers.h"
#include "texture.h"
#include "../utils/helpers.h"
#include "../utils/deletion.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

VkDeviceMemory textureImageMemory;
VkPipelineStageFlags sourceStage;
VkPipelineStageFlags destinationStage;

Texture::Image colorImage;
Texture::Image depthImage;

VkCommandBuffer beginSingleTimeCommands() {
  // This is a neat function! This sets up a command buffer using our previously
  // set command pool to return a command buffer to execute commands!
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = Buffers::getCommandPool(),
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(DeviceControl::getDevice(), &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}
void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  // This function takes a command buffer with the data we wish to execute and
  // submits it to the graphics queue. Afterwards, it purges the command buffer.
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {
  .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
  .commandBufferCount = 1,
  .pCommandBuffers = &commandBuffer,
  };
  vkQueueSubmit(DeviceControl::getGraphicsQueue(), 1, &submitInfo,
                VK_NULL_HANDLE);
  vkQueueWaitIdle(DeviceControl::getGraphicsQueue());

  vkFreeCommandBuffers(DeviceControl::getDevice(), Buffers::getCommandPool(), 1, &commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout,
                           uint32_t mipLevels) {
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
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                       uint32_t height) {
  // This handles copying from the buffer to the image, specifically what
  // *parts* to copy to the image.
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
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t textureWidth,
                     int32_t textureHeight, uint32_t mipLevels) {
  // Check if image format supports linear blitting
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(DeviceControl::getPhysicalDevice(), imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  // Specify the parameters of an image memory barrier
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = textureWidth;
  int32_t mipHeight = textureHeight;

  for (uint32_t mip = 1; mip < mipLevels; mip++) {
    barrier.subresourceRange.baseMipLevel = mip - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = mip - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = mip;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }
  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

Texture::Texture(const std::string& ID, const std::string& texturePath) {
  int textureWidth, textureHeight, textureChannels;
  stbi_uc *pixels = stbi_load(texturePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

  this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

  VkDeviceSize imageSize = textureWidth * textureHeight * 4;

  if (!pixels) {
    throw std::runtime_error("Failed to load texture!");
  }
  Agnosia_T::AllocatedBuffer stagingBuffer = Buffers::createBuffer(imageSize,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VMA_MEMORY_USAGE_AUTO);
  
  void *data;
    
  vmaMapMemory(Buffers::getAllocator(), stagingBuffer.allocation, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vmaUnmapMemory(Buffers::getAllocator(), stagingBuffer.allocation);
    
  stbi_image_free(pixels);
  
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = textureWidth;
  imageInfo.extent.height = textureHeight;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.mipLevels = mipLevels;

  VmaAllocationCreateInfo vmaCreateInfo = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };
  VmaAllocation alloc;
  VmaAllocationInfo allocInfo;
  
  vmaCreateImage(Buffers::getAllocator(), &imageInfo, &vmaCreateInfo, &this->image, &alloc, &allocInfo);

  transitionImageLayout(this->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->mipLevels);
  copyBufferToImage(stagingBuffer.buffer, this->image, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

  vmaDestroyBuffer(Buffers::getAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);

  generateMipmaps(this->image, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight, this->mipLevels);
  // Create a texture image view, which is a struct of information about the image.
  this->imageView = DeviceControl::createImageView(this->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

  // Create a sampler to access and parse the texture object.

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  // These two options define the filtering method when sampling the texture.
  // It also handles zooming in versus out, min vs mag!
  samplerInfo.magFilter = VK_FILTER_LINEAR; // TODO: CUBIC
  samplerInfo.minFilter = VK_FILTER_LINEAR; // TODO: CUBIC

  // These options define UVW edge modes, ClampToEdge extends the last pixels
  // to the edges when larger than the UVW.
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(DeviceControl::getPhysicalDevice(), &properties);
  // Enable or Disable Anisotropy, and set the amount.
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

  // When sampling with Clamp to Border, the border color is defined here.
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  // Normalizing coordinates changes texCoords from [0, texWidth] to [0, 1].
  // This is what should ALWAYS be used, because it means you can use varying
  // texture sizes. Another TODO: Normalizing
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  // Compare texels to a value and use the output in filtering!
  // This is mainly used in percentage-closer filtering on shadow maps, this
  // will be revisted eventually...
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  // Mipmaps are basically LoD's for textures, different resolutions to load
  // based on distance. These settings simply describe how to apply
  // mipmapping.
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

  VK_CHECK(vkCreateSampler(DeviceControl::getDevice(), &samplerInfo, nullptr, &this->sampler));
  
  VkImage image = this->image;
  VkImageView imageView = this->imageView;
  VkSampler sampler = this->sampler;
  
  DeletionQueue::get().push_function([=](){vmaDestroyImage(Buffers::getAllocator(), image, alloc);});
  DeletionQueue::get().push_function([=](){vkDestroyImageView(DeviceControl::getDevice(), imageView, nullptr);});
  DeletionQueue::get().push_function([=](){vkDestroySampler(DeviceControl::getDevice(), sampler, nullptr);});
}

void Texture::createColorImage() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = DeviceControl::getSwapChainExtent().width;
  imageInfo.extent.height = DeviceControl::getSwapChainExtent().height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = DeviceControl::getImageFormat();
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  imageInfo.samples = DeviceControl::getPerPixelSampleCount();
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.mipLevels = 1;

  VmaAllocationCreateInfo vmaCreateInfo = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };
  VmaAllocation alloc;
  VmaAllocationInfo allocInfo;
  vmaCreateImage(Buffers::getAllocator(), &imageInfo, &vmaCreateInfo, &colorImage.image, &alloc, &allocInfo);

  colorImage.imageView = DeviceControl::createImageView(colorImage.image, DeviceControl::getImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
  colorImage.alloc = alloc;
}
void Texture::createDepthImage() {

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = DeviceControl::getSwapChainExtent().width;
  imageInfo.extent.height = DeviceControl::getSwapChainExtent().height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = DeviceControl::getDepthFormat();
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = DeviceControl::getPerPixelSampleCount();
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.mipLevels = 1;

  VmaAllocationCreateInfo vmaCreateInfo = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };
  VmaAllocation alloc;
  VmaAllocationInfo allocInfo;
  vmaCreateImage(Buffers::getAllocator(), &imageInfo, &vmaCreateInfo, &depthImage.image, &alloc, &allocInfo);
  
  depthImage.imageView = DeviceControl::createImageView(depthImage.image, DeviceControl::getDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);
  depthImage.alloc = alloc;
}

// ---------------------------- Getters & Setters ---------------------------------//
uint32_t Texture::getMipLevels() { return this->mipLevels; }

Texture::Image &Texture::getColorImage() { return colorImage; }
Texture::Image &Texture::getDepthImage() { return depthImage; }

VkImage &Texture::getImage() { return this->image; }
VkImageView &Texture::getImageView() { return this->imageView; }
VkSampler &Texture::getSampler() { return this->sampler; }
