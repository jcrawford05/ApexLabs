#include "app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <SDL3/SDL.h>

#include "imgui.h"
#include "theme.h"
#include "widgets.h"

namespace {

std::string NowLabel() {
    SDL_Time time = 0;
    SDL_DateTime date_time{};
    if (!SDL_GetCurrentTime(&time) || !SDL_TimeToDateTime(time, &date_time, true)) {
        return "";
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", date_time.hour, date_time.minute);
    return buffer;
}

void LogActivity(App& app, const std::string& text, bool ok) {
    app.import_log.insert(app.import_log.begin(), {text, ok, NowLabel()});
    if (app.import_log.size() > 32) {
        app.import_log.resize(32);
    }
}

const char* ScreenTitle(Screen screen) {
    switch (screen) {
        case Screen::CarSetup:
            return "Car Setup";
        case Screen::Results:
            return "Results";
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
    SDL_Window* app_window,
    SDL_Renderer* renderer,
    const std::string& base_directory,
    std::string& error) {
    window = app_window;
    import_event = SDL_RegisterEvents(1);
    if (import_event == static_cast<uint32_t>(static_cast<Sint32>(-1))) {
        error = "Could not register the Import file-dialog event.";
        window = nullptr;
        return false;
    }

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
    const ImU32 background =
        (screen == Screen::Start || screen == Screen::CarSetup)
            ? theme::setup::kBg
            : theme::app::kBg;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, background);
    ImGui::Begin(
        "##root",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();

    switch (screen) {
        case Screen::Start:
            DrawStartScreen(*this);
            break;
        case Screen::CarSetup:
            DrawSetupScreen(*this);
            break;
        case Screen::Results:
            DrawResultsScreen(*this);
            break;
        case Screen::ImportExport:
            DrawImportExportScreen(*this);
            break;
        case Screen::Compare:
            DrawCompareScreen(*this);
            break;
        case Screen::Settings:
            DrawPlaceholderScreen(*this);
            break;
    }
    ImGui::End();

    const double now = ImGui::GetTime();
    if (!toast.empty() && now < toast_until) {
        const float alpha = static_cast<float>(
            std::min(1.0, (toast_until - now) / 0.3));
        ImDrawList* foreground = ImGui::GetForegroundDrawList();
        const ImVec2 text_size =
            ui::TextSize(theme::Face::PlexRegular, 13, toast.c_str());
        const ImVec2 toast_size(text_size.x + 32.0F, text_size.y + 20.0F);
        const ImVec2 position(
            viewport->Pos.x + (viewport->Size.x - toast_size.x) * 0.5F,
            viewport->Pos.y + viewport->Size.y - toast_size.y - 28.0F);
        const auto fade = [alpha](ImU32 color) {
            const int source_alpha = static_cast<int>((color >> 24) & 0xFF);
            const int next_alpha =
                static_cast<int>(source_alpha * alpha);
            return (color & 0x00FFFFFF) |
                (static_cast<ImU32>(next_alpha) << 24);
        };
        foreground->AddRectFilled(
            ImVec2(position.x, position.y + 4.0F),
            ImVec2(position.x + toast_size.x, position.y + toast_size.y + 6.0F),
            fade(IM_COL32(0, 0, 0, 120)),
            10.0F);
        foreground->AddRectFilled(
            position,
            ImVec2(position.x + toast_size.x, position.y + toast_size.y),
            fade(theme::setup::kPanel),
            10.0F);
        foreground->AddRect(
            position,
            ImVec2(position.x + toast_size.x, position.y + toast_size.y),
            fade(theme::setup::kPanelBorder),
            10.0F);
        foreground->AddRectFilled(
            ImVec2(position.x, position.y + 10.0F),
            ImVec2(position.x + 3.0F, position.y + toast_size.y - 10.0F),
            fade(theme::kRed),
            2.0F);
        ui::TextAt(
            foreground,
            ImVec2(position.x + 16.0F, position.y + 10.0F),
            theme::Face::PlexRegular,
            13.0F,
            fade(theme::kWhite),
            toast.c_str());
    } else if (!toast.empty() && now >= toast_until) {
        toast.clear();
    }
}

void App::GoTo(Screen destination) {
    if (destination == Screen::Results && screen != Screen::Results) {
        map_coloring = MapColoring::DeltaVsReference;
        open_section = -1;
    }
    screen = destination;
}

void App::ShowToast(const std::string& text, double seconds) {
    toast = text;
    toast_until = ImGui::GetTime() + seconds;
}

void App::SaveSetup() {
    saved_vehicle = vehicle;
    ShowToast("Setup saved.");
}

std::string App::ExportFolder() const {
    char* path = SDL_GetPrefPath("ApexLabs", "ApexLabs");
    if (!path) {
        return {};
    }
    std::string folder = path;
    SDL_free(path);
    return folder;
}

void App::ExportRunJson() {
    char name[64];
    std::snprintf(name, sizeof(name), "run_%d.json", run.run_id);
    ShowToast("JSON export isn't connected yet.");
    LogActivity(*this, std::string("Export of ") + name + " isn't connected yet.", false);
}

void App::ExportCarJson() {
    const std::string name = FileSlug(vehicle.name) + ".json";
    ShowToast("JSON export isn't connected yet.");
    LogActivity(*this, "Export of " + name + " isn't connected yet.", false);
}

void App::ExportTrackJson() {
    const std::string name = FileSlug(track.name) + ".json";
    ShowToast("JSON export isn't connected yet.");
    LogActivity(*this, "Export of " + name + " isn't connected yet.", false);
}

void App::ImportFile(const std::string& path) {
    const std::string file = path.substr(path.find_last_of("/\\") + 1);
    ShowToast("Picked " + file + " — JSON parsing not yet implemented.", 4.0);
    LogActivity(*this, file + ": JSON parsing not yet implemented.", false);
}

void App::OpenImportDialog() {
    static const SDL_DialogFileFilter filters[] = {
        {"Apex Labs JSON", "json"},
    };
    if (import_event == 0) {
        ShowToast("Couldn't open the file picker.");
        return;
    }

    // Callback may run on another thread: only copy the path and push an event.
    SDL_ShowOpenFileDialog(
        [](void* userdata, const char* const* files, int /*filter*/) {
            if (!files || !files[0]) {
                return;
            }
            SDL_Event event{};
            event.type = *static_cast<Uint32*>(userdata);
            event.user.data1 = SDL_strdup(files[0]);
            if (!event.user.data1) {
                return;
            }
            SDL_PushEvent(&event);
        },
        &import_event,
        window,
        filters,
        1,
        nullptr,
        false);
}

const RunSummary* App::FindRun(int run_id) const {
    for (const RunSummary& summary : history) {
        if (summary.run_id == run_id) {
            return &summary;
        }
    }
    return nullptr;
}

void DrawTopNav(App& app, const char* right_label) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    draw_list->AddRectFilled(
        position,
        ImVec2(position.x + width, position.y + 64.0F),
        theme::kNavBg);

    ImGui::SetCursorScreenPos(ImVec2(position.x + 32.0F, position.y + 4.0F));
    if (ui::HitArea("##logo", ImVec2(136.0F, 56.0F))) {
        app.GoTo(Screen::Start);
    }
    ui::SetHandCursorIfHovered();
    if (app.logo_wordmark) {
        constexpr float kScale = 136.4F / 507.0F;
        const ImVec2 image(
            position.x + 32.0F + 34.0F * kScale,
            position.y - 2.3F);
        draw_list->AddImage(
            app.logo_wordmark.id(),
            image,
            ImVec2(image.x + 473.0F * kScale, image.y + 191.0F * kScale));
    }

    struct Tab {
        const char* label;
        Screen screen;
    };
    const Tab tabs[] = {
        {"Car Setup", Screen::CarSetup},
        {"Results", Screen::Results},
        {"Import / Export", Screen::ImportExport},
        {"Comparison", Screen::Compare},
        {"Settings", Screen::Settings},
    };
    constexpr int kTabCount = static_cast<int>(sizeof(tabs) / sizeof(tabs[0]));

    float total = 0.0F;
    float widths[kTabCount];
    for (int i = 0; i < kTabCount; ++i) {
        widths[i] =
            ui::TextSize(theme::Face::PlexSemiBold, 14, tabs[i].label).x + 4.0F;
        total += widths[i] + (i ? 28.0F : 0.0F);
    }

    float x = position.x + (width - total) * 0.5F;
    for (int i = 0; i < kTabCount; ++i) {
        const Tab& tab = tabs[i];
        const bool active = app.screen == tab.screen;
        ImGui::SetCursorScreenPos(ImVec2(x, position.y));
        ImGui::PushID(i);
        bool hovered = false;
        if (ui::HitArea("##tab", ImVec2(widths[i], 64.0F), &hovered)) {
            app.GoTo(tab.screen);
        }
        ui::SetHandCursorIfHovered();
        ImGui::PopID();

        const ImU32 color = active
            ? theme::kNavActive
            : (hovered ? theme::Hex(0xC9C9D0) : theme::kNavIdle);
        ui::TextAt(
            draw_list,
            ImVec2(x + 2.0F, position.y + 22.0F),
            active ? theme::Face::PlexSemiBold : theme::Face::PlexRegular,
            14.0F,
            color,
            tab.label);
        if (active) {
            draw_list->AddRectFilled(
                ImVec2(x + 2.0F, position.y + 61.0F),
                ImVec2(x + widths[i] - 2.0F, position.y + 64.0F),
                theme::kRed);
        }
        x += widths[i] + 28.0F;
    }

    const ImVec2 label_size =
        ui::TextSize(theme::Face::PlexRegular, 13, right_label);
    ui::TextAt(
        draw_list,
        ImVec2(position.x + width - 32.0F - label_size.x, position.y + 24.0F),
        theme::Face::PlexRegular,
        13.0F,
        theme::kNavIdle,
        right_label);

    ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + 64.0F));
}
