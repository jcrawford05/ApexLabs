// Settings (Figma: "Settings", 1440 x 1590).
// Adapted from ApexLabsDevSpace settings_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. Uses Space Grotesk / IBM Plex Sans.
//
// Slice 1–2: shell (nav + groups) and prefs Save / Reset against the fields
// other screens already read. Per-section controls land in later slices.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

float ButtonWidth(const char* label, float font = 14.0F, float pad_x = 16.0F) {
    return ui::TextSize(Face::PlexSemiBold, font, label).x + pad_x * 2.0F;
}

float ButtonHeight(float font = 14.0F, float pad_y = 10.0F) {
    return LineHeight(Face::PlexSemiBold, font) + pad_y * 2.0F;
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

void StubNote(const char* text) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    draw_list->AddRectFilled(
        position,
        ImVec2(position.x + width, position.y + 1.0F),
        app::kCardBorder);
    ui::TextAt(
        draw_list,
        ImVec2(position.x, position.y + 14.0F),
        Face::PlexRegular,
        13.0F,
        app::kMuted,
        text);
    const float height = 14.0F * 2.0F + LineHeight(Face::PlexRegular, 13.0F);
    ImGui::Dummy(ImVec2(width, height));
}

}  // namespace

void DrawSettingsScreen(App& app) {
    DrawTopNav(app, "Apex Labs  \xC2\xB7  v0.1.0");
    Preferences& prefs = app.prefs;
    const Preferences& saved = app.saved_prefs;
    static int active_group = 0;
    static int scroll_to = -1;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0F, 24.0F));
    ImGui::BeginChild(
        "##settings",
        ImVec2(0.0F, 0.0F),
        ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float width = ImGui::GetContentRegionAvail().x;
    const int unsaved = prefs.DifferencesFrom(saved);

    // Page header
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
        group_top[0] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_sim", groups_w);
        GroupHeader(
            "Simulation",
            "How the physics engine runs a lap. Changing these makes new runs "
            "not directly comparable with old ones.");
        StubNote("Simulation controls land in the next Settings slice.");
        EndSettingsGroup();
        ui::Space(20.0F);

        group_top[1] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_units", groups_w);
        GroupHeader("Units & display", "How numbers and the Results page look.");
        StubNote("Units & display controls land in the next Settings slice.");
        EndSettingsGroup();
        ui::Space(20.0F);

        group_top[2] = ImGui::GetCursorScreenPos().y;
        BeginSettingsGroup("##g_data", groups_w);
        GroupHeader(
            "Data & storage",
            "Where saved cars and run history are kept.");
        StubNote("Data & storage controls land in a later Settings slice.");
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
}
