#pragma once

#include <functional>
#include <utility>
#include <vector>

// Singleton that manages all UI panels.
// Panels register themselves at static initialization time,
// then draw_all() is called each frame to render them.
class PanelManager {
public:
    // Returns the single global instance (lazy initialization)
    static PanelManager& instance() {
        static PanelManager manager;
        return manager;
    }

    // Register a panel's draw function
    void add(std::function<void()> draw_fn) { panels_.push_back(std::move(draw_fn)); }

    // Call all registered panel draw functions (call once per frame)
    void draw_all() const {
        for (const auto& panel : panels_) {
            panel();
        }
    }

private:
    PanelManager() = default;
    std::vector<std::function<void()>> panels_;
};

// Helper struct for static auto-registration.
// When constructed, it registers the given function with PanelManager.
struct PanelRegistrar {
    explicit PanelRegistrar(std::function<void()> draw_fn) {
        PanelManager::instance().add(std::move(draw_fn));
    }
};

// Macro for auto-registering a panel draw function.
// Usage: REGISTER_PANEL(MyDrawFunction);
// This creates a static PanelRegistrar that runs at program startup.
#define REGISTER_PANEL(func) static PanelRegistrar _registrar_##func(func)
