#pragma once
#include "volk.h"

class Gui {
public:
  static void drawImGui();
  static void initImgui(VkInstance instance);
  static bool getWireframe();
  static float getLineWidth();
};
