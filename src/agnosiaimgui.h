#pragma once
#include "assetcache.h"
#include "volk.h"

class Gui {
public:
  static void drawImGui(AssetCache& cache);
  static void initImgui(VkInstance instance);
  static bool getWireframe();
  static float getLineWidth();
};
