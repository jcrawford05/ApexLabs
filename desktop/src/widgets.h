// Reusable drawing helpers for migrated desktop screens.
// Adapted from ApexLabsDevSpace at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
#pragma once

#include "imgui.h"
#include "theme.h"

namespace ui {

using theme::Face;

ImVec2 TextSize(Face face, float size, const char* text);
void Text(Face face, float size, ImU32 color, const char* text);
void TextAt(
    ImDrawList* draw_list,
    ImVec2 position,
    Face face,
    float size,
    ImU32 color,
    const char* text);
void KeyValue(
    const char* key,
    const char* value,
    ImU32 value_color,
    Face key_face,
    float key_size,
    Face value_face,
    float value_size,
    ImU32 key_color);

void Space(float height);
void Divider(ImU32 color, float thickness = 1.0F);
void BeginCard(
    const char* id,
    ImU32 background,
    ImU32 border,
    float rounding,
    ImVec2 padding,
    float width = 0.0F);
void EndCard();

void IconPlus(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconImport(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconSwap(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconArrowRight(
    ImDrawList* draw_list,
    ImVec2 center,
    float size,
    ImU32 color,
    float thickness = 2.0F);

struct PillStyle {
    float pad_x = 11.0F;
    float pad_y = 7.0F;
    float rounding = 7.0F;
    float font_size = 10.0F;
};

bool Pill(
    const char* id,
    const char* label,
    bool selected,
    PillStyle style = {},
    bool enabled = true);
bool CategoryCard(
    const char* id,
    const char* title,
    const char* subtitle,
    bool selected,
    float width);
bool SetupSlider(
    const char* id,
    const char* label,
    float* value,
    float min,
    float max,
    const char* value_fmt,
    const char* min_label,
    const char* max_label,
    bool compact = false,
    float step = 0.0F);
void SectionLabel(const char* text);

bool HitArea(
    const char* id,
    ImVec2 size,
    bool* hovered = nullptr,
    bool* held = nullptr);
void SetHandCursorIfHovered();

}  // namespace ui
