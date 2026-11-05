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

void Space(float height);
void Divider(ImU32 color, float thickness = 1.0F);

void IconPlus(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconImport(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconSwap(ImDrawList* draw_list, ImVec2 center, float size, ImU32 color);
void IconArrowRight(
    ImDrawList* draw_list,
    ImVec2 center,
    float size,
    ImU32 color,
    float thickness = 2.0F);

bool CategoryCard(
    const char* id,
    const char* title,
    const char* subtitle,
    bool selected,
    float width);

bool HitArea(
    const char* id,
    ImVec2 size,
    bool* hovered = nullptr,
    bool* held = nullptr);
void SetHandCursorIfHovered();

}  // namespace ui
