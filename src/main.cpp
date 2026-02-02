#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"
#include "panel_manager.hpp"

#include <SDL3/SDL.h>
#include <iostream>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;
constexpr float CLEAR_R = 0.45F;
constexpr float CLEAR_G = 0.55F;
constexpr float CLEAR_B = 0.60F;
constexpr float CLEAR_A = 1.00F;

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    std::cerr << "Error: SDL_Init(): " << SDL_GetError() << "\n";
    return 1;
  }

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  SDL_Window *window = SDL_CreateWindow(
      "JSON Viewer", static_cast<int>(WINDOW_WIDTH * main_scale),
      static_cast<int>(WINDOW_HEIGHT * main_scale), window_flags);
  if (window == nullptr) {
    std::cerr << "Error: SDL_CreateWindow(): " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr) {
    std::cerr << "Error: SDL_CreateRenderer(): " << SDL_GetError() << "\n";
    return 1;
  }
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &imgui_io = ImGui::GetIO();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  ImVec4 clear_color = ImVec4(CLEAR_R, CLEAR_G, CLEAR_B, CLEAR_A);

  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT) {
        done = true;
      }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window)) {
        done = true;
      }
    }

    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    PanelManager::instance().draw_all();

    ImGui::Render();
    SDL_SetRenderScale(renderer, imgui_io.DisplayFramebufferScale.x,
                       imgui_io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y,
                                clear_color.z, clear_color.w);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
