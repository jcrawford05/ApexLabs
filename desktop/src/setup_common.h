// Shared layout helpers for Car Setup and Track Setup screens.
#pragma once

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

#include <string>

namespace setup_ui {

using theme::Face;

void BeginPanel(const char* id, ImVec2 position, ImVec2 size, ImVec2 padding);
void EndPanel();
void BeginGroup(const char* id, bool summary = false);
void EndGroup();
void PanelHeader(
    const char* title,
    const char* subtitle,
    const char* pill,
    float title_size);
void KV(const char* key, const char* value, bool accent = false);
void SummaryTitle(const char* text);
int IndexOf(
    const char* const* items,
    int count,
    const std::string& value,
    int fallback);
bool PillChoice(
    const char* id,
    const char* const* items,
    int count,
    int* selected);
void CategoryColumn(
    const char* heading,
    const char* blurb,
    const char* const* titles,
    const char* const* subtitles,
    int count,
    int selected,
    float card_width,
    int* clicked_index);
void DrawPrevNextFooter(
    float column_width,
    float column_height,
    const char* id,
    bool* prev_pressed,
    bool* next_pressed);
float SetupColumnWidth(float design_width, float available);
void RunSimulationAction(App& app);

}  // namespace setup_ui
