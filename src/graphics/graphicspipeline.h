#pragma once
#define VK_NO_PROTOTYPES
#include "volk.h"

class Graphics {
public:
  static void createGraphicsPipeline();
  static void destroyGraphicsPipeline();
  static void createFramebuffers();
  static void destroyFramebuffers();
  static void createCommandPool();
  static void destroyCommandPool();
  static void createCommandBuffer();
  static void recordCommandBuffer(VkCommandBuffer cmndBuffer,
                                  uint32_t imageIndex);

  static float *getCamPos();
  static float *getCenterPos();
  static float *getUpDir();
  static float &getDepthField();
  static float *getDistanceField();
};
