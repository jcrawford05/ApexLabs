// Track Setup screen: circuit, weather, surface, and simulation panels.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "app.h"
#include "imgui.h"
#include "setup_common.h"
#include "theme.h"
#include "track_map.h"
#include "widgets.h"

using namespace theme;
using namespace setup_ui;

namespace {

const char* const kTrackTitles[] = {
    "Circuit",
    "Weather",
    "Surface",
    "Simulation",
};
const char* const kTrackSubtitles[] = {
    "Layout & geometry",
    "Air, track & rain",
    "Grip & roughness",
    "Laps, start & fuel",
};

void DrawMiniMap(ImVec2 position, ImVec2 size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 end(position.x + size.x, position.y + size.y);
    draw_list->AddRectFilled(position, end, setup::kMap, 9.0F);
    draw_list->PushClipRect(position, end, true);
    for (float x = 19.0F; x < size.x; x += 50.0F) {
        draw_list->AddRectFilled(
            ImVec2(position.x + x, position.y),
            ImVec2(position.x + x + 1.0F, end.y),
            setup::kMapGrid);
    }
    for (float y = 24.0F; y < size.y; y += 50.0F) {
        draw_list->AddRectFilled(
            ImVec2(position.x, position.y + y),
            ImVec2(end.x, position.y + y + 1.0F),
            setup::kMapGrid);
    }

    const TrackGeometry& geometry = DemoCircuitGeometry();
    const MapTransform transform = FitTrack(
        geometry,
        ImVec2(position.x + 47.0F, position.y + 29.0F),
        ImVec2(300.0F, 160.0F),
        6.0F);
    for (const auto& section : geometry.sections) {
        std::vector<ImVec2> points;
        points.reserve(section.size());
        for (ImVec2 vertex : section) {
            points.push_back(transform(vertex));
        }
        draw_list->AddPolyline(
            points.data(),
            static_cast<int>(points.size()),
            Hex(0xE10600, 50),
            0,
            8.0F);
    }
    for (const auto& section : geometry.sections) {
        std::vector<ImVec2> points;
        points.reserve(section.size());
        for (ImVec2 vertex : section) {
            points.push_back(transform(vertex));
        }
        draw_list->AddPolyline(
            points.data(),
            static_cast<int>(points.size()),
            kRed,
            0,
            3.5F);
    }
    draw_list->AddLine(
        transform(ImVec2(216.0F, 446.0F)),
        transform(ImVec2(216.0F, 466.0F)),
        kWhite,
        2.0F);
    draw_list->PopClipRect();
    draw_list->AddRect(position, end, setup::kSummaryBorder, 9.0F);
    ImGui::Dummy(size);
}

void CircuitPanel(App& app) {
    const Track& track = app.track;
    std::string pill = track.name;
    for (char& character : pill) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
    }
    PanelHeader(
        "Circuit",
        "Select a circuit and inspect its characteristics.",
        pill.c_str(),
        18.0F);

    DrawMiniMap(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetContentRegionAvail().x, 225.0F));
    ui::Space(9.0F);
    ui::Text(Face::PlexSemiBold, 9.0F, setup::kMuted, "SELECT CIRCUIT");
    ui::Space(9.0F);
    if (ui::Pill("##demo", track.name.c_str(), true)) {
    }
    ImGui::SameLine(0.0F, 5.0F);
    if (ui::Pill("##apex02", "Apex 02", false)) {
        app.ShowToast("Only the Demo Circuit is in the sample data so far.");
    }
    ImGui::SameLine(0.0F, 5.0F);
    if (ui::Pill("##apex03", "Apex 03", false)) {
        app.ShowToast("Only the Demo Circuit is in the sample data so far.");
    }
    ImGui::SameLine(0.0F, 5.0F);
    if (ui::Pill("##browse", "Browse...", false)) {
        app.ShowToast("Opening track files isn't wired up yet.");
    }
    ui::Space(9.0F);

    BeginGroup("##trackstats");
    ui::SectionLabel("CIRCUIT PROFILE");
    ui::Space(8.0F);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f km", track.LengthM() / 1000.0F);
    KV("Length", buffer);
    ui::Space(8.0F);
    std::snprintf(buffer, sizeof(buffer), "%d", track.Corners());
    KV("Corners", buffer);
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%.2f km",
        track.LongestStraightM() / 1000.0F);
    KV("Longest straight", buffer);
    ui::Space(8.0F);
    KV("Character", track.Character(), true);
    EndGroup();
}

void ApplyPreset(Weather& weather, WeatherPreset preset) {
    weather.preset = preset;
    switch (preset) {
        case WeatherPreset::Dry:
            weather.rain_pct = 0.0F;
            weather.humidity_pct = 42.0F;
            break;
        case WeatherPreset::Overcast:
            weather.rain_pct = 0.0F;
            weather.humidity_pct = 65.0F;
            break;
        case WeatherPreset::LightRain:
            weather.rain_pct = 30.0F;
            weather.humidity_pct = 85.0F;
            break;
        case WeatherPreset::Wet:
            weather.rain_pct = 75.0F;
            weather.humidity_pct = 95.0F;
            break;
    }
}

void WeatherPanel(App& app) {
    Weather& weather = app.weather;
    std::string pill = WeatherPresetName(weather.preset);
    for (char& character : pill) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
    }
    PanelHeader(
        "Weather",
        "Define ambient and surface conditions for the run.",
        pill.c_str(),
        18.0F);

    const ImVec2 position = ImGui::GetCursorScreenPos();
    const ImVec2 size(ImGui::GetContentRegionAvail().x, 170.0F);
    const ImVec2 end(position.x + size.x, position.y + size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(position, end, setup::kMap, 9.0F);
    draw_list->PushClipRect(position, end, true);
    for (int i = 10; i > 0; --i) {
        draw_list->AddCircleFilled(
            ImVec2(position.x + 334.0F, position.y + 19.0F),
            9.0F * i,
            Hex(0xE10600, static_cast<uint8_t>(5 + (10 - i) * 1.2F)),
            48);
    }
    const bool raining = weather.preset == WeatherPreset::LightRain ||
        weather.preset == WeatherPreset::Wet;
    ui::IconWeather(
        draw_list,
        ImVec2(position.x + 24.0F, position.y + 37.0F),
        1.0F,
        raining);
    draw_list->PopClipRect();
    draw_list->AddRect(position, end, setup::kSummaryBorder, 9.0F);

    static const char* const kTitles[] = {
        "DRY CONDITIONS",
        "OVERCAST",
        "LIGHT RAIN",
        "WET TRACK",
    };
    static const char* const kDescs[] = {
        "Stable ambient conditions with no active precipitation.",
        "Cloud cover keeps the track cooler; no rain.",
        "Damp patches; grip drops and braking zones grow.",
        "Standing water; expect much lower grip.",
    };
    const int preset_index = static_cast<int>(weather.preset);
    ui::TextAt(
        draw_list,
        ImVec2(position.x + 141.0F, position.y + 41.0F),
        Face::PlexSemiBold,
        11.0F,
        setup::kTitle,
        kTitles[preset_index]);
    ui::TextAt(
        draw_list,
        ImVec2(position.x + 141.0F, position.y + 63.0F),
        Face::PlexRegular,
        9.0F,
        Hex(0x707782),
        kDescs[preset_index]);
    char temps[64];
    std::snprintf(
        temps,
        sizeof(temps),
        "%s AIR  \xC2\xB7  %s TRACK",
        FormatTemp(app, weather.air_c).c_str(),
        FormatTemp(app, weather.track_c).c_str());
    ui::TextAt(
        draw_list,
        ImVec2(position.x + 141.0F, position.y + 100.0F),
        Face::PlexSemiBold,
        13.0F,
        setup::kAccentValue,
        temps);
    ImGui::Dummy(size);
    ui::Space(9.0F);

    ui::BeginCard("##preset", setup::kGroup, setup::kGroupBorder, 9.0F, ImVec2(10, 8));
    ui::SectionLabel("WEATHER PRESET");
    ui::Space(7.0F);
    ui::PillStyle small{9.0F, 6.0F, 6.0F, 9.0F};
    const WeatherPreset presets[] = {
        WeatherPreset::Dry,
        WeatherPreset::Overcast,
        WeatherPreset::LightRain,
        WeatherPreset::Wet,
    };
    for (int i = 0; i < 4; ++i) {
        if (i) {
            ImGui::SameLine(0.0F, 5.0F);
        }
        ImGui::PushID(i);
        if (ui::Pill(
                "##p",
                WeatherPresetName(presets[i]),
                weather.preset == presets[i],
                small)) {
            ApplyPreset(weather, presets[i]);
        }
        ImGui::PopID();
    }
    ui::EndCard();
    ui::Space(9.0F);

    const bool fahrenheit = app.prefs.temp_unit == TempUnit::F;
    const char* temp_suffix = fahrenheit ? "\xC2\xB0" "F" : "\xC2\xB0" "C";
    float air_display = ConvertTemp(app.prefs.temp_unit, weather.air_c);
    float track_display = ConvertTemp(app.prefs.temp_unit, weather.track_c);
    const float air_min = fahrenheit ? ConvertTemp(TempUnit::F, 10.0F) : 10.0F;
    const float air_max = fahrenheit ? ConvertTemp(TempUnit::F, 40.0F) : 40.0F;
    const float track_min = fahrenheit ? ConvertTemp(TempUnit::F, 15.0F) : 15.0F;
    const float track_max = fahrenheit ? ConvertTemp(TempUnit::F, 60.0F) : 60.0F;
    char air_fmt[24], air_lo[24], air_hi[24];
    char track_fmt[24], track_lo[24], track_hi[24];
    std::snprintf(air_fmt, sizeof(air_fmt), "%%.0f %s", temp_suffix);
    std::snprintf(air_lo, sizeof(air_lo), "%.0f %s", air_min, temp_suffix);
    std::snprintf(air_hi, sizeof(air_hi), "%.0f %s", air_max, temp_suffix);
    std::snprintf(track_fmt, sizeof(track_fmt), "%%.0f %s", temp_suffix);
    std::snprintf(track_lo, sizeof(track_lo), "%.0f %s", track_min, temp_suffix);
    std::snprintf(track_hi, sizeof(track_hi), "%.0f %s", track_max, temp_suffix);

    ui::BeginCard("##temps", setup::kGroup, setup::kGroupBorder, 9.0F, ImVec2(10, 8));
    ui::SectionLabel("TEMPERATURE");
    ui::Space(7.0F);
    if (ui::SetupSlider(
            "##air",
            "Air temperature",
            &air_display,
            air_min,
            air_max,
            air_fmt,
            air_lo,
            air_hi,
            true,
            1.0F)) {
        weather.air_c = fahrenheit
            ? (air_display - 32.0F) * 5.0F / 9.0F
            : air_display;
    }
    ui::Space(7.0F);
    if (ui::SetupSlider(
            "##track",
            "Track temperature",
            &track_display,
            track_min,
            track_max,
            track_fmt,
            track_lo,
            track_hi,
            true,
            1.0F)) {
        weather.track_c = fahrenheit
            ? (track_display - 32.0F) * 5.0F / 9.0F
            : track_display;
    }
    ui::EndCard();
    ui::Space(9.0F);

    float wind_display = ConvertSpeed(app.prefs.speed_unit, weather.wind_kmh);
    const float wind_max =
        ConvertSpeed(app.prefs.speed_unit, 40.0F);
    char wind_fmt[32], wind_hi[32];
    std::snprintf(
        wind_fmt,
        sizeof(wind_fmt),
        "%%.0f %s",
        SpeedUnitLabel(app.prefs.speed_unit));
    std::snprintf(
        wind_hi,
        sizeof(wind_hi),
        "%.0f %s",
        wind_max,
        SpeedUnitLabel(app.prefs.speed_unit));

    ui::BeginCard("##atmo", setup::kGroup, setup::kGroupBorder, 9.0F, ImVec2(10, 8));
    ui::SectionLabel("ATMOSPHERE & PRECIPITATION");
    ui::Space(7.0F);
    ui::SetupSlider(
        "##hum",
        "Humidity",
        &weather.humidity_pct,
        0.0F,
        100.0F,
        "%.0f%%",
        "0%",
        "100%",
        true,
        1.0F);
    ui::Space(7.0F);
    if (ui::SetupSlider(
            "##wind",
            "Wind speed",
            &wind_display,
            0.0F,
            wind_max,
            wind_fmt,
            "0",
            wind_hi,
            true,
            1.0F)) {
        switch (app.prefs.speed_unit) {
            case SpeedUnit::Mph:
                weather.wind_kmh = wind_display / 0.621371F;
                break;
            case SpeedUnit::Ms:
                weather.wind_kmh = wind_display * 3.6F;
                break;
            case SpeedUnit::Kmh:
                weather.wind_kmh = wind_display;
                break;
        }
    }
    ui::Space(7.0F);
    if (ui::SetupSlider(
            "##rain",
            "Rain intensity",
            &weather.rain_pct,
            0.0F,
            100.0F,
            "%.0f%%",
            "Dry",
            "Heavy",
            true,
            1.0F)) {
        if (weather.rain_pct >= 50.0F) {
            weather.preset = WeatherPreset::Wet;
        } else if (weather.rain_pct > 0.0F) {
            weather.preset = WeatherPreset::LightRain;
        } else if (
            weather.preset == WeatherPreset::LightRain ||
            weather.preset == WeatherPreset::Wet) {
            weather.preset = WeatherPreset::Dry;
        }
    }
    ui::EndCard();
}

void SurfacePanel(App& app) {
    SurfaceSettings& surface = app.surface;
    static const char* const kStates[] = {"Green", "Rubbered-in", "Dusty"};
    static const float kGrip[] = {0.96F, 1.00F, 0.92F};
    static const char* const kEvolution[] = {"Static", "Evolving"};
    static const char* const kStatePills[] = {
        "GREEN TRACK",
        "RUBBERED-IN",
        "DUSTY",
    };
    char buffer[64];
    PanelHeader(
        "Surface",
        "Set how much grip the track offers.",
        kStatePills[surface.grip_state],
        18.0F);

    BeginGroup("##state");
    ui::SectionLabel("TRACK STATE");
    ui::Space(8.0F);
    PillChoice("##states", kStates, 3, &surface.grip_state);
    ui::Space(7.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        setup::kMuted,
        surface.grip_state == 0
            ? "Fresh or cleaned surface with little rubber laid down."
        : surface.grip_state == 1
            ? "Rubber from earlier sessions. The usual race-day state."
            : "Sand or debris on the racing line.");
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##rough");
    ui::SectionLabel("ROUGHNESS & EVOLUTION");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##roughness",
        "Roughness",
        &surface.roughness_pct,
        0.0F,
        100.0F,
        "%.0f%%",
        "Smooth",
        "Bumpy",
        false,
        1.0F);
    ui::Space(8.0F);
    int evolving = surface.track_evolution ? 1 : 0;
    if (PillChoice("##evo", kEvolution, 2, &evolving)) {
        surface.track_evolution = evolving == 1;
    }
    ui::Space(7.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        setup::kMuted,
        surface.track_evolution
            ? "Grip rises a little each lap as rubber builds up."
            : "Grip stays the same on every lap.");
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##sfsummary", true);
    SummaryTitle("EFFECTIVE GRIP");
    ui::Space(8.0F);
    const float rain = 1.0F - app.weather.rain_pct * 0.004F;
    const float rough = 1.0F - surface.roughness_pct * 0.0005F;
    const float grip = kGrip[surface.grip_state] * rain * rough;
    std::snprintf(buffer, sizeof(buffer), "\xC3\x97%.2f", grip);
    KV("Grip factor", buffer, grip < 0.95F);
    ui::Space(8.0F);
    const float lap_cost =
        (1.0F / grip - 1.0F) * kSampleLapTimeS * 0.35F;
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s%.1f s",
        lap_cost >= 0.0F ? "+" : "-",
        std::fabs(lap_cost));
    KV("Lap time vs ideal (estimate)", buffer, lap_cost > 0.3F);
    ui::Space(8.0F);
    KV(
        "Rain",
        app.weather.rain_pct > 0.0F ? "Lowering grip (see Weather)" : "None");
    EndGroup();
}

void SimulationPanel(App& app) {
    SimulationSettings& simulation = app.sim;
    static const char* const kStarts[] = {"Flying lap", "Standing start"};
    static const char* const kSteps[] = {"1 ms", "5 ms", "10 ms"};
    static const float kSecondsPerLap[] = {9.0F, 2.5F, 1.4F};
    char buffer[64];
    PanelHeader(
        "Simulation",
        "Choose how many laps to run and how they start.",
        nullptr,
        18.0F);

    BeginGroup("##laps");
    ui::SectionLabel("RUN");
    ui::Space(8.0F);
    float laps = static_cast<float>(simulation.laps);
    if (ui::SetupSlider("##laps", "Laps", &laps, 1.0F, 20.0F, "%.0f", "1", "20", false, 1.0F)) {
        simulation.laps = static_cast<int>(laps);
    }
    ui::Space(8.0F);
    PillChoice("##start", kStarts, 2, &simulation.start);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##fuel");
    ui::SectionLabel("FUEL");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##fuelkg",
        "Starting fuel",
        &simulation.fuel_kg,
        5.0F,
        110.0F,
        "%.0f kg",
        "5 kg",
        "110 kg",
        false,
        1.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##simsummary", true);
    SummaryTitle("THIS RUN");
    ui::Space(8.0F);
    const float seconds =
        kSecondsPerLap[app.prefs.time_step] * static_cast<float>(simulation.laps);
    if (seconds < 60.0F) {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "about %.0f s",
            std::ceil(seconds));
    } else {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "about %.0f min",
            std::ceil(seconds / 60.0F));
    }
    KV("Simulation time", buffer, seconds > 60.0F);
    ui::Space(8.0F);
    const int points = static_cast<int>(
        app.track.LengthM() / app.prefs.telemetry_spacing_m) *
        simulation.laps;
    std::snprintf(buffer, sizeof(buffer), "%d", points);
    std::string points_label = buffer;
    for (int i = static_cast<int>(points_label.size()) - 3; i > 0; i -= 3) {
        points_label.insert(static_cast<size_t>(i), ",");
    }
    KV("Telemetry points", points_label.c_str());
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "+%.1f s on lap 1",
        simulation.fuel_kg * 0.03F);
    KV("Fuel weight cost", buffer, simulation.fuel_kg > 60.0F);
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s (change in Settings)",
        kSteps[app.prefs.time_step]);
    KV("Physics time step", buffer);
    EndGroup();
}

void DrawTrackDetail(App& app) {
    switch (app.track_panel) {
        case TrackPanel::Circuit:
            CircuitPanel(app);
            break;
        case TrackPanel::Weather:
            WeatherPanel(app);
            break;
        case TrackPanel::Surface:
            SurfacePanel(app);
            break;
        case TrackPanel::Simulation:
            SimulationPanel(app);
            break;
        case TrackPanel::Count:
            break;
    }
}

void StepTrack(App& app, int direction) {
    const int count = static_cast<int>(TrackPanel::Count);
    int index = static_cast<int>(app.track_panel) + direction;
    index = std::clamp(index, 0, count - 1);
    app.track_panel = static_cast<TrackPanel>(index);
    app.track_focus = true;
}

void DrawTrackCategoryFooter(App& app, float column_width, float column_height) {
    bool prev = false;
    bool next = false;
    DrawPrevNextFooter(column_width, column_height, "track_steps", &prev, &next);
    if (prev) {
        StepTrack(app, -1);
    }
    if (next) {
        StepTrack(app, +1);
    }
}

}  // namespace

void DrawTrackSetupScreen(App& app) {
    DrawTopNav(app, app.track.name.c_str());

    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 window_size = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const float left = origin.x + 24.0F;
    const float right = origin.x + window_size.x - 24.0F;
    ui::TextAt(
        draw_list,
        ImVec2(left, origin.y + 84.5F),
        Face::SpaceBold,
        24.0F,
        setup::kTitle,
        "Track Setup");
    ui::TextAt(
        draw_list,
        ImVec2(left, origin.y + 118.5F),
        Face::PlexRegular,
        10.0F,
        Hex(0x737A84),
        "Select a circuit and conditions, then run the simulation.");

    constexpr float kRunWidth = 206.0F;
    constexpr float kButtonGap = 10.0F;
    constexpr float kHeaderPillGap = 8.0F;
    const char* back_label = "Back to Car Setup";
    const float back_width = std::max(
        220.0F,
        ui::TextSize(Face::PlexSemiBold, 16.0F, back_label).x + 36.0F);
    const float reset_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "RESET").x + 24.0F;
    const float save_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "SAVE SETUP").x + 24.0F;
    const float header_actions_width = reset_width + kHeaderPillGap +
        save_width + kHeaderPillGap + back_width + kButtonGap + kRunWidth;

    ImGui::SetCursorScreenPos(
        ImVec2(right - header_actions_width, origin.y + 93.5F));
    if (ui::Pill("##reset", "RESET", false)) {
        app.track = app.saved_track;
        app.weather = app.saved_weather;
        app.surface = app.saved_surface;
        app.sim = app.saved_sim;
        app.ShowToast("Setup reset to the last saved version.");
    }
    ImGui::SameLine(0.0F, kHeaderPillGap);
    if (ui::Pill("##save", "SAVE SETUP", false)) {
        app.SaveSetup();
    }

    ImGui::SetCursorScreenPos(
        ImVec2(right - kRunWidth - kButtonGap - back_width, origin.y + 85.0F));
    if (ui::BigOutlineButton(
            "##tocar",
            back_label,
            ImVec2(back_width, 46.0F))) {
        app.track_focus = false;
        app.GoTo(Screen::CarSetup);
    }

    ImGui::SetCursorScreenPos(ImVec2(right - kRunWidth, origin.y + 85.0F));
    if (ui::BigRedButton("##run", "RUN SIMULATION", ImVec2(kRunWidth, 46.0F))) {
        RunSimulationAction(app);
    }

    const float available = window_size.x - 48.0F - 48.0F;
    const float top = origin.y + 154.0F;
    const float height = std::max(300.0F, window_size.y - 154.0F - 32.0F);
    const float cat_width = SetupColumnWidth(225.0F, available);
    const float detail_width = SetupColumnWidth(431.0F, available);

    float x = left;
    BeginPanel(
        "##trackcats",
        ImVec2(x, top),
        ImVec2(cat_width, height),
        ImVec2(15, 15));
    int clicked = -1;
    CategoryColumn(
        "Track Setup",
        "Choose the circuit conditions.",
        kTrackTitles,
        kTrackSubtitles,
        static_cast<int>(TrackPanel::Count),
        static_cast<int>(app.track_panel),
        ImGui::GetContentRegionAvail().x,
        &clicked);
    if (clicked >= 0) {
        app.track_panel = static_cast<TrackPanel>(clicked);
        app.track_focus = true;
    }
    DrawTrackCategoryFooter(app, cat_width, height);
    EndPanel();
    x += cat_width + 16.0F;

    BeginPanel(
        "##trackdetail",
        ImVec2(x, top),
        ImVec2(detail_width, height),
        ImVec2(17, 15));
    DrawTrackDetail(app);
    EndPanel();
}
