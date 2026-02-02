#include "panels/loading_panel.hpp"
#include "panel_manager.hpp"
#include "utils/loading_state.hpp"

#include <imgui/imgui.h>

namespace {
constexpr float WINDOW_WIDTH = 500.0F;
constexpr float WINDOW_HEIGHT = 150.0F;
constexpr float CENTER_PIVOT = 0.5F;
} // namespace

void draw_loading_panel() {
  LoadingState &state = get_loading_state();

  // Only show when loading or just completed
  if (!state.is_loading && !state.is_complete) {
    return;
  }

  // Center the window
  ImGuiIO &imgui_io = ImGui::GetIO();
  ImVec2 center(imgui_io.DisplaySize.x * CENTER_PIVOT,
                imgui_io.DisplaySize.y * CENTER_PIVOT);
  ImGui::SetNextWindowPos(center, ImGuiCond_Always,
                          ImVec2(CENTER_PIVOT, CENTER_PIVOT));
  ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT),
                           ImGuiCond_Always);

  constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove;

  ImGui::Begin("Loading Status", nullptr, window_flags);

  // File size
  if (state.file_size_bytes > 0) {
    double size_gb = static_cast<double>(state.file_size_bytes.load()) /
                     (1024.0 * 1024.0 * 1024.0);
    ImGui::Text("File size: %zu bytes (%.2f GB)", state.file_size_bytes.load(),
                size_gb);
  }

  // Status message
  ImGui::Text("%s", state.status_message.c_str());

  // Documents loaded
  if (state.documents_loaded > 0) {
    ImGui::Text("Documents loaded: %zu", state.documents_loaded.load());
  }

  // Error message
  if (!state.error_message.empty()) {
    ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "Error: %s",
                       state.error_message.c_str());
  }

  // Loading indicator
  if (state.is_loading) {
    ImGui::Text("Please wait...");
  }

  // Close button when complete
  if (state.is_complete && !state.is_loading) {
    ImGui::Separator();
    if (ImGui::Button("Close", ImVec2(100.0F, 0.0F))) {
      state.reset();
    }
  }

  ImGui::End();
}

REGISTER_PANEL(draw_loading_panel);
