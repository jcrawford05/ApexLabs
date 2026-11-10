// Settings (Figma: "Settings", 1440 x 1590).
// Adapted from ApexLabsDevSpace settings_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. Uses Space Grotesk / IBM Plex Sans.
//
// Live edits go into app.prefs; Save copies them to saved_prefs. CHANGED marks
// rows that differ from the last save. Unit and display prefs feed Results,
// Compare, and Setup immediately.
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

constexpr float kRowPadY = 14.0F;
constexpr float kRowGap = 24.0F;
ImVec2 g_row_end;
float g_row_w = 0.0F;

float SegmentedWidth(const char* const* items, int count, int selected) {
    float total = 3.0F + 3.0F + 2.0F;
    for (int i = 0; i < count; ++i) {
        const Face face =
            (i == selected) ? Face::PlexSemiBold : Face::SpaceMedium;
        total += ui::TextSize(face, 13.0F, items[i]).x + 28.0F +
            (i ? 2.0F : 0.0F);
    }
    return total;
}

float SegmentedHeight() {
    return LineHeight(Face::SpaceMedium, 13.0F) + 12.0F + 8.0F;
}

float ButtonWidth(const char* label, float font = 14.0F, float pad_x = 16.0F) {
    return ui::TextSize(Face::PlexSemiBold, font, label).x + pad_x * 2.0F;
}

float ButtonHeight(float font = 14.0F, float pad_y = 10.0F) {
    return LineHeight(Face::PlexSemiBold, font) + pad_y * 2.0F;
}

ImVec2 Row(
    const char* title,
    const char* description,
    bool changed,
    ImVec2 control_size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    draw_list->AddRectFilled(
        position,
        ImVec2(position.x + width, position.y + 1.0F),
        app::kCardBorder);

    const float label_w = width - control_size.x - kRowGap;
    float label_h = LineHeight(Face::PlexSemiBold, 14.0F);
    ui::TextAt(
        draw_list,
        ImVec2(position.x, position.y + kRowPadY),
        Face::PlexSemiBold,
        14.0F,
        app::kText,
        title);
    if (changed) {
        const float title_w = ui::TextSize(Face::PlexSemiBold, 14.0F, title).x;
        ImGui::SetCursorScreenPos(
            ImVec2(position.x + title_w + 8.0F, position.y + kRowPadY + 1.0F));
        ui::Badge("CHANGED", app::kNeutral);
    }
    if (description) {
        ImFont* font = Font(Face::PlexRegular, 12.0F);
        const float font_size = LineHeight(Face::PlexRegular, 12.0F);
        const float description_h =
            font->CalcTextSizeA(font_size, FLT_MAX, label_w, description).y;
        draw_list->AddText(
            font,
            font_size,
            ImVec2(position.x, position.y + kRowPadY + label_h + 3.0F),
            app::kMuted,
            description,
            nullptr,
            label_w);
        label_h += 3.0F + description_h;
    }
    const float height =
        std::max(label_h, control_size.y) + kRowPadY * 2.0F;
    const ImVec2 control(
        position.x + width - control_size.x,
        position.y + (height - control_size.y) * 0.5F);
    g_row_end = ImVec2(position.x, position.y + height);
    g_row_w = width;
    ImGui::SetCursorScreenPos(control);
    return control;
}

void EndRow() {
    ImGui::SetCursorScreenPos(g_row_end);
    ImGui::Dummy(ImVec2(g_row_w, 0.0F));
}

void GroupHeader(const char* title, const char* description) {
    ui::Text(Face::PlexSemiBold, 18.0F, app::kText, title);
    ui::Space(4.0F);
    ui::TextWrapped(Face::PlexRegular, 12.0F, app::kMuted, description);
    ui::Space(12.0F);
}

void BeginSettingsGroup(const char* id, float width) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12.0F, ImVec2(24.0F, 8.0F), width);
    ui::Space(12.0F);
}

void EndSettingsGroup() {
    ui::EndCard();
}

bool SliderEntry(
    const char* id,
    float* value,
    float min,
    float max,
    float step,
    int ticks,
    const char* fmt,
    const char* unit,
    float box_w) {
    ImGui::PushID(id);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool changed = false;
    ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + 10.0F));
    changed |= ui::TickSlider("##slider", value, min, max, ticks, 180.0F, step);
    ImGui::SetCursorScreenPos(ImVec2(position.x + 192.0F, position.y));
    changed |= ui::NumberInput("##num", value, fmt, unit, box_w, min, max);
    ImGui::PopID();
    return changed;
}

}  // namespace

void DrawSettingsScreen(App& app) {
    DrawTopNav(app, "Apex Labs  \xC2\xB7  v0.1.0");
    Preferences& prefs = app.prefs;
    const Preferences& saved = app.saved_prefs;
    static int active_group = 0;
    static int scroll_to = -1;
    static bool confirm_clear = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0F, 24.0F));
    ImGui::BeginChild(
        "##settings",
        ImVec2(0.0F, 0.0F),
        ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float width = ImGui::GetContentRegionAvail().x;
    const int unsaved = prefs.DifferencesFrom(saved);

    {
        const ImVec2 header = ImGui::GetCursorScreenPos();
        ui::TextAt(draw_list, header, Face::SpaceBold, 28.0F, app::kText, "Settings");
        ui::TextAt(
            draw_list,
            ImVec2(header.x, header.y + 40.0F),
            Face::PlexRegular,
            13.0F,
            app::kMuted,
            "App-wide defaults. Car-specific values live in Car Setup.");
        const float button_h = ButtonHeight();
        const float y = header.y + (56.0F - button_h) * 0.5F;
        float x = header.x + width - ButtonWidth("Save changes");
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Button("##save", "Save changes", ui::ButtonKind::Primary)) {
            app.saved_prefs = prefs;
            app.ShowToast(unsaved ? "Settings saved." : "No changes to save.");
        }
        x -= 10.0F + ButtonWidth("Reset to defaults");
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Button("##reset", "Reset to defaults", ui::ButtonKind::Outline)) {
            prefs = Preferences{};
            app.map_coloring = prefs.default_map_coloring;
            app.weather.preset =
                prefs.default_wet ? WeatherPreset::Wet : WeatherPreset::Dry;
            app.weather.air_c = prefs.default_air_c;
            app.weather.track_c = prefs.default_track_c;
            app.ShowToast("Defaults restored. Save to keep them.");
        }
        if (unsaved > 0) {
            char label[48];
            std::snprintf(
                label,
                sizeof(label),
                unsaved == 1 ? "%d unsaved change" : "%d unsaved changes",
                unsaved);
            const float label_w = ui::TextSize(Face::PlexSemiBold, 12.0F, label).x;
            x -= 10.0F + label_w;
            ui::TextAt(
                draw_list,
                ImVec2(x, y + button_h * 0.5F - 7.5F),
                Face::PlexSemiBold,
                12.0F,
                app::kNeutral,
                label);
            ui::StatusDot(
                draw_list,
                ImVec2(x - 10.0F, y + button_h * 0.5F),
                4.0F,
                app::kNeutral);
        }
        ImGui::SetCursorScreenPos(ImVec2(header.x, header.y + 56.0F));
        ImGui::Dummy(ImVec2(width, 0.0F));
    }
    ui::Space(20.0F);

    const ImVec2 body = ImGui::GetCursorScreenPos();
    const float nav_w = 240.0F;
    const float gap = 20.0F;
    const float groups_w = width - nav_w - gap;
    const float scroll_y = ImGui::GetScrollY();
    const float content_top = ImGui::GetWindowPos().y;
    float group_top[4] = {};

    ImGui::SetCursorScreenPos(ImVec2(body.x + nav_w + gap, body.y));
    ImGui::BeginGroup();
    {
        static const char* const kSteps[] = {"1 ms", "5 ms", "10 ms"};
        static const char* const kWeather[] = {"Dry", "Wet"};
        static const char* const kSpeeds[] = {"km/h", "mph", "m/s"};
        static const char* const kTemps[] = {"\xC2\xB0" "C", "\xC2\xB0" "F"};
        static const char* const kMapViews[] = {
            "\xCE\x94 vs reference",
            "Speed",
            "Throttle / brake",
        };

        group_top[0] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_sim", groups_w);
        GroupHeader(
            "Simulation",
            "How the physics engine runs a lap. Changing these makes new runs "
            "not directly comparable with old ones.");

        Row(
            "Regulation set",
            "Budget, minimum mass and power limits every car is checked against.",
            prefs.regulation != saved.regulation,
            ImVec2(240.0F, 38.0F));
        ui::Dropdown(
            "##reg",
            kRegulations,
            kRegulationCount,
            &prefs.regulation,
            240.0F);
        EndRow();

        Row(
            "Physics time step",
            "Smaller steps are more accurate but slower to simulate.",
            prefs.time_step != saved.time_step,
            ImVec2(SegmentedWidth(kSteps, 3, prefs.time_step), SegmentedHeight()));
        ui::Segmented("##step", kSteps, 3, &prefs.time_step);
        EndRow();

        Row(
            "Telemetry sample spacing",
            "Distance between saved telemetry points. 5 m is about 1,000 points "
            "per lap.",
            prefs.telemetry_spacing_m != saved.telemetry_spacing_m,
            ImVec2(192.0F + 64.0F, 36.0F));
        SliderEntry(
            "##spacing",
            &prefs.telemetry_spacing_m,
            1.0F,
            20.0F,
            1.0F,
            5,
            "%.0f",
            "m",
            64.0F);
        EndRow();

        int wet = prefs.default_wet ? 1 : 0;
        Row(
            "Default weather",
            "Used for new runs unless you change it before running.",
            prefs.default_wet != saved.default_wet,
            ImVec2(SegmentedWidth(kWeather, 2, wet), SegmentedHeight()));
        if (ui::Segmented("##weather", kWeather, 2, &wet)) {
            prefs.default_wet = wet == 1;
            app.weather.preset =
                prefs.default_wet ? WeatherPreset::Wet : WeatherPreset::Dry;
        }
        EndRow();

        {
            const bool fahrenheit = prefs.temp_unit == TempUnit::F;
            float air = ConvertTemp(prefs.temp_unit, prefs.default_air_c);
            float track = ConvertTemp(prefs.temp_unit, prefs.default_track_c);
            const char* air_unit =
                fahrenheit ? "\xC2\xB0" "F air" : "\xC2\xB0" "C air";
            const char* track_unit =
                fahrenheit ? "\xC2\xB0" "F track" : "\xC2\xB0" "C track";
            const float air_w =
                50.0F + ui::TextSize(Face::PlexRegular, 12.0F, air_unit).x;
            const float track_w =
                50.0F + ui::TextSize(Face::PlexRegular, 12.0F, track_unit).x;
            const ImVec2 control = Row(
                "Default air / track temperature",
                "Starting conditions for new runs.",
                prefs.default_air_c != saved.default_air_c ||
                    prefs.default_track_c != saved.default_track_c,
                ImVec2(air_w + 8.0F + track_w, 36.0F));
            if (ui::NumberInput(
                    "##air",
                    &air,
                    "%.0f",
                    air_unit,
                    air_w,
                    fahrenheit ? -4.0F : -20.0F,
                    fahrenheit ? 122.0F : 50.0F)) {
                prefs.default_air_c = fahrenheit
                    ? (air - 32.0F) * 5.0F / 9.0F
                    : air;
                app.weather.air_c = prefs.default_air_c;
            }
            ImGui::SetCursorScreenPos(ImVec2(control.x + air_w + 8.0F, control.y));
            if (ui::NumberInput(
                    "##track",
                    &track,
                    "%.0f",
                    track_unit,
                    track_w,
                    fahrenheit ? 14.0F : -10.0F,
                    fahrenheit ? 176.0F : 80.0F)) {
                prefs.default_track_c = fahrenheit
                    ? (track - 32.0F) * 5.0F / 9.0F
                    : track;
                app.weather.track_c = prefs.default_track_c;
            }
            EndRow();
        }

        Row(
            "Reference lap for comparisons",
            "What the Results page compares each run against.",
            prefs.reference_lap != saved.reference_lap,
            ImVec2(300.0F, 38.0F));
        ui::Dropdown(
            "##ref",
            kReferenceLaps,
            kReferenceLapCount,
            &prefs.reference_lap,
            300.0F);
        EndRow();
        EndSettingsGroup();
        ui::Space(20.0F);

        group_top[1] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_units", groups_w);
        GroupHeader("Units & display", "How numbers and the Results page look.");

        int speed = static_cast<int>(prefs.speed_unit);
        Row(
            "Speed",
            nullptr,
            prefs.speed_unit != saved.speed_unit,
            ImVec2(SegmentedWidth(kSpeeds, 3, speed), SegmentedHeight()));
        if (ui::Segmented("##speed", kSpeeds, 3, &speed)) {
            prefs.speed_unit = static_cast<SpeedUnit>(speed);
        }
        EndRow();

        int temp = static_cast<int>(prefs.temp_unit);
        Row(
            "Temperature",
            nullptr,
            prefs.temp_unit != saved.temp_unit,
            ImVec2(SegmentedWidth(kTemps, 2, temp), SegmentedHeight()));
        if (ui::Segmented("##temp", kTemps, 2, &temp)) {
            prefs.temp_unit = static_cast<TempUnit>(temp);
        }
        EndRow();

        int view = static_cast<int>(prefs.default_map_coloring);
        Row(
            "Track map colors by default",
            "The view the track map opens in.",
            prefs.default_map_coloring != saved.default_map_coloring,
            ImVec2(240.0F, 38.0F));
        if (ui::Dropdown("##mapview", kMapViews, 3, &view, 240.0F)) {
            prefs.default_map_coloring = static_cast<MapColoring>(view);
            app.map_coloring = prefs.default_map_coloring;
        }
        EndRow();

        Row(
            "Neutral band for section deltas",
            "Sections within this margin show as amber instead of green or red.",
            prefs.neutral_band_s != saved.neutral_band_s,
            ImVec2(192.0F + 72.0F, 36.0F));
        SliderEntry(
            "##band",
            &prefs.neutral_band_s,
            0.01F,
            0.20F,
            0.01F,
            5,
            "%.2f",
            "s",
            72.0F);
        EndRow();

        Row(
            "Show section numbers on track map",
            nullptr,
            prefs.show_section_numbers != saved.show_section_numbers,
            ImVec2(40.0F, 22.0F));
        ui::Toggle("##numbers", &prefs.show_section_numbers);
        EndRow();
        EndSettingsGroup();
        ui::Space(20.0F);

        group_top[2] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_data", groups_w);
        GroupHeader(
            "Data & storage",
            "Where saved cars and run history are kept.");
        {
            const char* path = prefs.database_path.c_str();
            const float path_w =
                ui::TextSize(Face::PlexSemiBold, 13.0F, path).x + 26.0F;
            const float button_w = ButtonWidth("Change\xE2\x80\xA6");
            const ImVec2 control = Row(
                "Database location",
                "Local database file used for saved cars and runs.",
                prefs.database_path != saved.database_path,
                ImVec2(path_w + 8.0F + button_w, 38.0F));
            draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
                control,
                ImVec2(control.x + path_w, control.y + 38.0F),
                app::kRaised,
                8.0F);
            draw_list->AddRect(
                control,
                ImVec2(control.x + path_w, control.y + 38.0F),
                app::kControlBorder,
                8.0F);
            ui::TextAt(
                draw_list,
                ImVec2(control.x + 13.0F, control.y + 11.0F),
                Face::PlexSemiBold,
                13.0F,
                app::kMuted,
                path);
            ImGui::SetCursorScreenPos(
                ImVec2(control.x + path_w + 8.0F, control.y));
            if (ui::Button(
                    "##change",
                    "Change\xE2\x80\xA6",
                    ui::ButtonKind::Outline)) {
                app.ShowToast(
                    "Choosing a database file needs the storage layer, which "
                    "isn't connected yet.");
            }
            EndRow();
        }

        Row(
            "Save runs automatically",
            "Store every completed run in run history.",
            prefs.autosave_runs != saved.autosave_runs,
            ImVec2(40.0F, 22.0F));
        ui::Toggle("##autosave", &prefs.autosave_runs);
        EndRow();

        Row(
            "Keep telemetry for",
            "Older runs keep their lap and sector times but drop detailed "
            "telemetry.",
            prefs.keep_telemetry != saved.keep_telemetry,
            ImVec2(180.0F, 38.0F));
        ui::Dropdown(
            "##keep",
            kKeepTelemetry,
            kKeepTelemetryCount,
            &prefs.keep_telemetry,
            180.0F);
        EndRow();

        Row(
            "Include telemetry in JSON exports",
            "Makes files larger, but lets others replay the full lap.",
            prefs.telemetry_in_exports != saved.telemetry_in_exports,
            ImVec2(40.0F, 22.0F));
        ui::Toggle("##exporttel", &prefs.telemetry_in_exports);
        EndRow();

        Row(
            "Clear run history",
            "Deletes all saved runs. Saved cars are kept. This cannot be undone.",
            false,
            ImVec2(ButtonWidth("Clear history\xE2\x80\xA6"), ButtonHeight()));
        if (ui::Button(
                "##clear",
                "Clear history\xE2\x80\xA6",
                ui::ButtonKind::Danger)) {
            confirm_clear = true;
        }
        EndRow();
        EndSettingsGroup();
        ui::Space(20.0F);

        group_top[3] = ImGui::GetCursorScreenPos().y;
        {
            const ImVec2 about = ImGui::GetCursorScreenPos();
            const float about_h =
                14.0F * 2.0F + LineHeight(Face::PlexRegular, 12.0F);
            draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
                about,
                ImVec2(about.x + groups_w, about.y + about_h),
                app::kCard,
                12.0F);
            draw_list->AddRect(
                about,
                ImVec2(about.x + groups_w, about.y + about_h),
                app::kCardBorder,
                12.0F);
            ui::TextAt(
                draw_list,
                ImVec2(about.x + 24.0F, about.y + 14.0F),
                Face::PlexRegular,
                12.0F,
                app::kMuted,
                "Apex Labs v0.1.0  \xC2\xB7  Physics engine v0.1.0  \xC2\xB7  "
                "Schema v1");
            const char* updates = "Check for updates";
            const float updates_w =
                ui::TextSize(Face::PlexSemiBold, 12.0F, updates).x;
            ImGui::SetCursorScreenPos(
                ImVec2(about.x + groups_w - 24.0F - updates_w, about.y + 14.0F));
            bool hovered = false;
            if (ui::HitArea(
                    "##updates",
                    ImVec2(updates_w, LineHeight(Face::PlexSemiBold, 12.0F)),
                    &hovered)) {
                app.ShowToast("You're on the latest version (v0.1.0).");
            }
            ui::SetHandCursorIfHovered();
            ui::TextAt(
                draw_list,
                ImVec2(about.x + groups_w - 24.0F - updates_w, about.y + 14.0F),
                Face::PlexSemiBold,
                12.0F,
                hovered ? kWhite : app::kText,
                updates);
            ImGui::SetCursorScreenPos(ImVec2(about.x, about.y + about_h));
            ImGui::Dummy(ImVec2(groups_w, 0.0F));
        }
    }
    ImGui::EndGroup();

    {
        int current = 0;
        for (int i = 0; i < 4; ++i) {
            if (group_top[i] - content_top <= 140.0F) {
                current = i;
            }
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0F &&
            ImGui::GetScrollMaxY() > 0.0F) {
            current = 3;
        }
        if (scroll_to < 0) {
            active_group = current;
        }
    }

    {
        const float sticky = std::max(body.y, content_top + 24.0F);
        ImGui::SetCursorScreenPos(ImVec2(body.x, sticky));
        static const char* const kNav[] = {
            "Simulation",
            "Units & display",
            "Data & storage",
            "About",
        };
        const float item_h =
            10.0F * 2.0F + LineHeight(Face::PlexSemiBold, 14.0F);
        const float nav_h = 12.0F * 2.0F + item_h * 4.0F + 4.0F * 3.0F;
        const ImVec2 nav = ImGui::GetCursorScreenPos();
        draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(
            nav,
            ImVec2(nav.x + nav_w, nav.y + nav_h),
            app::kCard,
            12.0F);
        draw_list->AddRect(
            nav,
            ImVec2(nav.x + nav_w, nav.y + nav_h),
            app::kCardBorder,
            12.0F);
        for (int i = 0; i < 4; ++i) {
            const ImVec2 item(
                nav.x + 12.0F,
                nav.y + 12.0F + static_cast<float>(i) * (item_h + 4.0F));
            ImGui::SetCursorScreenPos(item);
            ImGui::PushID(i);
            bool hovered = false;
            if (ui::HitArea("##nav", ImVec2(nav_w - 24.0F, item_h), &hovered)) {
                scroll_to = i;
                active_group = i;
            }
            ui::SetHandCursorIfHovered();
            ImGui::PopID();
            const bool selected = active_group == i;
            if (selected || hovered) {
                draw_list->AddRectFilled(
                    item,
                    ImVec2(item.x + nav_w - 24.0F, item.y + item_h),
                    app::kRaised,
                    8.0F);
            }
            if (selected) {
                draw_list->AddRectFilled(
                    ImVec2(item.x + 12.0F, item.y + item_h * 0.5F - 8.0F),
                    ImVec2(item.x + 15.0F, item.y + item_h * 0.5F + 8.0F),
                    kRed,
                    2.0F);
            }
            ui::TextAt(
                draw_list,
                ImVec2(item.x + 25.0F, item.y + 10.0F),
                selected ? Face::PlexSemiBold : Face::SpaceMedium,
                14.0F,
                selected ? app::kText
                         : (hovered ? app::kTextSoft : app::kMuted),
                kNav[i]);
        }
    }

    if (scroll_to >= 0) {
        float target =
            scroll_y + (group_top[scroll_to] - content_top) - 24.0F;
        target = std::clamp(target, 0.0F, ImGui::GetScrollMaxY());
        const float y = ImGui::GetScrollY();
        float next =
            y + (target - y) * std::min(1.0F, ImGui::GetIO().DeltaTime * 14.0F);
        if (std::fabs(target - next) < 1.0F) {
            next = target;
            scroll_to = -1;
        }
        ImGui::SetScrollY(next);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    if (confirm_clear) {
        ImGui::OpenPopup("##confirm_clear");
        confirm_clear = false;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->Pos.x + viewport->Size.x * 0.5F,
            viewport->Pos.y + viewport->Size.y * 0.5F),
        ImGuiCond_Always,
        ImVec2(0.5F, 0.5F));
    ImGui::SetNextWindowSize(ImVec2(420.0F, 0.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0F, 22.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0F);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kCard);
    ImGui::PushStyleColor(ImGuiCol_Border, app::kControlBorder);
    if (ImGui::BeginPopupModal(
            "##confirm_clear",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ui::Text(Face::PlexSemiBold, 18.0F, app::kText, "Clear run history?");
        ui::Space(8.0F);
        ui::TextWrapped(
            Face::PlexRegular,
            13.0F,
            app::kMuted,
            "All saved runs and their telemetry will be deleted. Saved cars "
            "are kept. This cannot be undone.");
        ui::Space(20.0F);
        const float popup_w = ImGui::GetContentRegionAvail().x;
        const ImVec2 footer = ImGui::GetCursorScreenPos();
        const float delete_w = ButtonWidth("Delete all runs");
        const float cancel_w = ButtonWidth("Cancel");
        ImGui::SetCursorScreenPos(
            ImVec2(footer.x + popup_w - delete_w - 8.0F - cancel_w, footer.y));
        if (ui::Button("##cancel", "Cancel", ui::ButtonKind::Outline) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetCursorScreenPos(
            ImVec2(footer.x + popup_w - delete_w, footer.y));
        if (ui::Button("##confirm", "Delete all runs", ui::ButtonKind::Danger)) {
            ImGui::CloseCurrentPopup();
            app.ShowToast(
                "Run history isn't stored yet, so there was nothing to delete.");
        }
        ImGui::SetCursorScreenPos(ImVec2(footer.x, footer.y + ButtonHeight()));
        ImGui::Dummy(ImVec2(popup_w, 0.0F));
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
