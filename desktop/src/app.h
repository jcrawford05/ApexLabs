// Application state and navigation for migrated desktop screens.
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

enum class CarPanel {
    Aerodynamics,
    Tires,
    Powertrain,
    Chassis,
    Brakes,
    Count,
};

enum class TrackPanel {
    Circuit,
    Weather,
    Surface,
    Simulation,
    Count,
};

struct App {
    Screen screen = Screen::Start;
    CarPanel car_panel = CarPanel::Aerodynamics;
    TrackPanel track_panel = TrackPanel::Circuit;
    bool track_focus = false;
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
void DrawSetupScreen(App& app);
void DrawTopNav(App& app, const char* right_label);
