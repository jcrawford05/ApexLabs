#include "widgets.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace ui {

namespace {

ImU32 Lighten(ImU32 color, float amount) {
    ImVec4 value = ImGui::ColorConvertU32ToFloat4(color);
    value.x += (1.0F - value.x) * amount;
    value.y += (1.0F - value.y) * amount;
    value.z += (1.0F - value.z) * amount;
    return ImGui::ColorConvertFloat4ToU32(value);
}

ImU32 WithAlpha(ImU32 color, float alpha) {
    ImVec4 value = ImGui::ColorConvertU32ToFloat4(color);
    value.w *= alpha;
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

void KeyValue(
    const char* key,
    const char* value,
    ImU32 value_color,
    Face key_face,
    float key_size,
    Face value_face,
    float value_size,
    ImU32 key_color) {
    const ImVec2 key_size_px = TextSize(key_face, key_size, key);
    const ImVec2 value_size_px = TextSize(value_face, value_size, value);
    const float height = std::max(key_size_px.y, value_size_px.y);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    TextAt(
        DrawList(),
        ImVec2(position.x, position.y + (height - key_size_px.y) * 0.5F),
        key_face,
        key_size,
        key_color,
        key);
    TextAt(
        DrawList(),
        ImVec2(
            position.x + width - value_size_px.x,
            position.y + (height - value_size_px.y) * 0.5F),
        value_face,
        value_size,
        value_color,
        value);
    ImGui::Dummy(ImVec2(width, height));
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

void BeginCard(
    const char* id,
    ImU32 background,
    ImU32 border,
    float rounding,
    ImVec2 padding,
    float width) {
    if (width <= 0.0F) {
        width = ImGui::GetContentRegionAvail().x;
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, border ? 1.0F : 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(
        id,
        ImVec2(width, 0.0F),
        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding |
            (border ? ImGuiChildFlags_Borders : 0),
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
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

bool Pill(
    const char* id,
    const char* label,
    bool selected,
    PillStyle style,
    bool enabled) {
    const ImVec2 text_size = TextSize(Face::PlexSemiBold, style.font_size, label);
    const ImVec2 size(
        text_size.x + style.pad_x * 2.0F + 2.0F,
        text_size.y + style.pad_y * 2.0F + 2.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    ImGui::BeginDisabled(!enabled);
    const bool pressed = HitArea(id, size, &hovered);
    ImGui::EndDisabled();
    if (enabled) {
        SetHandCursorIfHovered();
    }

    ImU32 background =
        selected ? theme::setup::kPillActive : theme::setup::kPill;
    ImU32 border =
        selected ? theme::setup::kPillActiveBorder : theme::setup::kPillBorder;
    ImU32 text =
        selected ? theme::setup::kPillActiveText : theme::setup::kPillText;
    if (hovered && enabled && !selected) {
        border = Lighten(border, 0.12F);
        text = Lighten(text, 0.3F);
    }
    if (!enabled) {
        text = WithAlpha(text, 0.45F);
    }

    ImDrawList* draw_list = DrawList();
    draw_list->AddRectFilled(
        position,
        ImVec2(position.x + size.x, position.y + size.y),
        background,
        style.rounding);
    draw_list->AddRect(
        position,
        ImVec2(position.x + size.x, position.y + size.y),
        border,
        style.rounding);
    TextAt(
        draw_list,
        ImVec2(position.x + 1.0F + style.pad_x, position.y + 1.0F + style.pad_y),
        Face::PlexSemiBold,
        style.font_size,
        text,
        label);
    return pressed && enabled;
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

bool SetupSlider(
    const char* id,
    const char* label,
    float* value,
    float min,
    float max,
    const char* value_fmt,
    const char* min_label,
    const char* max_label,
    bool compact,
    float step) {
    const float label_size = compact ? 10.0F : 11.0F;
    const float bar_height = compact ? 5.0F : 6.0F;
    const float gap = compact ? 6.0F : 7.0F;
    const float width = ImGui::GetContentRegionAvail().x;

    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), value_fmt, *value);
    KeyValue(
        label,
        buffer,
        theme::setup::kValue,
        Face::PlexRegular,
        label_size,
        Face::PlexSemiBold,
        label_size,
        theme::setup::kLabel);
    Space(gap);

    const ImVec2 bar = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(bar.x, bar.y - 6.0F));
    bool hovered = false;
    bool held = false;
    ImGui::PushID(id);
    HitArea("##bar", ImVec2(width, bar_height + 12.0F), &hovered, &held);
    ImGui::PopID();
    if (hovered || held) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    bool changed = false;
    if (held) {
        float t = std::clamp((ImGui::GetIO().MousePos.x - bar.x) / width, 0.0F, 1.0F);
        float next = min + t * (max - min);
        if (step > 0.0F) {
            next = std::round(next / step) * step;
        }
        next = std::clamp(next, min, max);
        if (next != *value) {
            *value = next;
            changed = true;
        }
    }

    const float t = (max > min)
        ? std::clamp((*value - min) / (max - min), 0.0F, 1.0F)
        : 0.0F;
    ImDrawList* draw_list = DrawList();
    draw_list->AddRectFilled(
        bar,
        ImVec2(bar.x + width, bar.y + bar_height),
        theme::setup::kTrack,
        3.0F);
    if (t > 0.0F) {
        draw_list->AddRectFilled(
            bar,
            ImVec2(bar.x + std::max(width * t, bar_height), bar.y + bar_height),
            theme::kRed,
            3.0F);
    }
    if (hovered || held) {
        const ImVec2 knob(bar.x + width * t, bar.y + bar_height * 0.5F);
        const float radius = held ? 7.0F : 6.0F;
        draw_list->AddCircleFilled(knob, radius, theme::kWhite, 24);
        draw_list->AddCircle(knob, radius, theme::kRed, 24, 2.0F);
    }

    ImGui::SetCursorScreenPos(ImVec2(bar.x, bar.y + bar_height));
    ImGui::Dummy(ImVec2(width, 0.0F));
    Space(gap);

    const ImVec2 range = ImGui::GetCursorScreenPos();
    const ImVec2 max_size = TextSize(Face::PlexRegular, 8.0F, max_label);
    TextAt(draw_list, range, Face::PlexRegular, 8.0F, theme::setup::kRange, min_label);
    TextAt(
        draw_list,
        ImVec2(range.x + width - max_size.x, range.y),
        Face::PlexRegular,
        8.0F,
        theme::setup::kRange,
        max_label);
    ImGui::Dummy(ImVec2(width, max_size.y));
    return changed;
}

void SectionLabel(const char* text) {
    Text(Face::PlexSemiBold, 9.0F, theme::setup::kMuted2, text);
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
