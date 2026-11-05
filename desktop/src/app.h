// Application state and navigation for migrated desktop screens.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "run_result.h"
#include "textures.h"
#include "track_setup.h"
#include "vehicle.h"

struct SDL_Renderer;
struct SDL_Window;

enum class Screen {
    Start,
    CarSetup,
    Results,
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

enum class MapColoring { DeltaVsReference, Speed, ThrottleBrake };
enum class TraceChannel { Speed, Throttle, Brake, TireTemp };
enum class SpeedUnit { Kmh, Mph, Ms };
enum class TempUnit { C, F };

// App-wide defaults from Settings. Live edits go into App::prefs; Save copies
// them to App::saved_prefs.
struct Preferences {
    int regulation = 0;  // index into kRegulations
    int time_step = 1;   // 0 = 1 ms, 1 = 5 ms, 2 = 10 ms
    float telemetry_spacing_m = 5.0F;
    bool default_wet = false;
    float default_air_c = 24.0F;
    float default_track_c = 38.0F;
    int reference_lap = 0;  // index into kReferenceLaps
    SpeedUnit speed_unit = SpeedUnit::Kmh;
    TempUnit temp_unit = TempUnit::C;
    MapColoring default_map_coloring = MapColoring::DeltaVsReference;
    float neutral_band_s = 0.05F;
    bool show_section_numbers = true;
    std::string database_path = "~/Documents/ApexLabs/apex_labs.db";
    bool autosave_runs = true;
    int keep_telemetry = 1;  // index into kKeepTelemetry
    bool telemetry_in_exports = false;

    int DifferencesFrom(const Preferences& other) const;
};

extern const char* const kRegulations[];
extern const int kRegulationCount;
extern const char* const kReferenceLaps[];
extern const int kReferenceLapCount;
extern const char* const kKeepTelemetry[];
extern const int kKeepTelemetryCount;

struct App;

const char* SpeedUnitLabel(SpeedUnit unit);
float ConvertSpeed(SpeedUnit unit, float kmh);
float ConvertTemp(TempUnit unit, float celsius);
std::string FormatSpeed(const App& app, float kmh, bool with_unit = true);
std::string FormatTemp(const App& app, float celsius);

struct App {
    struct LogEntry {
        std::string text;
        bool ok = false;
        std::string when;
    };

    Screen screen = Screen::Start;
    CarPanel car_panel = CarPanel::Aerodynamics;
    TrackPanel track_panel = TrackPanel::Circuit;
    bool track_focus = false;
    bool request_quit = false;

    Vehicle vehicle;
    Vehicle saved_vehicle;
    Track track = MakeDemoCircuit();
    Weather weather;
    SurfaceSettings surface;
    SimulationSettings sim;
    Preferences prefs;
    Preferences saved_prefs;
    RunResult run = MakeSampleRun();
    std::vector<RunSummary> history = MakeSampleHistory();
    std::vector<LogEntry> import_log;
    uint32_t import_event = 0;
    SDL_Window* window = nullptr;

    MapColoring map_coloring = MapColoring::DeltaVsReference;
    TraceChannel trace_channel = TraceChannel::Speed;
    int open_section = -1;
    int hover_section = -1;
    int compare_a = 42;
    int compare_b = 41;

    std::string toast;
    double toast_until = 0.0;

    Image logo_wordmark;
    Image logo_badge;
    Image track_line_left;
    Image track_line_right;

    bool Init(
        SDL_Window* window,
        SDL_Renderer* renderer,
        const std::string& base_directory,
        std::string& error);
    void Shutdown();
    void Frame();
    void GoTo(Screen destination);
    void ShowToast(const std::string& text, double seconds = 3.0);
    void SaveSetup();

    std::string ExportFolder() const;
    void ExportRunJson();
    void ExportCarJson();
    void ExportTrackJson();
    void ImportFile(const std::string& path);
    void OpenImportDialog();
    const RunSummary* FindRun(int run_id) const;
};

void DrawStartScreen(App& app);
void DrawSetupScreen(App& app);
void DrawResultsScreen(App& app);
void DrawImportExportScreen(App& app);
void DrawCompareScreen(App& app);
void DrawSettingsScreen(App& app);
void DrawTopNav(App& app, const char* right_label);
