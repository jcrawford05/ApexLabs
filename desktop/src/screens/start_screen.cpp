// Start Screen migration from ApexLabsDevSpace commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0 (1440 x 900 reference frame).
#include <cmath>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

void RadialGlow(ImDrawList* draw_list, ImVec2 center, float radius, ImU32 inner) {
    constexpr int kSegments = 96;
    constexpr int kRings = 12;
    const ImVec4 color = ImGui::ColorConvertU32ToFloat4(inner);
    const auto ring_color = [&](float progress) {
        const float alpha =
            color.w * std::pow(1.0F - progress, 1.6F);
        return ImGui::ColorConvertFloat4ToU32(
            ImVec4(color.x, color.y, color.z, alpha));
    };

    for (int ring = 0; ring < kRings; ++ring) {
        const float progress0 = static_cast<float>(ring) / kRings;
        const float progress1 = static_cast<float>(ring + 1) / kRings;
        const ImU32 color0 = ring_color(progress0);
        const ImU32 color1 = ring_color(progress1);
        const float radius0 = radius * progress0;
        const float radius1 = radius * progress1;
        draw_list->PrimReserve(kSegments * 6, kSegments * 4);
        for (int segment = 0; segment < kSegments; ++segment) {
            constexpr float kTau = 2.0F * 3.14159265F;
            const float angle0 = kTau * segment / kSegments;
            const float angle1 = kTau * (segment + 1) / kSegments;
            const ImVec2 point00(
                center.x + std::cos(angle0) * radius0,
                center.y + std::sin(angle0) * radius0);
            const ImVec2 point01(
                center.x + std::cos(angle1) * radius0,
                center.y + std::sin(angle1) * radius0);
            const ImVec2 point10(
                center.x + std::cos(angle0) * radius1,
                center.y + std::sin(angle0) * radius1);
            const ImVec2 point11(
                center.x + std::cos(angle1) * radius1,
                center.y + std::sin(angle1) * radius1);
            const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            const ImDrawIdx index =
                static_cast<ImDrawIdx>(draw_list->_VtxCurrentIdx);
            draw_list->PrimWriteVtx(point00, uv, color0);
            draw_list->PrimWriteVtx(point01, uv, color0);
            draw_list->PrimWriteVtx(point11, uv, color1);
            draw_list->PrimWriteVtx(point10, uv, color1);
            draw_list->PrimWriteIdx(index);
            draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(index + 1));
            draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(index + 2));
            draw_list->PrimWriteIdx(index);
            draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(index + 2));
            draw_list->PrimWriteIdx(static_cast<ImDrawIdx>(index + 3));
        }
    }
}

enum class MenuIcon {
    Plus,
    Import,
    Swap,
};

bool MenuAction(
    const char* id,
    const char* title,
    const char* subtitle,
    MenuIcon icon,
    bool primary) {
    const ImVec2 size(540.0F, 70.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool held = false;
    const bool pressed = ui::HitArea(id, size, &hovered, &held);
    ui::SetHandCursorIfHovered();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 end(position.x + size.x, position.y + size.y);

    const ImU32 background = primary
        ? (held ? kRedDark : hovered ? Hex(0xF2160F) : kRed)
        : (hovered ? Hex(0x181B21) : setup::kMenuCard);
    const ImU32 border = primary
        ? kRedBorder
        : (hovered ? Hex(0x3A3E46) : setup::kMenuBorder);
    draw_list->AddRectFilled(position, end, background, 10.0F);
    draw_list->AddRect(position, end, border, 10.0F);

    const ImVec2 icon_box(position.x + 19.0F, position.y + 18.0F);
    draw_list->AddRectFilled(
        icon_box,
        ImVec2(icon_box.x + 34.0F, icon_box.y + 34.0F),
        primary ? kRedDark : setup::kMenuIcon,
        7.0F);
    const ImVec2 icon_center(icon_box.x + 17.0F, icon_box.y + 17.0F);
    const ImU32 icon_color = primary ? kWhite : setup::kMenuIconText;
    switch (icon) {
        case MenuIcon::Plus:
            ui::IconPlus(draw_list, icon_center, 11.0F, icon_color);
            break;
        case MenuIcon::Import:
            ui::IconImport(draw_list, icon_center, 14.0F, icon_color);
            break;
        case MenuIcon::Swap:
            ui::IconSwap(draw_list, icon_center, 14.0F, icon_color);
            break;
    }

    ui::TextAt(
        draw_list,
        ImVec2(position.x + 67.0F, position.y + 17.0F),
        Face::SpaceBold,
        15.0F,
        primary ? kWhite : setup::kMenuTitle,
        title);
    ui::TextAt(
        draw_list,
        ImVec2(position.x + 67.0F, position.y + 39.0F),
        Face::PlexRegular,
        11.0F,
        primary ? setup::kMenuSubActive : setup::kMenuSub,
        subtitle);
    const float nudge = hovered ? 3.0F : 0.0F;
    ui::IconArrowRight(
        draw_list,
        ImVec2(position.x + 513.0F + nudge, position.y + 35.0F),
        13.0F,
        primary
            ? kWhite
            : (hovered ? setup::kMenuTitle : setup::kMenuArrow),
        2.0F);
    return pressed;
}

bool FooterLink(
    const char* id,
    const char* label,
    ImVec2 position,
    bool enabled = true) {
    const ImVec2 text_size = ui::TextSize(Face::PlexSemiBold, 20.0F, label);
    ImGui::SetCursorScreenPos(position);
    bool hovered = false;
    const bool pressed = enabled && ui::HitArea(id, text_size, &hovered);
    if (!enabled) {
        ImGui::Dummy(text_size);
    }
    if (enabled) {
        ui::SetHandCursorIfHovered();
    }
    ui::TextAt(
        ImGui::GetWindowDrawList(),
        position,
        Face::PlexSemiBold,
        20.0F,
        hovered ? setup::kTagline : setup::kFooter,
        label);
    return pressed;
}

}  // namespace

void DrawStartScreen(App& app) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float width = size.x;
    const float height = size.y;

    for (float x = -1.0F; x < width; x += 80.0F) {
        draw_list->AddRectFilled(
            ImVec2(origin.x + x, origin.y),
            ImVec2(origin.x + x + 1.0F, origin.y + height),
            Hex(0x2A2D33, 41));
    }
    for (float y = 20.0F; y < height; y += 80.0F) {
        draw_list->AddRectFilled(
            ImVec2(origin.x, origin.y + y),
            ImVec2(origin.x + width, origin.y + y + 1.0F),
            Hex(0x2A2D33, 31));
    }
    draw_list->AddRectFilled(
        origin,
        ImVec2(origin.x + width, origin.y + 2.0F),
        kRed);
    draw_list->AddRectFilled(
        ImVec2(origin.x, origin.y + height - 1.0F),
        ImVec2(origin.x + width, origin.y + height),
        Hex(0x2A2D33));

    const float offset_x = width - 1440.0F;
    const float offset_y = (height - 900.0F) * 0.5F;
    ImVec2 image_position(origin.x + 313.0F + offset_x, origin.y);
    draw_list->AddImage(
        app.track_line_right.id(),
        image_position,
        ImVec2(image_position.x + 1128.0F, image_position.y + 883.0F));
    image_position = ImVec2(origin.x, origin.y + height - 491.0F);
    draw_list->AddImage(
        app.track_line_left.id(),
        image_position,
        ImVec2(image_position.x + 522.0F, image_position.y + 405.0F));
    RadialGlow(
        draw_list,
        ImVec2(
            origin.x + 1128.0F + offset_x,
            origin.y + 465.0F + offset_y),
        397.0F,
        Hex(0xE10600, 170));
    image_position = ImVec2(
        origin.x + 839.0F + offset_x,
        origin.y + 176.0F + offset_y);
    draw_list->AddImage(
        app.logo_badge.id(),
        image_position,
        ImVec2(image_position.x + 578.0F, image_position.y + 578.0F));

    const float hero_x = origin.x + 132.0F;
    const float hero_y = origin.y + 118.0F + offset_y;
    draw_list->AddImage(
        app.logo_wordmark.id(),
        ImVec2(hero_x, hero_y + 15.0F),
        ImVec2(hero_x + 473.0F, hero_y + 206.0F));
    ui::TextAt(
        draw_list,
        ImVec2(hero_x, hero_y + 224.0F),
        Face::SpaceMedium,
        18.0F,
        setup::kTagline,
        "Design. Simulate. Analyze. Iterate.");

    ImGui::SetCursorScreenPos(ImVec2(hero_x, hero_y + 265.0F));
    if (MenuAction(
            "##new",
            "NEW CONFIGURATION",
            "Create a car setup from a clean baseline",
            MenuIcon::Plus,
            true)) {
        app.car_panel = CarPanel::Aerodynamics;
        app.track_panel = TrackPanel::Circuit;
        app.track_focus = false;
        app.GoTo(Screen::CarSetup);
    }
    ImGui::SetCursorScreenPos(ImVec2(hero_x, hero_y + 347.0F));
    if (MenuAction(
            "##import",
            "IMPORT SAVED",
            "Continue from an existing configuration",
            MenuIcon::Import,
            false)) {
        app.GoTo(Screen::ImportExport);
    }
    ImGui::SetCursorScreenPos(ImVec2(hero_x, hero_y + 429.0F));
    if (MenuAction(
            "##compare",
            "COMPARISON TOOL",
            "Compare previous simulation runs side by side",
            MenuIcon::Swap,
            false)) {
        app.GoTo(Screen::Compare);
    }

    const float footer_y = origin.y + height - 73.0F;
    if (FooterLink(
            "##settings",
            "SETTINGS",
            ImVec2(origin.x + 58.0F, footer_y))) {
        app.GoTo(Screen::Settings);
    }
    if (FooterLink(
            "##exit",
            "EXIT",
            ImVec2(origin.x + 197.0F, footer_y))) {
        app.request_quit = true;
    }
    FooterLink(
        "##version",
        "v0.1.0",
        ImVec2(origin.x + 283.0F, footer_y),
        false);
}
