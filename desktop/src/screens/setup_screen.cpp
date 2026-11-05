// Car & Track Setup migration from ApexLabsDevSpace commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0 (Figma 1440 x 900).
//
// Current slice: shell plus all five car setup panels. Track panels stay as
// placeholders until later slices.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "app.h"
#include "imgui.h"
#include "theme.h"
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
        ImVec2(12.0F, 12.0F));
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
    ui::Space(13.0F);
    ui::Divider(setup::kDivider);
    ui::Space(13.0F);
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
    ui::Space(11.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        Hex(0x6E7580),
        track ? "Choose the circuit conditions."
              : "Choose a vehicle system to configure.");
    ui::Space(11.0F);
    ui::Divider(setup::kDivider);
    ui::Space(11.0F);

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
        ui::Space(11.0F);
    }
}

void DetailPlaceholder(const char* title, const char* message) {
    ui::Text(Face::SpaceBold, 18.0F, setup::kTitle, title);
    ui::Space(10.0F);
    ui::Text(Face::PlexRegular, 10.0F, setup::kMuted, message);
}

const char* TrackPanelTitle(TrackPanel panel) {
    const int index = static_cast<int>(panel);
    if (index >= 0 && index < static_cast<int>(TrackPanel::Count)) {
        return kTrackTitles[index];
    }
    return "Track Setup";
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
            ImGui::SameLine(0.0F, 8.0F);
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
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##rear");
    ui::SectionLabel("REAR / FLOOR");
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

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
    ui::Space(12.0F);
    for (int i = 0; i < TireCatalogCount(); ++i) {
        if (i) {
            ImGui::SameLine(0.0F, 8.0F);
        }
        ImGui::PushID(i);
        if (ui::Pill("##c", TireAt(i).short_name, vehicle.compound == i)) {
            vehicle.compound = i;
        }
        ImGui::PopID();
    }
    ui::Space(12.0F);
    KV("Estimated grip", tire.grip, std::strcmp(tire.grip, "High") == 0);
    ui::Space(12.0F);
    KV(
        "Expected durability",
        tire.durability,
        std::strcmp(tire.durability, "Low") == 0);
    ui::Space(12.0F);
    KV("Warm-up", tire.warm_up);
    EndGroup();
    ui::Space(13.0F);

    BeginGroup("##pressures");
    ui::SectionLabel("PRESSURES");
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##tiresummary", true);
    ui::Text(Face::PlexSemiBold, 9.0F, Hex(0x646B76), "PROJECTED TIRE BEHAVIOR");
    ui::Space(12.0F);
    KV("Peak grip window", tire.peak_window);
    ui::Space(12.0F);
    char life[32];
    std::snprintf(life, sizeof(life), "%d laps", tire.life_laps);
    KV("Estimated life", life);
    ui::Space(12.0F);
    KV(
        "Degradation",
        tire.degradation,
        std::strcmp(tire.degradation, "High") == 0);
    ui::Space(12.0F);
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
    ui::Space(12.0F);
    if (PillChoice("##maps", kMaps, 3, &map)) {
        vehicle.power_map = kMaps[map];
    }
    ui::Space(10.0F);
    ui::Text(
        Face::PlexRegular,
        9.0F,
        setup::kMuted,
        map == 0 ? "Full power for one lap. Not allowed for long runs."
        : map == 1 ? "Default race mode, balanced against fuel use."
                   : "Lower power to save fuel and engine life.");
    EndGroup();
    ui::Space(13.0F);

    BeginGroup("##gearing");
    ui::SectionLabel("GEARING");
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##ptsummary", true);
    SummaryTitle("PROJECTED OUTPUT");
    ui::Space(12.0F);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%.0f kW",
        vehicle.engine_peak_kw * kPower[map]);
    KV("Peak power", buffer);
    ui::Space(12.0F);
    // Speed at the rev limit in top gear scales with 1 / final drive
    // (340 km/h at 3.20), matching DevSpace setup_screen.cpp.
    const float limit_kmh = 340.0F * 3.2F / vehicle.final_drive *
        (vehicle.shift_rpm / vehicle.engine_max_rpm);
    std::snprintf(buffer, sizeof(buffer), "%.0f km/h", limit_kmh);
    constexpr float kSampleTopSpeedKmh = 317.0F;
    KV("Top-gear speed limit", buffer, limit_kmh < kSampleTopSpeedKmh + 5.0F);
    ui::Space(12.0F);
    KV("Fuel use", kFuel[map], map == 0);
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##ride");
    ui::SectionLabel("RIDE HEIGHT");
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##chsummary", true);
    SummaryTitle("CURRENT CONFIGURATION");
    ui::Space(12.0F);
    const float mass = vehicle.TotalMassKg();
    std::snprintf(buffer, sizeof(buffer), "%.0f kg", mass);
    KV("Car mass", buffer);
    ui::Space(12.0F);
    // Without ballast the car is 45.5% front; ballast shifts that toward
    // where it sits.
    const float front =
        ((mass - vehicle.ballast_kg) * 0.455F +
         vehicle.ballast_kg * vehicle.ballast_front_pct / 100.0F) /
        mass;
    std::snprintf(buffer, sizeof(buffer), "%.1f%% front", front * 100.0F);
    KV("Weight distribution", buffer);
    ui::Space(12.0F);
    const float rake =
        std::atan(
            (vehicle.rear_ride_height_mm - vehicle.front_ride_height_mm) /
            (vehicle.wheelbase_m * 1000.0F)) *
        57.2958F;
    std::snprintf(buffer, sizeof(buffer), "%.2f\xC2\xB0", rake);
    KV("Rake", buffer, rake > 1.0F);
    ui::Space(12.0F);
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
    ui::Space(12.0F);
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
    ui::Space(13.0F);

    BeginGroup("##cooling");
    ui::SectionLabel("COOLING DUCTS");
    ui::Space(12.0F);
    if (PillChoice("##cool", kCooling, 3, &cooling)) {
        vehicle.brake_cooling = kCooling[cooling];
    }
    EndGroup();
    ui::Space(13.0F);

    BeginGroup("##brsummary", true);
    SummaryTitle("PROJECTED BRAKING");
    ui::Space(12.0F);
    std::snprintf(buffer, sizeof(buffer), "%.1f g", vehicle.brakes_max_decel_g);
    KV("Max deceleration", buffer);
    ui::Space(12.0F);
    const char* balance = vehicle.brake_bias_front_pct > 60.0F
        ? "Front lock-up risk"
        : vehicle.brake_bias_front_pct < 53.0F ? "Rear unstable"
                                               : "Balanced";
    KV(
        "Balance",
        balance,
        vehicle.brake_bias_front_pct > 60.0F ||
            vehicle.brake_bias_front_pct < 53.0F);
    ui::Space(12.0F);
    static const char* const kTemp[] = {
        "High, fast warm-up",
        "In window",
        "Low, slow warm-up",
    };
    KV("Brake temperature", kTemp[cooling], cooling != 1);
    ui::Space(12.0F);
    static const char* const kDrag[] = {"Lowest", "Low", "Higher"};
    KV("Duct drag", kDrag[cooling], cooling == 2);
    ui::Space(12.0F);
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
    EndPanel();
    x += widths[0] + 16.0F;

    BeginPanel("##cardetail", ImVec2(x, top), ImVec2(widths[1], height), ImVec2(17, 15));
    DrawCarDetail(app);
    EndPanel();
    x += widths[1] + 16.0F;

    BeginPanel("##trackcats", ImVec2(x, top), ImVec2(widths[2], height), ImVec2(15, 15));
    CategoryColumn(app, true, ImGui::GetContentRegionAvail().x);
    EndPanel();
    x += widths[2] + 16.0F;

    BeginPanel(
        "##trackdetail",
        ImVec2(x, top),
        ImVec2(right - x, height),
        ImVec2(17, 15));
    DetailPlaceholder(
        TrackPanelTitle(app.track_panel),
        "This panel will host the selected track setup controls.");
    EndPanel();
}
