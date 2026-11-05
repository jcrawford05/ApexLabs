// Minimal application state and navigation for the Start Screen migration.
#pragma once

#include <string>

#include "textures.h"

struct SDL_Renderer;

enum class Screen {
    Start,
    CarSetup,
    ImportExport,
    Compare,
    Settings,
};

struct App {
    Screen screen = Screen::Start;
    bool request_quit = false;

    Image logo_wordmark;
    Image logo_badge;
    Image track_line_left;
    Image track_line_right;

    bool Init(
        SDL_Renderer* renderer,
        const std::string& base_directory,
        std::string& error);
    void Shutdown();
    void Frame();
    void GoTo(Screen destination);
};

void DrawStartScreen(App& app);
