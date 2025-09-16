#pragma once
#include "volk.h"
#include "../utils/types.h"
#include "../assetcache.h"

class Graphics {
public:
  static void createCommandPool();
  static void createCommandBuffer();
  static void recordCommandBuffer(VkCommandBuffer cmndBuffer, uint32_t imageIndex, AssetCache& cache);

  static void addGraphicsPipeline(Agnosia_T::Pipeline pipeline);
  static void addFullscreenPipeline(Agnosia_T::Pipeline pipeline);
  
  static float *getCamPos();
  static float *getLightPos();
  static float *getLightColor();
  static float &getLightPower();
  static float *getCenterPos();
  static float *getUpDir();
  static float &getDepthField();
  static float *getDistanceField();
  
};
