#include "agnosiaimgui.h"
#include "imgui.h"

namespace agnosia_imgui {

struct {
} ImGuiSettings;

void Gui::drawTabs() {
  if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_Reorderable)) {
    if (ImGui::BeginTabItem("Graphics Pipeline")) {
      ImGui::Text("Test");
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}
} // namespace agnosia_imgui
