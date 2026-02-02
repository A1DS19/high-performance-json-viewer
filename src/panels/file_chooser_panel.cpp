#include "panels/file_chooser_panel.hpp"
#include "panel_manager.hpp"
#include "utils/json_parser.hpp"
#include "utils/loading_state.hpp"

#include <SDL3/SDL.h>
#include <array>
#include <cstring>
#include <imgui/imgui.h>
#include <string>
#include <thread>

namespace {
constexpr int PATH_BUFFER_SIZE = 512;
constexpr float WINDOW_WIDTH = 400.0F;
constexpr float WINDOW_HEIGHT = 120.0F;
constexpr float CENTER_PIVOT = 0.5F;
constexpr float BUTTON_PADDING = 80.0F;
constexpr float BUTTON_WIDTH = 70.0F;

void file_dialog_callback(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* path_buffer = static_cast<std::array<char, PATH_BUFFER_SIZE>*>(userdata);
    if (filelist != nullptr && filelist[0] != nullptr) {
        std::strncpy(path_buffer->data(), filelist[0], PATH_BUFFER_SIZE - 1);
        path_buffer->at(PATH_BUFFER_SIZE - 1) = '\0';
    }
}

void start_parsing_thread(const std::string& path) {
    // Detached thread for background parsing
    std::thread parser_thread([path]() { json_parser(path); });
    parser_thread.detach();
}
}  // namespace

void draw_file_chooser_panel() {
    static std::array<char, PATH_BUFFER_SIZE> path_buffer = {};

    LoadingState& state = get_loading_state();

    // Hide file chooser when loading
    if (state.is_loading) {
        return;
    }

    // Center the window
    ImGuiIO& imgui_io = ImGui::GetIO();
    ImVec2 center(imgui_io.DisplaySize.x * CENTER_PIVOT,
                  imgui_io.DisplaySize.y * CENTER_PIVOT);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(CENTER_PIVOT, CENTER_PIVOT));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Open JSON File", nullptr, window_flags);

    ImGui::Text("File path:");
    ImGui::SetNextItemWidth(-BUTTON_PADDING);
    ImGui::InputText("##path", path_buffer.data(), path_buffer.size());
    ImGui::SameLine();

    if (ImGui::Button("Browse", ImVec2(BUTTON_WIDTH, 0.0F))) {
        std::array<SDL_DialogFileFilter, 1> filters = {{{"JSON files", "json;ndjson"}}};
        SDL_ShowOpenFileDialog(file_dialog_callback, &path_buffer, nullptr, filters.data(),
                               filters.size(), nullptr, false);
    }

    if (ImGui::Button("Open", ImVec2(BUTTON_WIDTH, 0.0F))) {
        std::string path = path_buffer.data();
        if (!path.empty()) {
            start_parsing_thread(path);
        }
    }

    ImGui::End();
}

REGISTER_PANEL(draw_file_chooser_panel);
