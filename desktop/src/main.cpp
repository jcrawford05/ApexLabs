// Apex Lab desktop foundation: SDL3 window + SDL_Renderer3 + Dear ImGui.
//
// Initialization and shutdown flow adapted from the existing team prototype:
// C:/Users/cesar/Documents/ApexLabsDevSpace at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.

#include <cstdio>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "app.h"
#include "theme.h"

namespace {

constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;
constexpr int kMinimumWindowWidth = 1280;
constexpr int kMinimumWindowHeight = 760;

void Shutdown(SDL_Window* window, SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

}  // namespace

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Apex Lab",
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(
        window,
        kMinimumWindowWidth,
        kMinimumWindowHeight);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
        std::fprintf(stderr, "Dear ImGui SDL3 backend initialization failed.\n");
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        std::fprintf(stderr, "Dear ImGui SDL_Renderer3 backend initialization failed.\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const char* base_path = SDL_GetBasePath();
    const std::string base_directory = base_path ? base_path : "";
    float pixel_scale = SDL_GetWindowPixelDensity(window);
    if (pixel_scale <= 0) {
        pixel_scale = 1;
    }
    if (!theme::LoadFonts(base_directory + "assets/fonts", pixel_scale)) {
        std::fprintf(stderr, "Apex Lab fonts could not be loaded.\n");
        Shutdown(window, renderer);
        return 1;
    }
    theme::ApplyStyle();

    App app;
    std::string error;
    if (!app.Init(window, renderer, base_directory, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        Shutdown(window, renderer);
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
            // .json dropped on the window, or picked in the Import dialog.
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data) {
                app.ImportFile(event.drop.data);
            }
            if (app.import_event != 0 && event.type == app.import_event &&
                event.user.data1) {
                app.ImportFile(static_cast<const char*>(event.user.data1));
                SDL_free(event.user.data1);
            }
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        app.Frame();

        ImGui::Render();
        SDL_SetRenderScale(
            renderer,
            io.DisplayFramebufferScale.x,
            io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 18, 18, 22, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        if (app.request_quit) {
            running = false;
        }
    }

    app.Shutdown();
    Shutdown(window, renderer);
    return 0;
}
