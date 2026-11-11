#include "setup_common.h"

#include <algorithm>
#include <cmath>

using namespace theme;

namespace setup_ui {

namespace {

// ImGui's default wheel scroll jumps by coarse steps. Own the wheel here and
// ease ScrollY toward a target so setup panels feel continuous on trackpads.
void ApplySmoothPanelScroll() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID target_id = ImGui::GetID("smooth_scroll_target");
    const ImGuiID last_id = ImGui::GetID("smooth_scroll_last");

    const float scroll = ImGui::GetScrollY();
    const float max_scroll = ImGui::GetScrollMaxY();
    float target = storage->GetFloat(target_id, scroll);
    const float last_set = storage->GetFloat(last_id, scroll);

    if (std::fabs(scroll - last_set) > 0.5F && io.MouseWheel == 0.0F) {
        target = scroll;
    }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        io.MouseWheel != 0.0F) {
        constexpr float kPixelsPerWheelUnit = 56.0F;
        target -= io.MouseWheel * kPixelsPerWheelUnit;
    }

    target = std::clamp(target, 0.0F, std::max(max_scroll, 0.0F));
    constexpr float kSmoothing = 16.0F;
    const float blend = 1.0F - std::exp(-kSmoothing * io.DeltaTime);
    const float next = scroll + (target - scroll) * blend;
    ImGui::SetScrollY(next);

    storage->SetFloat(target_id, target);
    storage->SetFloat(last_id, next);
}

float PillRowWidth(const char* left_label, const char* right_label) {
    const float left_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, left_label).x + 24.0F;
    const float right_width =
        ui::TextSize(Face::PlexSemiBold, 10.0F, right_label).x + 24.0F;
    return left_width + 8.0F + right_width;
}

void PlaceBottomStepRow(
    float column_width,
    float column_height,
    float row_width,
    float* out_x,
    float* out_y) {
    constexpr float kRowHeight = 29.0F;
    const ImVec2 window_pos = ImGui::GetWindowPos();
    *out_x = window_pos.x + (column_width - row_width) * 0.5F;
    *out_y = std::max(
        ImGui::GetCursorScreenPos().y + 8.0F,
        window_pos.y + column_height - 20.0F - kRowHeight);
}

}  // namespace

void BeginPanel(const char* id, ImVec2 position, ImVec2 size, ImVec2 padding) {
    ImGui::SetCursorScreenPos(position);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, setup::kPanel);
    ImGui::PushStyleColor(ImGuiCol_Border, setup::kPanelBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(
        id,
        size,
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollWithMouse);
}

void EndPanel() {
    ApplySmoothPanelScroll();
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginGroup(const char* id, bool summary) {
    ui::BeginCard(
        id,
        summary ? setup::kSummary : setup::kGroup,
        summary ? setup::kSummaryBorder : setup::kGroupBorder,
        9.0F,
        ImVec2(10.0F, 9.0F));
}

void EndGroup() {
    ui::EndCard();
}

void PanelHeader(
    const char* title,
    const char* subtitle,
    const char* pill,
    float title_size) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ui::TextAt(draw_list, position, Face::SpaceBold, title_size, setup::kTitle, title);
    const float title_height = LineHeight(Face::SpaceBold, title_size);
    if (pill) {
        const float pill_width =
            ui::TextSize(Face::PlexSemiBold, 10.0F, pill).x + 24.0F;
        ImGui::SetCursorScreenPos(
            ImVec2(position.x + width - pill_width, position.y + 4.5F));
        ui::Pill("##headerpill", pill, true);
    }
    ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + title_height + 3.0F));
    ui::Text(Face::PlexRegular, 9.0F, setup::kMuted, subtitle);
    ui::Space(9.0F);
    ui::Divider(setup::kDivider);
    ui::Space(9.0F);
}

void KV(const char* key, const char* value, bool accent) {
    ui::KeyValue(
        key,
        value,
        accent ? setup::kAccentValue : setup::kKvValue,
        Face::PlexRegular,
        10.0F,
        Face::PlexSemiBold,
        11.0F,
        setup::kMuted);
}

void SummaryTitle(const char* text) {
    ui::Text(Face::PlexSemiBold, 9.0F, Hex(0x646B76), text);
}

int IndexOf(
    const char* const* items,
    int count,
    const std::string& value,
    int fallback) {
    for (int i = 0; i < count; ++i) {
        if (value == items[i]) {
            return i;
        }
    }
    return fallback;
}

bool PillChoice(
    const char* id,
    const char* const* items,
    int count,
    int* selected) {
    bool changed = false;
    ImGui::PushID(id);
    for (int i = 0; i < count; ++i) {
        if (i) {
            ImGui::SameLine(0.0F, 6.0F);
        }
        ImGui::PushID(i);
        if (ui::Pill("##p", items[i], *selected == i) && *selected != i) {
            *selected = i;
            changed = true;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}

void CategoryColumn(
    const char* heading,
    const char* blurb,
    const char* const* titles,
    const char* const* subtitles,
    int count,
    int selected,
    float card_width,
    int* clicked_index) {
    *clicked_index = -1;
    ui::Text(Face::SpaceBold, 17.0F, Hex(0xF3F4F5), heading);
    ui::Space(8.0F);
    ui::Text(Face::PlexRegular, 9.0F, Hex(0x6E7580), blurb);
    ui::Space(8.0F);
    ui::Divider(setup::kDivider);
    ui::Space(8.0F);

    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        if (ui::CategoryCard(
                "##cat",
                titles[i],
                subtitles[i],
                selected == i,
                card_width)) {
            *clicked_index = i;
        }
        ImGui::PopID();
        ui::Space(8.0F);
    }
}

void DrawPrevNextFooter(
    float column_width,
    float column_height,
    const char* id,
    bool* prev_pressed,
    bool* next_pressed) {
    *prev_pressed = false;
    *next_pressed = false;
    const float step_width = PillRowWidth("PREVIOUS", "NEXT");
    float x = 0.0F;
    float y = 0.0F;
    PlaceBottomStepRow(column_width, column_height, step_width, &x, &y);
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    ImGui::PushID(id);
    if (ui::Pill("##prev", "PREVIOUS", false)) {
        *prev_pressed = true;
    }
    ImGui::SameLine(0.0F, 8.0F);
    if (ui::Pill("##next", "NEXT", false)) {
        *next_pressed = true;
    }
    ImGui::PopID();
}

float SetupColumnWidth(float design_width, float available) {
    constexpr float kDesignTotal = 1336.0F;
    return std::floor(design_width * available / kDesignTotal);
}

void RunSimulationAction(App& app) {
    app.GoTo(Screen::Results);
    app.ShowToast(
        "The physics engine isn't connected yet, so this shows sample Run #42.",
        4.0);
}

}  // namespace setup_ui
