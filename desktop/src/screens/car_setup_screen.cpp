// Car Setup screen: vehicle configuration panels.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "app.h"
#include "imgui.h"
#include "setup_common.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;
using namespace setup_ui;

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

void AerodynamicsPanel(App& app) {
    Vehicle& vehicle = app.vehicle;
    app.vehicle_viewport.SetFocus(ViewportFocus::FullCar);
    PanelHeader(
        "Aerodynamics",
        "Tune the car's high-speed efficiency and balance.",
        vehicle.AeroBias(),
        18.0F);

    BeginGroup("##front");
    ui::SectionLabel("FRONT AERO");
    ui::Space(8.0F);
    bool front_wing_focus = false;
    ui::SetupSlider(
        "##fw",
        "Front wing angle",
        &vehicle.front_wing_deg,
        0.0F,
        24.0F,
        "%.1f\xC2\xB0",
        "0\xC2\xB0",
        "24\xC2\xB0",
        false,
        0.5F,
        &front_wing_focus);
    if (front_wing_focus) {
        app.vehicle_viewport.SetFocus(ViewportFocus::FrontWing);
    }
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
    bool rear_wing_focus = false;
    ui::SetupSlider(
        "##rw",
        "Rear wing angle",
        &vehicle.rear_wing_deg,
        0.0F,
        32.0F,
        "%.1f\xC2\xB0",
        "0\xC2\xB0",
        "32\xC2\xB0",
        false,
        0.5F,
        &rear_wing_focus);
    if (rear_wing_focus) {
        app.vehicle_viewport.SetFocus(ViewportFocus::RearWing);
    }
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

void DrawCarDetail(App& app) {
    if (app.car_panel != CarPanel::Aerodynamics) {
        app.vehicle_viewport.SetFocus(ViewportFocus::FullCar);
    }
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

void StepCar(App& app, int direction) {
    const int count = static_cast<int>(CarPanel::Count);
    int index = static_cast<int>(app.car_panel) + direction;
    index = std::clamp(index, 0, count - 1);
    app.car_panel = static_cast<CarPanel>(index);
    app.track_focus = false;
}

void DrawCarCategoryFooter(App& app, float column_width, float column_height) {
    bool prev = false;
    bool next = false;
    DrawPrevNextFooter(column_width, column_height, "car_steps", &prev, &next);
    if (prev) {
        StepCar(app, -1);
    }
    if (next) {
        StepCar(app, +1);
    }
}

}  // namespace

void DrawCarSetupScreen(App& app) {
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
        "Car Setup");
    ui::TextAt(
        draw_list,
        ImVec2(left, origin.y + 118.5F),
        Face::PlexRegular,
        10.0F,
        Hex(0x737A84),
        "Configure the vehicle, then continue to track configuration.");

    constexpr float kRunWidth = 206.0F;
    constexpr float kButtonGap = 10.0F;
    constexpr float kHeaderPillGap = 8.0F;
    const char* track_label = "Move to Track Configuration";
    const float track_width = std::max(
        280.0F,
        ui::TextSize(Face::PlexSemiBold, 16.0F, track_label).x + 36.0F);
    const float reset_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "RESET").x + 24.0F;
    const float save_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, "SAVE SETUP").x + 24.0F;
    const float header_actions_width = reset_width + kHeaderPillGap +
        save_width + kHeaderPillGap + track_width + kButtonGap + kRunWidth;

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

    ImGui::SetCursorScreenPos(
        ImVec2(right - kRunWidth - kButtonGap - track_width, origin.y + 85.0F));
    if (ui::BigOutlineButton(
            "##totrack",
            track_label,
            ImVec2(track_width, 46.0F))) {
        app.track_focus = true;
        app.GoTo(Screen::TrackSetup);
    }

    ImGui::SetCursorScreenPos(ImVec2(right - kRunWidth, origin.y + 85.0F));
    if (ui::BigRedButton("##run", "RUN SIMULATION", ImVec2(kRunWidth, 46.0F))) {
        RunSimulationAction(app);
    }

    const float available = window_size.x - 48.0F - 48.0F;
    const float top = origin.y + 154.0F;
    const float height = std::max(300.0F, window_size.y - 154.0F - 32.0F);
    const float cat_width = SetupColumnWidth(210.0F, available);
    const float detail_width = SetupColumnWidth(390.0F, available);
    const float viewport_width = std::max(
        280.0F,
        available - cat_width - detail_width - 32.0F);

    float x = left;
    BeginPanel(
        "##carcats",
        ImVec2(x, top),
        ImVec2(cat_width, height),
        ImVec2(15, 15));
    int clicked = -1;
    CategoryColumn(
        "Car Setup",
        "Choose a vehicle system to configure.",
        kCarTitles,
        kCarSubtitles,
        static_cast<int>(CarPanel::Count),
        static_cast<int>(app.car_panel),
        ImGui::GetContentRegionAvail().x,
        &clicked);
    if (clicked >= 0) {
        app.car_panel = static_cast<CarPanel>(clicked);
        app.track_focus = false;
    }
    DrawCarCategoryFooter(app, cat_width, height);
    EndPanel();
    x += cat_width + 16.0F;

    BeginPanel(
        "##cardetail",
        ImVec2(x, top),
        ImVec2(detail_width, height),
        ImVec2(17, 15));
    DrawCarDetail(app);
    EndPanel();
    x += detail_width + 16.0F;

    BeginPanel(
        "##carviewport",
        ImVec2(x, top),
        ImVec2(viewport_width, height),
        ImVec2(14, 14));
    ui::SectionLabel("VEHICLE PREVIEW");
    ui::Space(6.0F);
    {
        const ImVec2 status_pos = ImGui::GetCursorScreenPos();
        ui::TextAt(
            ImGui::GetWindowDrawList(),
            status_pos,
            Face::PlexRegular,
            11.0F,
            Hex(0x8B93A0),
            app.vehicle_viewport.status.c_str());
        ImGui::Dummy(ImVec2(0.0F, 16.0F));
    }
    ui::Space(10.0F);

    const ImVec2 preview_pos = ImGui::GetCursorScreenPos();
    const float preview_w = ImGui::GetContentRegionAvail().x;
    const float preview_h = std::max(180.0F, ImGui::GetContentRegionAvail().y - 8.0F);
    const float dpi = std::max(1.0F, ImGui::GetIO().DisplayFramebufferScale.x);
    app.viewport_pixel_w = std::max(1, static_cast<int>(preview_w * dpi));
    app.viewport_pixel_h = std::max(1, static_cast<int>(preview_h * dpi));
    app.vehicle_viewport.EnsureSize(app.viewport_pixel_w, app.viewport_pixel_h);

    ImDrawList* panel_draw = ImGui::GetWindowDrawList();
    panel_draw->AddRectFilled(
        preview_pos,
        ImVec2(preview_pos.x + preview_w, preview_pos.y + preview_h),
        Hex(0x12141A),
        10.0F);

    if (app.vehicle_viewport.HasTexture()) {
        panel_draw->AddImageRounded(
            app.vehicle_viewport.TextureId(),
            preview_pos,
            ImVec2(preview_pos.x + preview_w, preview_pos.y + preview_h),
            ImVec2(0, 0),
            ImVec2(1, 1),
            IM_COL32_WHITE,
            10.0F);
    }

    ImGui::SetCursorScreenPos(preview_pos);
    ImGui::InvisibleButton("##orbit", ImVec2(preview_w, preview_h));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        app.vehicle_viewport.Orbit(delta.x, delta.y);
    }
    if (ImGui::IsItemHovered()) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0F) {
            app.vehicle_viewport.Zoom(wheel);
        }
    }

    ui::TextAt(
        panel_draw,
        ImVec2(preview_pos.x + 12.0F, preview_pos.y + preview_h - 22.0F),
        Face::PlexRegular,
        10.0F,
        Hex(0x8B93A0),
        "Drag to orbit  ·  Scroll to zoom");
    EndPanel();
}
