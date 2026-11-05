// Car & Track Setup migration from ApexLabsDevSpace commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0 (Figma 1440 x 900).
//
// Current slice: shell plus the Aerodynamics car panel. Remaining car/track
// panels stay as placeholders until later slices.
#include <algorithm>
#include <cmath>
#include <cstdio>

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
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
}

void EndPanel() {
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

const char* CarPanelTitle(CarPanel panel) {
    const int index = static_cast<int>(panel);
    if (index >= 0 && index < static_cast<int>(CarPanel::Count)) {
        return kCarTitles[index];
    }
    return "Car Setup";
}

const char* TrackPanelTitle(TrackPanel panel) {
    const int index = static_cast<int>(panel);
    if (index >= 0 && index < static_cast<int>(TrackPanel::Count)) {
        return kTrackTitles[index];
    }
    return "Track Setup";
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

void DrawCarDetail(App& app) {
    switch (app.car_panel) {
        case CarPanel::Aerodynamics:
            AerodynamicsPanel(app);
            break;
        case CarPanel::Tires:
        case CarPanel::Powertrain:
        case CarPanel::Chassis:
        case CarPanel::Brakes:
            DetailPlaceholder(
                CarPanelTitle(app.car_panel),
                "This panel will host the selected car setup controls.");
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
