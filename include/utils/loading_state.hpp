#pragma once

#include <atomic>
#include <cstddef>
#include <string>

// Shared state for loading progress (thread-safe)
struct LoadingState {
    std::atomic<bool> is_loading{false};
    std::atomic<bool> is_complete{false};
    std::atomic<size_t> documents_loaded{0};
    std::atomic<size_t> file_size_bytes{0};
    std::string status_message;
    std::string error_message;

    void reset() {
        is_loading = false;
        is_complete = false;
        documents_loaded = 0;
        file_size_bytes = 0;
        status_message.clear();
        error_message.clear();
    }
};

// Global loading state (accessible from UI and parser)
LoadingState& get_loading_state();
