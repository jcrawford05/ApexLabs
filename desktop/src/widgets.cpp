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

void IconWeather(
    ImDrawList* draw_list,
    ImVec2 top_left,
    float scale,
    bool rain) {
    auto Point = [&](float x, float y) {
        return ImVec2(top_left.x + x * scale, top_left.y + y * scale);
    };
    const ImU32 sun = theme::kRed;
    const ImU32 cloud = theme::Hex(0xB8BCC4);
    const ImU32 background = theme::setup::kMap;
    const ImVec2 sun_center = Point(30.0F, 28.0F);
    for (int i = 0; i < 8; ++i) {
        const float angle = i * 3.14159265F / 4.0F;
        const ImVec2 direction(std::cos(angle), std::sin(angle));
        draw_list->AddLine(
            ImVec2(
                sun_center.x + direction.x * 15.0F * scale,
                sun_center.y + direction.y * 15.0F * scale),
            ImVec2(
                sun_center.x + direction.x * 21.0F * scale,
                sun_center.y + direction.y * 21.0F * scale),
            sun,
            2.0F * scale);
    }
    draw_list->AddCircle(sun_center, 11.0F * scale, sun, 32, 2.0F * scale);

    struct Blob {
        float x;
        float y;
        float radius;
    };
    const Blob blobs[] = {
        {44.0F, 50.0F, 11.0F},
        {58.0F, 38.0F, 16.0F},
        {76.0F, 44.0F, 13.0F},
        {86.0F, 54.0F, 10.0F},
    };
    constexpr float kOutline = 2.0F;
    for (const Blob& blob : blobs) {
        draw_list->AddCircleFilled(
            Point(blob.x, blob.y),
            (blob.radius + kOutline) * scale,
            cloud,
            40);
    }
    draw_list->AddRectFilled(
        Point(44.0F, 50.0F - kOutline),
        Point(86.0F, 64.0F + kOutline),
        cloud,
        10.0F * scale);
    for (const Blob& blob : blobs) {
        draw_list->AddCircleFilled(
            Point(blob.x, blob.y),
            blob.radius * scale,
            background,
            40);
    }
    draw_list->AddRectFilled(
        Point(44.0F, 50.0F),
        Point(86.0F, 64.0F),
        background,
        8.0F * scale);
    if (rain) {
        for (int i = 0; i < 4; ++i) {
            const float x = 52.0F + i * 9.0F;
            draw_list->AddLine(
                Point(x, 68.0F),
                Point(x - 3.0F, 76.0F),
                theme::Hex(0x6FA8FF),
                2.0F * scale);
        }
    }
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
    const float gap = compact ? 4.0F : 5.0F;
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

bool BigRedButton(const char* id, const char* label, ImVec2 size) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool held = false;
    const bool pressed = HitArea(id, size, &hovered, &held);
    SetHandCursorIfHovered();
    const ImU32 background = held
        ? theme::kRedDark
        : (hovered ? theme::Hex(0xF2160F) : theme::kRed);
    ImDrawList* draw_list = DrawList();
    draw_list->AddRectFilled(
        position,
        ImVec2(position.x + size.x, position.y + size.y),
        background,
        12.0F);
    const ImVec2 text_size = TextSize(Face::PlexSemiBold, 20.0F, label);
    TextAt(
        draw_list,
        ImVec2(
            position.x + (size.x - text_size.x) * 0.5F,
            position.y + (size.y - text_size.y) * 0.5F),
        Face::PlexSemiBold,
        20.0F,
        theme::kWhite,
        label);
    return pressed;
}

bool Button(
    const char* id,
    const char* label,
    ButtonKind kind,
    float font_size,
    float pad_x,
    float pad_y) {
    const bool subtle = kind == ButtonKind::Subtle;
    const Face face = subtle ? Face::PlexSemiBold : Face::PlexSemiBold;
    const bool outlined =
        kind == ButtonKind::Outline || kind == ButtonKind::Danger;
    const ImVec2 text_size = TextSize(face, font_size, label);
    const float border = outlined ? 1.0F : 0.0F;
    const ImVec2 size(
        text_size.x + pad_x * 2.0F + border * 2.0F,
        text_size.y + pad_y * 2.0F + border * 2.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    bool held = false;
    const bool pressed = HitArea(id, size, &hovered, &held);
    SetHandCursorIfHovered();
    ImDrawList* draw_list = DrawList();
    const ImVec2 end(position.x + size.x, position.y + size.y);
    ImU32 text = theme::app::kText;
    switch (kind) {
        case ButtonKind::Primary:
            draw_list->AddRectFilled(
                position,
                end,
                held ? theme::kRedDark
                     : (hovered ? theme::Hex(0xF2160F) : theme::kRed),
                8.0F);
            break;
        case ButtonKind::Outline:
            if (hovered) {
                draw_list->AddRectFilled(
                    position, end, theme::app::kRaised, 8.0F);
            }
            draw_list->AddRect(
                position,
                end,
                hovered ? theme::Hex(0x4A4A52) : theme::app::kControlBorder,
                8.0F);
            break;
        case ButtonKind::Danger:
            if (hovered) {
                draw_list->AddRectFilled(
                    position, end, theme::Hex(0xE10600, 28), 8.0F);
            }
            draw_list->AddRect(position, end, theme::kRed, 8.0F);
            text = theme::app::kLost;
            break;
        case ButtonKind::Subtle:
            draw_list->AddRectFilled(
                position,
                end,
                hovered ? theme::Hex(0x2A2A30) : theme::app::kRaised,
                8.0F);
            text = hovered ? theme::app::kText : theme::app::kMuted;
            break;
    }
    TextAt(
        draw_list,
        ImVec2(position.x + border + pad_x, position.y + border + pad_y),
        face,
        font_size,
        text,
        label);
    return pressed;
}

bool Chip(const char* id, const char* label, bool selected) {
    const ImVec2 text_size = TextSize(Face::PlexSemiBold, 12.0F, label);
    const ImVec2 size(text_size.x + 24.0F, text_size.y + 12.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    bool hovered = false;
    const bool pressed = HitArea(id, size, &hovered);
    SetHandCursorIfHovered();
    const ImU32 background = selected
        ? theme::kRed
        : (hovered ? theme::Hex(0x2A2A30) : theme::app::kRaised);
    const ImU32 text = selected
        ? theme::kWhite
        : (hovered ? theme::app::kText : theme::app::kMuted);
    DrawList()->AddRectFilled(
        position,
        ImVec2(position.x + size.x, position.y + size.y),
        background,
        size.y * 0.5F);
    TextAt(
        DrawList(),
        ImVec2(position.x + 12.0F, position.y + 6.0F),
        Face::PlexSemiBold,
        12.0F,
        text,
        label);
    return pressed;
}

void StatusDot(ImDrawList* draw_list, ImVec2 center, float radius, ImU32 color) {
    draw_list->AddCircleFilled(center, radius, color, 16);
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
