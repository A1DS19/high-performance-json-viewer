#include "panels/json_viewer_panel.hpp"

#include "panel_manager.hpp"
#include "utils/json_data_store.hpp"
#include "utils/json_parser.hpp"

#include <SDL3/SDL.h>
#include <imgui/imgui.h>
#include <simdjson.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int SEARCH_BUFFER_SIZE = 256;
constexpr float TOOLBAR_BUTTON_WIDTH = 100.0F;
constexpr size_t SEARCH_BATCH_SIZE = 1000; // Documents to search per frame

// Pretty-print JSON with indentation
std::string format_json(const std::string& raw_json) {
    simdjson::dom::parser parser;
    auto doc = parser.parse(raw_json);
    if (doc.error() != simdjson::SUCCESS) {
        return raw_json; // Return as-is if parsing fails
    }

    std::ostringstream oss;
    oss << simdjson::to_string(doc.value());

    // simdjson::to_string produces minified JSON, so we need to pretty-print
    std::string minified = oss.str();
    std::ostringstream formatted;

    int indent = 0;
    bool in_string = false;
    constexpr int INDENT_SIZE = 2;

    for (size_t i = 0; i < minified.size(); i++) {
        char chr = minified[i];

        if (chr == '"' && (i == 0 || minified[i - 1] != '\\')) {
            in_string = !in_string;
            formatted << chr;
            continue;
        }

        if (in_string) {
            formatted << chr;
            continue;
        }

        switch (chr) {
        case '{':
        case '[':
            formatted << chr << '\n';
            indent += INDENT_SIZE;
            formatted << std::string(static_cast<size_t>(indent), ' ');
            break;
        case '}':
        case ']':
            formatted << '\n';
            indent -= INDENT_SIZE;
            if (indent < 0) {
                indent = 0;
            }
            formatted << std::string(static_cast<size_t>(indent), ' ') << chr;
            break;
        case ',':
            formatted << chr << '\n' << std::string(static_cast<size_t>(indent), ' ');
            break;
        case ':':
            formatted << chr << ' ';
            break;
        default:
            formatted << chr;
            break;
        }
    }

    return formatted.str();
}

void file_dialog_callback(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* path_buffer = static_cast<std::array<char, SEARCH_BUFFER_SIZE>*>(userdata);
    if (filelist != nullptr && filelist[0] != nullptr) {
        std::strncpy(path_buffer->data(), filelist[0], SEARCH_BUFFER_SIZE - 1);
        path_buffer->at(SEARCH_BUFFER_SIZE - 1) = '\0';

        // Start parsing in background thread
        std::string path = path_buffer->data();
        std::thread parser_thread([path]() { json_parser(path); });
        parser_thread.detach();
    }
}

} // namespace

void draw_json_viewer_panel() {
    JsonDataStore& data = get_json_data();

    if (!data.is_ready()) {
        return;
    }

    static std::array<char, SEARCH_BUFFER_SIZE> search_buffer = {};
    static std::array<char, SEARCH_BUFFER_SIZE> file_path_buffer = {};
    static std::vector<size_t> filtered_indices;
    static std::string active_search;
    static size_t last_generation = 0;

    // Search state for incremental searching
    static bool search_in_progress = false;
    static size_t search_current_index = 0;
    static std::string search_query;

    size_t total_count = data.document_count();

    // Reset state when a new file is loaded
    size_t current_generation = data.generation();
    if (current_generation != last_generation) {
        last_generation = current_generation;
        search_buffer.fill('\0');
        active_search.clear();
        filtered_indices.clear();
        search_in_progress = false;
        search_current_index = 0;
        search_query.clear();
        // Initialize with all documents
        for (size_t i = 0; i < total_count; i++) {
            filtered_indices.push_back(i);
        }
    }

    // Incremental search processing
    if (search_in_progress) {
        size_t end_index = std::min(search_current_index + SEARCH_BATCH_SIZE, total_count);

        for (size_t i = search_current_index; i < end_index; i++) {
            std::string doc = data.get_document(i);
            if (doc.find(search_query) != std::string::npos) {
                filtered_indices.push_back(i);
            }
        }

        search_current_index = end_index;

        if (search_current_index >= total_count) {
            search_in_progress = false;
        }
    }

    // Make window fullscreen
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("JSON Documents", nullptr, WINDOW_FLAGS);

    // === TOOLBAR ===
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0F, 5.0F));

    // Open File button
    if (ImGui::Button("Open File", ImVec2(TOOLBAR_BUTTON_WIDTH, 0.0F))) {
        std::array<SDL_DialogFileFilter, 1> filters = {{{"JSON files", "json;ndjson;ndjson.gz"}}};
        SDL_ShowOpenFileDialog(file_dialog_callback, &file_path_buffer, nullptr, filters.data(),
                               filters.size(), nullptr, false);
    }

    ImGui::SameLine();

    // Search input
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300.0F);
    ImGui::InputText("##search", search_buffer.data(), search_buffer.size());

    ImGui::SameLine();
    // Disable Search button while search is in progress
    bool was_searching = search_in_progress;
    if (was_searching) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Search")) {
        std::string search_str = search_buffer.data();
        active_search = search_str;
        filtered_indices.clear();

        if (search_str.empty()) {
            // No filter - show all
            for (size_t i = 0; i < total_count; i++) {
                filtered_indices.push_back(i);
            }
        } else {
            // Start incremental search
            search_query = search_str;
            search_current_index = 0;
            search_in_progress = true;
        }
    }
    if (was_searching) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        search_buffer.fill('\0');
        active_search.clear();
        filtered_indices.clear();
        search_in_progress = false;
        search_current_index = 0;
        search_query.clear();
        for (size_t i = 0; i < total_count; i++) {
            filtered_indices.push_back(i);
        }
    }

    ImGui::PopStyleVar();

    // Search progress indicator
    if (search_in_progress) {
        float progress = static_cast<float>(search_current_index) / static_cast<float>(total_count);
        ImGui::ProgressBar(progress, ImVec2(-1.0F, 0.0F), "Searching...");
        ImGui::Text("Searched %zu / %zu documents, found %zu matches", search_current_index,
                    total_count, filtered_indices.size());
    }

    // Document count (only show when not searching)
    size_t display_count = filtered_indices.size();
    if (!search_in_progress) {
        if (active_search.empty()) {
            ImGui::Text("Total documents: %zu", total_count);
        } else {
            ImGui::Text("Showing %zu of %zu documents (search: \"%s\")", display_count, total_count,
                        active_search.c_str());
        }
    }

    ImGui::Separator();

    // === DOCUMENT LIST ===
    ImGui::BeginChild("DocumentList", ImVec2(0, 0), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Virtual scrolling - only renders visible items
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(display_count));

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            size_t doc_index = filtered_indices[static_cast<size_t>(i)];
            ImGui::PushID(static_cast<int>(doc_index));

            // Collapsible tree node for each document
            if (ImGui::TreeNode("", "Document %zu", doc_index)) {
                std::string raw_doc = data.get_document(doc_index);
                std::string formatted = format_json(raw_doc);

                // Calculate height based on line count
                size_t line_count = 1;
                for (char chr : formatted) {
                    if (chr == '\n') {
                        line_count++;
                    }
                }
                float line_height = ImGui::GetTextLineHeight();
                float text_height = static_cast<float>(line_count) * line_height + line_height;

                // Selectable read-only text input
                ImGui::InputTextMultiline("##json", formatted.data(), formatted.size() + 1,
                                          ImVec2(-FLT_MIN, text_height),
                                          ImGuiInputTextFlags_ReadOnly);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

REGISTER_PANEL(draw_json_viewer_panel);
