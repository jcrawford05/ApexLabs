#include "widgets.h"

#include <algorithm>
#include <cfloat>

namespace ui {

namespace {

ImU32 Lighten(ImU32 color, float amount) {
    ImVec4 value = ImGui::ColorConvertU32ToFloat4(color);
    value.x += (1.0F - value.x) * amount;
    value.y += (1.0F - value.y) * amount;
    value.z += (1.0F - value.z) * amount;
    return ImGui::ColorConvertFloat4ToU32(value);
}

ImDrawList* DrawList() {
    return ImGui::GetWindowDrawList();
}

}  // namespace

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

void Text(Face face, float size, ImU32 color, const char* text) {
    const ImVec2 measured = TextSize(face, size, text);
    TextAt(DrawList(), ImGui::GetCursorScreenPos(), face, size, color, text);
    ImGui::Dummy(measured);
}

void Space(float height) {
    ImGui::Dummy(ImVec2(0.0F, height));
}

void Divider(ImU32 color, float thickness) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    DrawList()->AddRectFilled(
        position,
        ImVec2(position.x + width, position.y + thickness),
        color);
    ImGui::Dummy(ImVec2(width, thickness));
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

bool CategoryCard(
    const char* id,
    const char* title,
    const char* subtitle,
    bool selected,
    float width) {
    const ImVec2 size(width, 58.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    const bool pressed = HitArea(id, size, &hovered);
    SetHandCursorIfHovered();

    ImDrawList* draw_list = DrawList();
    ImU32 background = selected ? theme::setup::kCardActive : theme::setup::kCard;
    ImU32 border =
        selected ? theme::setup::kCardActiveBorder : theme::setup::kCardBorder;
    if (hovered && !selected) {
        background = Lighten(background, 0.03F);
        border = Lighten(border, 0.08F);
    }

    const ImVec2 end(position.x + size.x, position.y + size.y);
    draw_list->AddRectFilled(position, end, background, 9.0F);
    draw_list->AddRect(position, end, border, 9.0F);
    if (selected) {
        draw_list->AddRectFilled(
            ImVec2(position.x, position.y + 11.0F),
            ImVec2(position.x + 3.0F, position.y + 45.0F),
            theme::kRed,
            2.0F);
    }

    TextAt(
        draw_list,
        ImVec2(position.x + 14.0F, position.y + 12.0F),
        selected ? Face::PlexSemiBold : Face::PlexRegular,
        12.0F,
        selected ? theme::kWhite : theme::setup::kCardTitle,
        title);
    TextAt(
        draw_list,
        ImVec2(position.x + 14.0F, position.y + 31.0F),
        Face::PlexRegular,
        9.0F,
        selected ? theme::setup::kActiveSub : theme::setup::kMuted3,
        subtitle);
    return pressed;
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
