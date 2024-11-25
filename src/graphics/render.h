#pragma once

#include <cstdint>
class Render {
public:
  static void drawFrame();
  static void createSyncObject();
  static void destroyFenceSemaphores();
  static void cleanupSwapChain();
  static float getFloatBar();
  static uint32_t getCurrentFrame();
};
