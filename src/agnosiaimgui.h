#pragma once
#include "assetcache.h"
#define VK_NO_PROTOTYPES
#include "volk.h"

class Gui {
public:
  static void drawImGui(AssetCache& cache);
  static void initImgui(VkInstance instance);
  static bool getWireframe();
  static float getLineWidth();
};
