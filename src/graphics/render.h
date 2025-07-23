#pragma once

#include <cstdint>
#include "../assetcache.h"
class Render {
public:
  static void drawFrame(AssetCache& cache);
  static void createSyncObject();
  static void destroyFenceSemaphores();
  static void cleanupSwapChain();
  static float getFloatBar();
  static uint32_t getCurrentFrame();
};
