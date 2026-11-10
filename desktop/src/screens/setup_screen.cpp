// Car & Track Setup migration from ApexLabsDevSpace commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0 (Figma 1440 x 900).
//
// Car & Track Setup: all car and track configuration panels.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "track_map.h"
#include "widgets.h"

using namespace theme;

namespace {

const char* const kCarTitles[] = {
    "Aerodynamics",
    "Tires",
    "Powertrain",
    "Chassis",
    "Brakes",
};
const char* const kCarSubtitles[] = {
    "Wings, floor & drag",
    "Compound & pressure",
    "Power & gearing",
    "Mass & ride height",
    "Bias & cooling",
};
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

// ImGui's default wheel scroll jumps by coarse steps. Own the wheel here and
// ease ScrollY toward a target so the Aerodynamics (and sibling) panels feel
// continuous, especially on trackpads.
void ApplySmoothPanelScroll() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID target_id = ImGui::GetID("smooth_scroll_target");
    const ImGuiID last_id = ImGui::GetID("smooth_scroll_last");

    const float scroll = ImGui::GetScrollY();
    const float max_scroll = ImGui::GetScrollMaxY();
    float target = storage->GetFloat(target_id, scroll);
    const float last_set = storage->GetFloat(last_id, scroll);

    // Scrollbar drag updates ScrollY directly; keep the eased target in sync.
    if (std::fabs(scroll - last_set) > 0.5F && io.MouseWheel == 0.0F) {
        target = scroll;
    }

    // Include nested BeginCard children; plain IsWindowHovered() is false when
    // the cursor is over FRONT AERO / REAR / summary groups.
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        io.MouseWheel != 0.0F) {
        constexpr float kPixelsPerWheelUnit = 56.0F;
        target -= io.MouseWheel * kPixelsPerWheelUnit;
    }

    target = std::clamp(target, 0.0F, std::max(max_scroll, 0.0F));
    constexpr float kSmoothing = 16.0F;
    const float blend = 1.0F - std::exp(-kSmoothing * io.DeltaTime);
    const float next = scroll + (target - scroll) * blend;
    ImGui::SetScrollY(next);

    storage->SetFloat(target_id, target);
    storage->SetFloat(last_id, next);
}

void BeginPanel(const char* id, ImVec2 position, ImVec2 size, ImVec2 padding) {
    ImGui::SetCursorScreenPos(position);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, setup::kPanel);
    ImGui::PushStyleColor(ImGuiCol_Border, setup::kPanelBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(
        id,
        size,
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollWithMouse);
}

void EndPanel() {
    ApplySmoothPanelScroll();
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginGroup(const char* id, bool summary = false) {
    ui::BeginCard(
        id,
        summary ? setup::kSummary : setup::kGroup,
        summary ? setup::kSummaryBorder : setup::kGroupBorder,
        9.0F,
        ImVec2(10.0F, 9.0F));
}

void EndGroup() {
    ui::EndCard();
}

void PanelHeader(
    const char* title,
    const char* subtitle,
    const char* pill,
    float title_size) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ui::TextAt(draw_list, position, Face::SpaceBold, title_size, setup::kTitle, title);
    const float title_height = LineHeight(Face::SpaceBold, title_size);
    if (pill) {
        const float pill_width =
            ui::TextSize(Face::PlexSemiBold, 10.0F, pill).x + 24.0F;
        ImGui::SetCursorScreenPos(
            ImVec2(position.x + width - pill_width, position.y + 4.5F));
        ui::Pill("##headerpill", pill, true);
    }
    ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + title_height + 3.0F));
    ui::Text(Face::PlexRegular, 9.0F, setup::kMuted, subtitle);
    ui::Space(9.0F);
    ui::Divider(setup::kDivider);
    ui::Space(9.0F);
}

void KV(const char* key, const char* value, bool accent = false) {
    ui::KeyValue(
        key,
        value,
        accent ? setup::kAccentValue : setup::kKvValue,
        Face::PlexRegular,
        10.0F,
        Face::PlexSemiBold,
        11.0F,
        setup::kMuted);
}

void CategoryColumn(App& app, bool track, float card_width) {
    ui::Text(
        Face::SpaceBold,
        17.0F,
        Hex(0xF3F4F5),
        track ? "Track Setup" : "Car Setup");
    ui::Space(8.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        Hex(0x6E7580),
        track ? "Choose the circuit conditions."
              : "Choose a vehicle system to configure.");
    ui::Space(8.0F);
    ui::Divider(setup::kDivider);
    ui::Space(8.0F);

    const int count =
        track ? static_cast<int>(TrackPanel::Count)
              : static_cast<int>(CarPanel::Count);
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        const bool selected = track
            ? static_cast<int>(app.track_panel) == i
            : static_cast<int>(app.car_panel) == i;
        if (ui::CategoryCard(
                "##cat",
                track ? kTrackTitles[i] : kCarTitles[i],
                track ? kTrackSubtitles[i] : kCarSubtitles[i],
                selected,
                card_width)) {
            if (track) {
                app.track_panel = static_cast<TrackPanel>(i);
            } else {
                app.car_panel = static_cast<CarPanel>(i);
            }
            app.track_focus = track;
        }
        ImGui::PopID();
        ui::Space(8.0F);
    }
}

void SummaryTitle(const char* text) {
    ui::Text(Face::PlexSemiBold, 9.0F, Hex(0x646B76), text);
}

int IndexOf(
    const char* const* items,
    int count,
    const std::string& value,
    int fallback) {
    for (int i = 0; i < count; ++i) {
        if (value == items[i]) {
            return i;
        }
    }
    return fallback;
}

bool PillChoice(
    const char* id,
    const char* const* items,
    int count,
    int* selected) {
    bool changed = false;
    ImGui::PushID(id);
    for (int i = 0; i < count; ++i) {
        if (i) {
            ImGui::SameLine(0.0F, 6.0F);
        }
        ImGui::PushID(i);
        if (ui::Pill("##p", items[i], *selected == i) && *selected != i) {
            *selected = i;
            changed = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

void AerodynamicsPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    PanelHeader(
        "Aerodynamics",
        "Tune the car's high-speed efficiency and balance.",
        vehicle.AeroBias(),
        18.0F);

    BeginGroup("##front");
    ui::SectionLabel("FRONT AERO");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##fw",
        "Front wing angle",
        &vehicle.front_wing_deg,
        2.0F,
        15.0F,
        "%.1f\xC2\xB0",
        "2\xC2\xB0",
        "15\xC2\xB0",
        false,
        0.5F);
    ui::Space(8.0F);
    ui::SetupSlider(
        "##frh",
        "Front ride height",
        &vehicle.front_ride_height_mm,
        22.0F,
        45.0F,
        "%.0f mm",
        "22 mm",
        "45 mm",
        false,
        1.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##rear");
    ui::SectionLabel("REAR / FLOOR");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##rw",
        "Rear wing angle",
        &vehicle.rear_wing_deg,
        4.0F,
        20.0F,
        "%.1f\xC2\xB0",
        "4\xC2\xB0",
        "20\xC2\xB0",
        false,
        0.5F);
    ui::Space(8.0F);
    ui::SetupSlider(
        "##floor",
        "Floor load",
        &vehicle.floor_load_pct,
        0.0F,
        100.0F,
        "%.0f%%",
        "Low",
        "High",
        false,
        1.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##aerosummary", true);
    ui::Text(Face::PlexSemiBold, 9.0F, Hex(0x646B76), "CURRENT CONFIGURATION");
    ui::Space(8.0F);
    char mass[32];
    std::snprintf(mass, sizeof(mass), "%.0f kg", vehicle.TotalMassKg());
    KV("Mass", mass);
    ui::Space(8.0F);
    KV("Drag tendency", vehicle.DragTendency(), true);
    EndGroup();
}

void TiresPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    const TireCompound& tire = vehicle.SelectedTire();
    PanelHeader(
        "Tires",
        "Configure compound, pressures, temperatures, and wear.",
        nullptr,
        18.0F);

    BeginGroup("##compound");
    ui::SectionLabel("COMPOUND");
    ui::Space(8.0F);
    for (int i = 0; i < TireCatalogCount(); ++i) {
        if (i) {
            ImGui::SameLine(0.0F, 6.0F);
        }
        ImGui::PushID(i);
        if (ui::Pill("##c", TireAt(i).short_name, vehicle.compound == i)) {
            vehicle.compound = i;
        }
        ImGui::PopID();
    }
    ui::Space(8.0F);
    KV("Estimated grip", tire.grip, std::strcmp(tire.grip, "High") == 0);
    ui::Space(8.0F);
    KV(
        "Expected durability",
        tire.durability,
        std::strcmp(tire.durability, "Low") == 0);
    ui::Space(8.0F);
    KV("Warm-up", tire.warm_up);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##pressures");
    ui::SectionLabel("PRESSURES");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##pf",
        "Front pressure",
        &vehicle.pressure_front_psi,
        20.0F,
        26.0F,
        "%.1f psi",
        "20 psi",
        "26 psi",
        false,
        0.1F);
    ui::Space(8.0F);
    ui::SetupSlider(
        "##pr",
        "Rear pressure",
        &vehicle.pressure_rear_psi,
        20.0F,
        26.0F,
        "%.1f psi",
        "20 psi",
        "26 psi",
        false,
        0.1F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##tiresummary", true);
    ui::Text(Face::PlexSemiBold, 9.0F, Hex(0x646B76), "PROJECTED TIRE BEHAVIOR");
    ui::Space(8.0F);
    KV("Peak grip window", tire.peak_window);
    ui::Space(8.0F);
    char life[32];
    std::snprintf(life, sizeof(life), "%d laps", tire.life_laps);
    KV("Estimated life", life);
    ui::Space(8.0F);
    KV(
        "Degradation",
        tire.degradation,
        std::strcmp(tire.degradation, "High") == 0);
    ui::Space(8.0F);
    char mass[32];
    std::snprintf(mass, sizeof(mass), "%.0f kg", tire.mass_kg);
    KV("Mass contribution", mass);
    EndGroup();
}

void PowertrainPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    static const char* const kMaps[] = {"Qualifying", "Race", "Save"};
    static const float kPower[] = {1.0F, 0.97F, 0.92F};
    static const char* const kFuel[] = {"High", "Medium", "Low"};
    int map = IndexOf(kMaps, 3, vehicle.power_map, 1);
    char buffer[64];

    PanelHeader(
        "Powertrain",
        "Set engine power delivery and gearing.",
        map == 0 ? "MAX POWER" : nullptr,
        18.0F);

    BeginGroup("##map");
    ui::SectionLabel("POWER MAP");
    ui::Space(8.0F);
    if (PillChoice("##maps", kMaps, 3, &map)) {
        vehicle.power_map = kMaps[map];
    }
    ui::Space(7.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        setup::kMuted,
        map == 0 ? "Full power for one lap. Not allowed for long runs."
        : map == 1 ? "Default race mode, balanced against fuel use."
                   : "Lower power to save fuel and engine life.");
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##gearing");
    ui::SectionLabel("GEARING");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##fd",
        "Final drive ratio",
        &vehicle.final_drive,
        2.8F,
        3.6F,
        "%.2f",
        "Long",
        "Short",
        false,
        0.05F);
    ui::Space(8.0F);
    ui::SetupSlider(
        "##shift",
        "Shift point",
        &vehicle.shift_rpm,
        10500.0F,
        vehicle.engine_max_rpm,
        "%.0f rpm",
        "10,500",
        "15,000",
        false,
        100.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##ptsummary", true);
    SummaryTitle("PROJECTED OUTPUT");
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%.0f kW",
        vehicle.engine_peak_kw * kPower[map]);
    KV("Peak power", buffer);
    ui::Space(8.0F);
    // Speed at the rev limit in top gear scales with 1 / final drive
    // (340 km/h at 3.20), matching DevSpace setup_screen.cpp.
    const float limit_kmh = 340.0F * 3.2F / vehicle.final_drive *
        (vehicle.shift_rpm / vehicle.engine_max_rpm);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s",
        FormatSpeed(app, limit_kmh).c_str());
    constexpr float kSampleTopSpeedKmh = 317.0F;
    KV("Top-gear speed limit", buffer, limit_kmh < kSampleTopSpeedKmh + 5.0F);
    ui::Space(8.0F);
    KV("Fuel use", kFuel[map], map == 0);
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s, %.0f kg",
        vehicle.engine_name.c_str(),
        vehicle.engine_mass_kg);
    KV("Engine", buffer);
    EndGroup();
}

void ChassisPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    char buffer[64];
    PanelHeader(
        "Chassis",
        "Place ballast and set the rear ride height.",
        nullptr,
        18.0F);

    BeginGroup("##ballast");
    ui::SectionLabel("BALLAST");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##ballast",
        "Ballast",
        &vehicle.ballast_kg,
        0.0F,
        20.0F,
        "%.0f kg",
        "0 kg",
        "20 kg",
        false,
        1.0F);
    ui::Space(8.0F);
    ui::SetupSlider(
        "##bpos",
        "Ballast position",
        &vehicle.ballast_front_pct,
        30.0F,
        70.0F,
        "%.0f%% front",
        "Rear",
        "Front",
        false,
        1.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##ride");
    ui::SectionLabel("RIDE HEIGHT");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##rrh",
        "Rear ride height",
        &vehicle.rear_ride_height_mm,
        60.0F,
        100.0F,
        "%.0f mm",
        "60 mm",
        "100 mm",
        false,
        1.0F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##chsummary", true);
    SummaryTitle("CURRENT CONFIGURATION");
    ui::Space(8.0F);
    const float mass = vehicle.TotalMassKg();
    std::snprintf(buffer, sizeof(buffer), "%.0f kg", mass);
    KV("Car mass", buffer);
    ui::Space(8.0F);
    // Without ballast the car is 45.5% front; ballast shifts that toward
    // where it sits.
    const float front =
        ((mass - vehicle.ballast_kg) * 0.455F +
         vehicle.ballast_kg * vehicle.ballast_front_pct / 100.0F) /
        mass;
    std::snprintf(buffer, sizeof(buffer), "%.1f%% front", front * 100.0F);
    KV("Weight distribution", buffer);
    ui::Space(8.0F);
    const float rake =
        std::atan(
            (vehicle.rear_ride_height_mm - vehicle.front_ride_height_mm) /
            (vehicle.wheelbase_m * 1000.0F)) *
        57.2958F;
    std::snprintf(buffer, sizeof(buffer), "%.2f\xC2\xB0", rake);
    KV("Rake", buffer, rake > 1.0F);
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%.2f m / %.2f m",
        vehicle.wheelbase_m,
        vehicle.cg_height_m);
    KV("Wheelbase / CG height", buffer);
    EndGroup();
}

void BrakesPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    static const char* const kCooling[] = {"Closed", "Standard", "Open"};
    int cooling = IndexOf(kCooling, 3, vehicle.brake_cooling, 1);
    char buffer[64];
    PanelHeader(
        "Brakes",
        "Balance braking front to rear and size the cooling ducts.",
        nullptr,
        18.0F);

    BeginGroup("##bias");
    ui::SectionLabel("BRAKE BIAS");
    ui::Space(8.0F);
    ui::SetupSlider(
        "##bias",
        "Front bias",
        &vehicle.brake_bias_front_pct,
        50.0F,
        65.0F,
        "%.1f%%",
        "50% (rear)",
        "65% (front)",
        false,
        0.5F);
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##cooling");
    ui::SectionLabel("COOLING DUCTS");
    ui::Space(8.0F);
    if (PillChoice("##cool", kCooling, 3, &cooling)) {
        vehicle.brake_cooling = kCooling[cooling];
    }
    EndGroup();
    ui::Space(9.0F);

    BeginGroup("##brsummary", true);
    SummaryTitle("PROJECTED BRAKING");
    ui::Space(8.0F);
    std::snprintf(buffer, sizeof(buffer), "%.1f g", vehicle.brakes_max_decel_g);
    KV("Max deceleration", buffer);
    ui::Space(8.0F);
    const char* balance = vehicle.brake_bias_front_pct > 60.0F
        ? "Front lock-up risk"
        : vehicle.brake_bias_front_pct < 53.0F ? "Rear unstable"
                                               : "Balanced";
    KV(
        "Balance",
        balance,
        vehicle.brake_bias_front_pct > 60.0F ||
            vehicle.brake_bias_front_pct < 53.0F);
    ui::Space(8.0F);
    static const char* const kTemp[] = {
        "High, fast warm-up",
        "In window",
        "Low, slow warm-up",
    };
    KV("Brake temperature", kTemp[cooling], cooling != 1);
    ui::Space(8.0F);
    static const char* const kDrag[] = {"Lowest", "Low", "Higher"};
    KV("Duct drag", kDrag[cooling], cooling == 2);
    ui::Space(8.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s, %.0f kg",
        vehicle.brakes_name.c_str(),
        vehicle.brakes_mass_kg);
    KV("Brakes", buffer);
    EndGroup();
}

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

void DrawCarDetail(App& app) {
    switch (app.car_panel) {
        case CarPanel::Aerodynamics:
            AerodynamicsPanel(app);
            break;
        case CarPanel::Tires:
            TiresPanel(app);
            break;
        case CarPanel::Powertrain:
            PowertrainPanel(app);
            break;
        case CarPanel::Chassis:
            ChassisPanel(app);
            break;
        case CarPanel::Brakes:
            BrakesPanel(app);
            break;
        case CarPanel::Count:
            break;
    }
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

void StepCar(App& app, int direction) {
    const int count = static_cast<int>(CarPanel::Count);
    int index = static_cast<int>(app.car_panel) + direction;
    index = std::clamp(index, 0, count - 1);
    app.car_panel = static_cast<CarPanel>(index);
    app.track_focus = false;
}

void StepTrack(App& app, int direction) {
    const int count = static_cast<int>(TrackPanel::Count);
    int index = static_cast<int>(app.track_panel) + direction;
    index = std::clamp(index, 0, count - 1);
    app.track_panel = static_cast<TrackPanel>(index);
    app.track_focus = true;
}

float PillRowWidth(const char* left_label, const char* right_label) {
    const float left_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, left_label).x + 24.0F;
    const float right_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, right_label).x + 24.0F;
    return left_width + 8.0F + right_width;
}

void PlaceBottomStepRow(
    float column_width,
    float column_height,
    float row_width,
    float* out_x,
    float* out_y) {
    constexpr float kRowHeight = 29.0F;
    const ImVec2 window_pos = ImGui::GetWindowPos();
    *out_x = window_pos.x + (column_width - row_width) * 0.5F;
    *out_y = std::max(
        ImGui::GetCursorScreenPos().y + 8.0F,
        window_pos.y + column_height - 20.0F - kRowHeight);
}

void DrawCarCategoryFooter(App& app, float column_width, float column_height) {
    const float step_width = PillRowWidth("PREVIOUS", "NEXT");
    float x = 0.0F;
    float y = 0.0F;
    PlaceBottomStepRow(column_width, column_height, step_width, &x, &y);
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    ImGui::PushID("car_steps");
    if (ui::Pill("##prev", "PREVIOUS", false)) {
        StepCar(app, -1);
    }
    ImGui::SameLine(0.0F, 8.0F);
    if (ui::Pill("##next", "NEXT", false)) {
        StepCar(app, +1);
    }
    ImGui::PopID();
}

void DrawTrackCategoryFooter(App& app, float column_width, float column_height) {
    const float step_width = PillRowWidth("PREVIOUS", "NEXT");
    float x = 0.0F;
    float y = 0.0F;
    PlaceBottomStepRow(column_width, column_height, step_width, &x, &y);
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    ImGui::PushID("track_steps");
    if (ui::Pill("##prev", "PREVIOUS", false)) {
        StepTrack(app, -1);
    }
    ImGui::SameLine(0.0F, 8.0F);
    if (ui::Pill("##next", "NEXT", false)) {
        StepTrack(app, +1);
    }
    ImGui::PopID();
}

}  // namespace

void DrawSetupScreen(App& app) {
    DrawTopNav(app, app.vehicle.name.c_str());

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
        "Car & Track Setup");
    ui::TextAt(
        draw_list,
        ImVec2(left, origin.y + 118.5F),
        Face::PlexRegular,
        10.0F,
        Hex(0x737A84),
        "Configure the vehicle, select a circuit, then run the simulation.");

    constexpr float kRunWidth = 206.0F;
    constexpr float kHeaderPillGap = 8.0F;
    const float reset_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "RESET").x + 24.0F;
    const float save_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "SAVE SETUP").x + 24.0F;
    const float header_actions_width =
        reset_width + kHeaderPillGap + save_width + kHeaderPillGap + kRunWidth;

    ImGui::SetCursorScreenPos(
        ImVec2(right - header_actions_width, origin.y + 93.5F));
    if (ui::Pill("##reset", "RESET", false)) {
        app.vehicle = app.saved_vehicle;
        app.ShowToast("Setup reset to the last saved version.");
    }
    ImGui::SameLine(0.0F, kHeaderPillGap);
    if (ui::Pill("##save", "SAVE SETUP", false)) {
        app.SaveSetup();
    }

    ImGui::SetCursorScreenPos(ImVec2(right - kRunWidth, origin.y + 85.0F));
    if (ui::BigRedButton("##run", "RUN SIMULATION", ImVec2(kRunWidth, 46.0F))) {
        app.GoTo(Screen::Results);
        app.ShowToast(
            "The physics engine isn't connected yet, so this shows sample Run #42.",
            4.0);
    }

    constexpr float kDesign[4] = {225.0F, 455.0F, 225.0F, 431.0F};
    constexpr float kDesignTotal = 1336.0F;
    const float available = window_size.x - 48.0F - 48.0F;
    const float top = origin.y + 154.0F;
    const float height = std::max(300.0F, window_size.y - 154.0F - 32.0F);

    float x = left;
    float widths[4];
    for (int i = 0; i < 4; ++i) {
        widths[i] = std::floor(kDesign[i] * available / kDesignTotal);
    }

    BeginPanel("##carcats", ImVec2(x, top), ImVec2(widths[0], height), ImVec2(15, 15));
    CategoryColumn(app, false, ImGui::GetContentRegionAvail().x);
    DrawCarCategoryFooter(app, widths[0], height);
    EndPanel();
    x += widths[0] + 16.0F;

    BeginPanel("##cardetail", ImVec2(x, top), ImVec2(widths[1], height), ImVec2(17, 15));
    DrawCarDetail(app);
    EndPanel();
    x += widths[1] + 16.0F;

    BeginPanel("##trackcats", ImVec2(x, top), ImVec2(widths[2], height), ImVec2(15, 15));
    CategoryColumn(app, true, ImGui::GetContentRegionAvail().x);
    DrawTrackCategoryFooter(app, widths[2], height);
    EndPanel();
    x += widths[2] + 16.0F;

    BeginPanel(
        "##trackdetail",
        ImVec2(x, top),
        ImVec2(right - x, height),
        ImVec2(17, 15));
    DrawTrackDetail(app);
    EndPanel();
}
