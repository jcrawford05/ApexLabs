#include "widgets.h"

#include <algorithm>
#include <cfloat>

namespace ui {

ImVec2 TextSize(Face face, float size, const char* text) {
    return theme::Font(face, size)->CalcTextSizeA(
        theme::LineHeight(face, size), FLT_MAX, 0.0F, text);
}

void TextAt(
    ImDrawList* draw_list,
    ImVec2 position,
    Face face,
    float size,
    ImU32 color,
    const char* text) {
    draw_list->AddText(
        theme::Font(face, size),
        theme::LineHeight(face, size),
        position,
        color,
        text);
}

void IconPlus(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color) {
    const float half = size * 0.5F;
    draw_list->AddLine(
        ImVec2(center.x - half, center.y),
        ImVec2(center.x + half, center.y),
        color,
        2.0F);
    draw_list->AddLine(
        ImVec2(center.x, center.y - half),
        ImVec2(center.x, center.y + half),
        color,
        2.0F);
}

void IconImport(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color) {
    const float half = size * 0.5F;
    draw_list->AddLine(
        ImVec2(center.x - half * 0.7F, center.y - half),
        ImVec2(center.x + half * 0.7F, center.y - half),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x, center.y - half * 0.65F),
        ImVec2(center.x, center.y + half),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x, center.y - half * 0.65F),
        ImVec2(center.x - half * 0.45F, center.y - half * 0.2F),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x, center.y - half * 0.65F),
        ImVec2(center.x + half * 0.45F, center.y - half * 0.2F),
        color,
        1.6F);
}

void IconSwap(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color) {
    const float half = size * 0.5F;
    const float upper = center.y - half * 0.35F;
    const float lower = center.y + half * 0.35F;
    draw_list->AddLine(
        ImVec2(center.x - half, upper),
        ImVec2(center.x + half, upper),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x + half, upper),
        ImVec2(center.x + half * 0.55F, upper - half * 0.4F),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x - half, lower),
        ImVec2(center.x + half, lower),
        color,
        1.6F);
    draw_list->AddLine(
        ImVec2(center.x - half, lower),
        ImVec2(center.x - half * 0.55F, lower + half * 0.4F),
        color,
        1.6F);
}

void IconArrowRight(
    ImDrawList* draw_list,
    ImVec2 center,
    float size,
    ImU32 color,
    float thickness) {
    const float half = size * 0.5F;
    draw_list->AddLine(
        ImVec2(center.x - half, center.y),
        ImVec2(center.x + half, center.y),
        color,
        thickness);
    draw_list->AddLine(
        ImVec2(center.x + half, center.y),
        ImVec2(center.x + half * 0.25F, center.y - half * 0.7F),
        color,
        thickness);
    draw_list->AddLine(
        ImVec2(center.x + half, center.y),
        ImVec2(center.x + half * 0.25F, center.y + half * 0.7F),
        color,
        thickness);
}

bool HitArea(const char* id, ImVec2 size, bool* hovered, bool* held) {
    const bool pressed = ImGui::InvisibleButton(
        id,
        ImVec2(std::max(size.x, 1.0F), std::max(size.y, 1.0F)));
    if (hovered) {
        *hovered = ImGui::IsItemHovered();
    }
    if (held) {
        *held = ImGui::IsItemActive();
    }
    return pressed;
}

void SetHandCursorIfHovered() {
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

}  // namespace ui
