#pragma once
#include "../global.h"

namespace render_present {
class Render {
public:
  static void drawFrame();
  static void createSyncObject();
  static void destroyFenceSemaphores();
  static void cleanupSwapChain();
  static void init_imgui(VkInstance instance);
  static void drawImGui();
};
} // namespace render_present
