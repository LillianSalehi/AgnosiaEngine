#pragma once
#include "global.h"
#include "graphics/texture.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <map>

namespace agnosia_imgui {
class Gui {
public:
  static void drawImGui();
  static void initImgui(VkInstance instance);
};
} // namespace agnosia_imgui
