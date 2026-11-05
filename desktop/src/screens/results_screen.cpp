// Results overview shell (Figma: "Results – Overview").
// Slice 1: header, SAMPLE badge, action stubs, and KPI row from sample Run #42.
// Track map, section table, telemetry chart, and section popup follow in later
// slices. Adapted from ApexLabsDevSpace results_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

enum class DeltaClass { Gained, Neutral, Lost };

DeltaClass Classify(float delta, float band) {
    // Round to the displayed precision first so "−0.05 s" with a 0.05 s band
    // counts as gained, matching the legend wording ("0.05 s or more").
    const float rounded = std::round(delta * 100.0F) / 100.0F;
    if (rounded <= -band + 1e-4F) {
        return DeltaClass::Gained;
    }
    if (rounded >= band - 1e-4F) {
        return DeltaClass::Lost;
    }
    return DeltaClass::Neutral;
}

ImU32 DeltaColor(float delta, float band) {
    switch (Classify(delta, band)) {
        case DeltaClass::Gained:
            return app::kGained;
        case DeltaClass::Neutral:
            return app::kNeutral;
        case DeltaClass::Lost:
            return app::kLost;
    }
    return app::kMuted;
}

void BeginContentCard(const char* id, float width, ImVec2 pad = ImVec2(20, 20)) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12.0F, pad, width);
}

void Kpi(
    const char* id,
    const char* label,
    const char* value,
    const char* delta,
    ImU32 delta_color,
    const char* note,
    float width) {
    BeginContentCard(id, width, ImVec2(18, 16));
    ui::Text(Face::PlexSemiBold, 11.0F, app::kMuted, label);
    ui::Space(6.0F);
    ui::Text(Face::SpaceBold, 28.0F, app::kText, value);
    ui::Space(6.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (delta) {
        ui::StatusDot(
            draw_list, ImVec2(position.x + 4.0F, position.y + 7.5F), 4.0F, delta_color);
        ui::TextAt(
            draw_list,
            ImVec2(position.x + 14.0F, position.y),
            Face::PlexSemiBold,
            12.0F,
            delta_color,
            delta);
        const float delta_width =
            ui::TextSize(Face::PlexSemiBold, 12.0F, delta).x;
        ui::TextAt(
            draw_list,
            ImVec2(position.x + 14.0F + delta_width + 6.0F, position.y),
            Face::PlexRegular,
            12.0F,
            app::kMuted,
            "vs reference");
    } else if (note) {
        const float available = ImGui::GetContentRegionAvail().x;
        std::string text = note;
        if (ui::TextSize(Face::PlexRegular, 12.0F, note).x > available) {
            while (!text.empty() &&
                   ui::TextSize(
                       Face::PlexRegular,
                       12.0F,
                       (text + "\xE2\x80\xA6").c_str())
                           .x > available) {
                text.pop_back();
            }
            while (!text.empty() && text.back() == ' ') {
                text.pop_back();
            }
            text += "\xE2\x80\xA6";
        }
        ui::TextAt(
            draw_list,
            position,
            Face::PlexRegular,
            12.0F,
            app::kMuted,
            text.c_str());
        if (text != note &&
            ImGui::IsMouseHoveringRect(
                position, ImVec2(position.x + available, position.y + 16.0F))) {
            ImGui::SetTooltip("%s", note);
        }
    }
    ImGui::Dummy(ImVec2(1.0F, LineHeight(Face::PlexRegular, 12.0F)));
    ui::EndCard();
}

void PlaceholderPanel(
    const char* id,
    const char* title,
    const char* subtitle,
    float width,
    float height) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12.0F, ImVec2(20, 20), width);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ui::TextAt(draw_list, origin, Face::PlexSemiBold, 18.0F, app::kText, title);
    ui::TextAt(
        draw_list,
        ImVec2(origin.x, origin.y + 28.0F),
        Face::PlexRegular,
        12.0F,
        app::kMuted,
        subtitle);
    ImGui::Dummy(ImVec2(width - 40.0F, std::max(40.0F, height - 40.0F)));
    ui::EndCard();
}

}  // namespace

void DrawResultsScreen(App& app) {
    const RunResult& run = app.run;
    char nav_label[96];
    std::snprintf(
        nav_label,
        sizeof(nav_label),
        "%s  \xC2\xB7  Run #%d",
        run.track_name.c_str(),
        run.run_id);
    DrawTopNav(app, nav_label);
    app.hover_section = -1;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 24));
    ImGui::BeginChild(
        "##results", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float width = ImGui::GetContentRegionAvail().x;
    const float band = app.prefs.neutral_band_s;

    // Page header
    {
        const ImVec2 position = ImGui::GetCursorScreenPos();
        ui::TextAt(
            draw_list, position, Face::SpaceBold, 28.0F, app::kText, "Results");
        if (run.sample_data) {
            const float title_width =
                ui::TextSize(Face::SpaceBold, 28.0F, "Results").x;
            const float badge_x = position.x + title_width + 12.0F;
            const ImVec2 badge_size =
                ui::TextSize(Face::PlexSemiBold, 10.0F, "SAMPLE DATA");
            const ImVec2 badge_min(badge_x, position.y + 7.0F);
            const ImVec2 badge_max(
                badge_x + badge_size.x + 18.0F,
                position.y + 7.0F + badge_size.y + 6.0F);
            draw_list->AddRect(badge_min, badge_max, app::kNeutral, 4.0F);
            ui::TextAt(
                draw_list,
                ImVec2(badge_min.x + 9.0F, badge_min.y + 3.0F),
                Face::PlexSemiBold,
                10.0F,
                app::kNeutral,
                "SAMPLE DATA");
        }

        char subtitle[256];
        std::snprintf(
            subtitle,
            sizeof(subtitle),
            "%s  \xC2\xB7  %s  \xC2\xB7  %s, air %s / track %s  \xC2\xB7  "
            "Engine v%s  \xC2\xB7  Compared with reference Run #%d",
            run.vehicle_name.c_str(),
            run.track_name.c_str(),
            run.weather_preset.c_str(),
            FormatTemp(run.air_c).c_str(),
            FormatTemp(run.track_c).c_str(),
            run.engine_version.c_str(),
            run.reference_run_id);
        ui::TextAt(
            draw_list,
            ImVec2(position.x, position.y + 40.0F),
            Face::PlexRegular,
            13.0F,
            app::kMuted,
            subtitle);

        const float rerun_width =
            ui::TextSize(Face::PlexSemiBold, 14.0F, "Re-run simulation").x +
            32.0F;
        const float export_width =
            ui::TextSize(Face::PlexSemiBold, 14.0F, "Export JSON").x + 34.0F;
        const float compare_width =
            ui::TextSize(Face::PlexSemiBold, 14.0F, "Compare runs").x + 34.0F;
        const float actions_x =
            position.x + width - rerun_width - 10.0F - export_width - 10.0F -
            compare_width;

        ImGui::SetCursorScreenPos(ImVec2(actions_x, position.y + 8.5F));
        if (ui::Button("##compare", "Compare runs", ui::ButtonKind::Outline)) {
            app.compare_a = run.run_id;
            app.compare_b = run.reference_run_id;
            app.GoTo(Screen::Compare);
        }
        ImGui::SetCursorScreenPos(
            ImVec2(actions_x + compare_width + 10.0F, position.y + 8.5F));
        if (ui::Button("##export", "Export JSON", ui::ButtonKind::Outline)) {
            app.ShowToast("JSON export lands with Import / Export.");
        }
        ImGui::SetCursorScreenPos(ImVec2(
            actions_x + compare_width + 10.0F + export_width + 10.0F,
            position.y + 9.5F));
        if (ui::Button(
                "##rerun", "Re-run simulation", ui::ButtonKind::Primary)) {
            app.ShowToast("The physics engine isn't connected yet.");
        }
        ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + 56.0F));
        ImGui::Dummy(ImVec2(width, 0.0F));
    }
    ui::Space(20.0F);

    // KPI row
    {
        const float gap = 16.0F;
        const float kpi_width = (width - gap * 4.0F) / 5.0F;
        const ImVec2 position = ImGui::GetCursorScreenPos();
        const std::string lap = FormatLapTime(run.lap_time_s);
        const std::string lap_delta = FormatDelta(run.lap_delta_s);
        Kpi(
            "##k0",
            "LAP TIME",
            lap.c_str(),
            lap_delta.c_str(),
            DeltaColor(run.lap_delta_s, band),
            nullptr,
            kpi_width);
        ImGui::SetCursorScreenPos(ImVec2(position.x + (kpi_width + gap), position.y));
        const std::string top = FormatSpeed(run.top_speed_kmh);
        Kpi(
            "##k1",
            "TOP SPEED",
            top.c_str(),
            nullptr,
            0,
            run.top_speed_note.c_str(),
            kpi_width);
        for (int sector = 0; sector < 3; ++sector) {
            ImGui::SetCursorScreenPos(ImVec2(
                position.x + (kpi_width + gap) * static_cast<float>(2 + sector),
                position.y));
            char label[16];
            char value[16];
            char id[8];
            std::snprintf(label, sizeof(label), "SECTOR %d", sector + 1);
            std::snprintf(
                value, sizeof(value), "%.3f", run.sector_times_s[sector]);
            std::snprintf(id, sizeof(id), "##k%d", sector + 2);
            const std::string delta = FormatDelta(run.sector_deltas_s[sector]);
            Kpi(
                id,
                label,
                value,
                delta.c_str(),
                DeltaColor(run.sector_deltas_s[sector], band),
                nullptr,
                kpi_width);
        }
    }
    ui::Space(20.0F);

    // Placeholder main row until map/table slices land.
    {
        const float table_width = 420.0F;
        const float gap = 20.0F;
        const float map_width = width - table_width - gap;
        const float card_height = 320.0F;
        const ImVec2 position = ImGui::GetCursorScreenPos();

        PlaceholderPanel(
            "##mapcard",
            "Track map",
            "Colored section map lands in the next Results slice.",
            map_width,
            card_height);
        ImGui::SetCursorScreenPos(
            ImVec2(position.x + map_width + gap, position.y));
        PlaceholderPanel(
            "##table",
            "Section times",
            "Sector table and section detail popup come next.",
            table_width,
            card_height);
        ImGui::SetCursorScreenPos(
            ImVec2(position.x, position.y + card_height));
        ImGui::Dummy(ImVec2(width, 0.0F));
    }
    ui::Space(20.0F);

    PlaceholderPanel(
        "##trace",
        "Speed along the lap",
        "Telemetry channels land after the map and section table.",
        width,
        180.0F);

    ImGui::EndChild();
    ImGui::PopStyleVar();
}
