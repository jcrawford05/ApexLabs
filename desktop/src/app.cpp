#include "app.h"

#include "imgui.h"
#include "theme.h"

namespace {

const char* ScreenTitle(Screen screen) {
    switch (screen) {
        case Screen::CarSetup:
            return "Car Setup";
        case Screen::ImportExport:
            return "Import / Export";
        case Screen::Compare:
            return "Comparison";
        case Screen::Settings:
            return "Settings";
        case Screen::Start:
            return "Start";
    }
    return "Apex Lab";
}

void DrawPlaceholderScreen(App& app) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();

    draw_list->AddRectFilled(
        origin,
        ImVec2(origin.x + size.x, origin.y + 2.0F),
        theme::kRed);
    draw_list->AddText(
        theme::Font(theme::Face::SpaceBold, 24),
        theme::LineHeight(theme::Face::SpaceBold, 24),
        ImVec2(origin.x + 64.0F, origin.y + 72.0F),
        theme::kWhite,
        ScreenTitle(app.screen));
    draw_list->AddText(
        theme::Font(theme::Face::PlexRegular, 14),
        theme::LineHeight(theme::Face::PlexRegular, 14),
        ImVec2(origin.x + 64.0F, origin.y + 118.0F),
        theme::setup::kTagline,
        "This screen has not yet been migrated.");

    ImGui::SetCursorScreenPos(ImVec2(origin.x + 64.0F, origin.y + 164.0F));
    ImGui::PushFont(theme::Font(theme::Face::PlexSemiBold, 14));
    if (ImGui::Button("Back to Home", ImVec2(160.0F, 44.0F))) {
        app.GoTo(Screen::Start);
    }
    ImGui::PopFont();
}

}  // namespace

bool App::Init(
    SDL_Renderer* renderer,
    const std::string& base_directory,
    std::string& error) {
    const std::string images = base_directory + "assets/images/";
    logo_wordmark = LoadImage(renderer, images + "logo_wordmark.png");
    logo_badge = LoadImage(renderer, images + "logo_badge.png");
    track_line_left = LoadImage(renderer, images + "track_line_left.png");
    track_line_right = LoadImage(renderer, images + "track_line_right.png");

    if (!logo_wordmark || !logo_badge || !track_line_left || !track_line_right) {
        error = "Could not load the required Apex Lab Start Screen artwork.";
        Shutdown();
        return false;
    }
    return true;
}

void App::Shutdown() {
    FreeImage(logo_wordmark);
    FreeImage(logo_badge);
    FreeImage(track_line_left);
    FreeImage(track_line_right);
}

void App::Frame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::setup::kBg);
    ImGui::Begin(
        "##root",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();

    if (screen == Screen::Start) {
        DrawStartScreen(*this);
    } else {
        DrawPlaceholderScreen(*this);
    }
    ImGui::End();
}

void App::GoTo(Screen destination) {
    screen = destination;
}
