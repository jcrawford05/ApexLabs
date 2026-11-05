// Import / Export (no Figma frame yet; uses the Results / app palette).
// Adapted from ApexLabsDevSpace import_export_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. Uses Space Grotesk / IBM Plex Sans.
// File I/O is stubbed until JSON import/export lands; the UI binds live App state.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

// Dashed rounded-ish rectangle for the drop zone.
void DashedRect(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float dash = 8, float gap = 6) {
    auto line = [&](ImVec2 p, ImVec2 q) {
        float len = std::hypot(q.x - p.x, q.y - p.y);
        ImVec2 d((q.x - p.x) / len, (q.y - p.y) / len);
        for (float t = 0; t < len; t += dash + gap) {
            float e = std::min(len, t + dash);
            dl->AddLine(ImVec2(p.x + d.x * t, p.y + d.y * t), ImVec2(p.x + d.x * e, p.y + d.y * e), col, 1.5f);
        }
    };
    line(a, ImVec2(b.x, a.y));
    line(ImVec2(b.x, a.y), b);
    line(b, ImVec2(a.x, b.y));
    line(ImVec2(a.x, b.y), a);
}

void LoadedRow(const char* kind, const std::string& name, const char* note) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ui::TextAt(dl, ImVec2(p.x, p.y), Face::PlexSemiBold, 13, app::kMuted, kind);
    ui::TextAt(dl, ImVec2(p.x + 70, p.y), Face::PlexSemiBold, 13, app::kText, name.c_str());
    if (note) {
        float nw = ui::TextSize(Face::PlexRegular, 12, note).x;
        ui::TextAt(dl, ImVec2(p.x + w - nw, p.y + 1), Face::PlexRegular, 12, app::kMuted, note);
    }
    ImGui::Dummy(ImVec2(w, LineHeight(Face::PlexSemiBold, 13)));
}

// One export option: title, description, target file name, and a button.
bool ExportRow(const char* id, const char* title, const std::string& detail, const std::string& file, bool last = false) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 66;
    ui::TextAt(dl, ImVec2(p.x, p.y + 12), Face::PlexSemiBold, 14, app::kText, title);
    ui::TextAt(dl, ImVec2(p.x, p.y + 32), Face::PlexRegular, 12, app::kMuted, detail.c_str());
    const float bw = ui::TextSize(Face::PlexSemiBold, 14, "Export").x + 34;
    float fw = ui::TextSize(Face::PlexSemiBold, 12, file.c_str()).x;
    ui::TextAt(dl, ImVec2(p.x + w - bw - 14 - fw, p.y + 24), Face::PlexSemiBold, 12, app::kTextSoft, file.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x + w - bw, p.y + (h - ui::TextSize(Face::PlexSemiBold, 14, "E").y - 22) * 0.5f));
    bool pressed = ui::Button(id, "Export", ui::ButtonKind::Outline);
    if (!last) dl->AddRectFilled(ImVec2(p.x, p.y + h - 1), ImVec2(p.x + w, p.y + h), app::kCardBorder);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h));
    ImGui::Dummy(ImVec2(w, 0));
    return pressed;
}

}  // namespace

void DrawImportExportScreen(App& app) {
    DrawTopNav(app, "Files are JSON, schema v1");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 24));
    ImGui::BeginChild("##importexport", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = ImGui::GetContentRegionAvail().x;

    // Header
    {
        ImVec2 h = ImGui::GetCursorScreenPos();
        ui::TextAt(dl, h, Face::SpaceBold, 28, app::kText, "Import / Export");
        ui::TextAt(dl, ImVec2(h.x, h.y + 40), Face::PlexRegular, 13, app::kMuted,
                   "Bring in cars, tracks and runs from teammates, or save yours to share.");
        const char* open = "Open data folder";
        float ow = ui::TextSize(Face::PlexSemiBold, 14, open).x + 34;
        ImGui::SetCursorScreenPos(ImVec2(h.x + W - ow, h.y + 9));
        if (ui::Button("##openfolder", open, ui::ButtonKind::Outline)) {
            std::string url = "file://" + app.ExportFolder();
            if (!SDL_OpenURL(url.c_str())) app.ShowToast("Couldn't open " + app.ExportFolder());
        }
        ImGui::SetCursorScreenPos(ImVec2(h.x, h.y + 56));
        ImGui::Dummy(ImVec2(W, 0));
    }
    ui::Space(20);

    const float gap = 20, col_w = std::floor((W - gap) * 0.5f);
    ImVec2 row = ImGui::GetCursorScreenPos();

    // Import
    ui::BeginCard("##import", app::kCard, app::kCardBorder, 12, ImVec2(24, 20), col_w);
    {
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Import");
        ui::Space(4);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Car, track or run files. The type is detected from the file.");
        ui::Space(16);
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        ImVec2 z = ImGui::GetCursorScreenPos();
        const float zw = ImGui::GetContentRegionAvail().x, zh = 196;
        bool hovered = false;
        bool clicked = ui::HitArea("##dropzone", ImVec2(zw, zh), &hovered);
        ui::SetHandCursorIfHovered();
        cdl->AddRectFilled(z, ImVec2(z.x + zw, z.y + zh), hovered ? Hex(0x1C1C20) : app::kBg, 10);
        DashedRect(cdl, ImVec2(z.x + 1, z.y + 1), ImVec2(z.x + zw - 1, z.y + zh - 1), hovered ? kRed : app::kControlBorder);
        const ImVec2 ic(z.x + zw * 0.5f, z.y + 58);
        cdl->AddCircleFilled(ic, 24, hovered ? Hex(0x341311) : app::kRaised, 32);
        ui::IconImport(cdl, ic, 18, hovered ? Hex(0xFFB2AE) : app::kTextSoft);
        const char* t1 = "Drop a .json file here";
        const char* t2 = "or click to choose a file";
        ImVec2 s1 = ui::TextSize(Face::PlexSemiBold, 14, t1), s2 = ui::TextSize(Face::PlexRegular, 12, t2);
        ui::TextAt(cdl, ImVec2(ic.x - s1.x * 0.5f, z.y + 100), Face::PlexSemiBold, 14, app::kText, t1);
        ui::TextAt(cdl, ImVec2(ic.x - s2.x * 0.5f, z.y + 122), Face::PlexRegular, 12, app::kMuted, t2);
        const char* t3 = "Accepts: cars  \xC2\xB7  tracks  \xC2\xB7  runs";
        ImVec2 s3 = ui::TextSize(Face::PlexSemiBold, 11, t3);
        ui::TextAt(cdl, ImVec2(ic.x - s3.x * 0.5f, z.y + 156), Face::PlexSemiBold, 11, app::kMuted, t3);
        if (clicked) app.OpenImportDialog();
        ImGui::SetCursorScreenPos(ImVec2(z.x, z.y + zh));
        ImGui::Dummy(ImVec2(zw, 0));
        ui::Space(20);

        ui::Text(Face::PlexSemiBold, 11, app::kMuted, "LOADED NOW");
        ui::Space(10);
        LoadedRow("Car", app.vehicle.name, nullptr);
        ui::Space(8);
        char track_note[48];
        std::snprintf(track_note, sizeof(track_note), "%.2f km, %zu sections", app.track.LengthM() / 1000,
                      app.track.segments.size());
        LoadedRow("Track", app.track.name, track_note);
        ui::Space(8);
        std::string run = "Run #" + std::to_string(app.run.run_id);
        LoadedRow("Run", run, app.run.sample_data ? "Sample data" : nullptr);
    }
    ui::EndCard();
    const float import_h = ImGui::GetItemRectSize().y;

    // Export
    ImGui::SetCursorScreenPos(ImVec2(row.x + col_w + gap, row.y));
    ui::BeginCard("##export", app::kCard, app::kCardBorder, 12, ImVec2(24, 20), W - col_w - gap);
    {
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Export");
        ui::Space(4);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Saves a copy into the data folder. Nothing is uploaded.");
        ui::Space(6);
        char detail[128];
        std::snprintf(detail, sizeof(detail), "%s  \xC2\xB7  %.0f kg  \xC2\xB7  %s tires", app.vehicle.name.c_str(),
                      app.vehicle.TotalMassKg(), app.vehicle.SelectedTire().short_name);
        if (ExportRow("##excar", "Car setup", detail, FileSlug(app.vehicle.name) + ".json")) app.ExportCarJson();
        std::snprintf(detail, sizeof(detail), "%s  \xC2\xB7  %.2f km  \xC2\xB7  %d corners", app.track.name.c_str(),
                      app.track.LengthM() / 1000, app.track.Corners());
        if (ExportRow("##extrack", "Track", detail, FileSlug(app.track.name) + ".json")) app.ExportTrackJson();
        std::snprintf(detail, sizeof(detail), "Run #%d  \xC2\xB7  %s  \xC2\xB7  %s", app.run.run_id,
                      FormatLapTime(app.run.lap_time_s).c_str(), app.prefs.telemetry_in_exports ? "with telemetry" : "times only");
        char runfile[32];
        std::snprintf(runfile, sizeof(runfile), "run_%d.json", app.run.run_id);
        if (ExportRow("##exrun", "Run results", detail, runfile, true)) app.ExportRunJson();
        ui::Space(10);
        ImVec2 t = ImGui::GetCursorScreenPos();
        const float tw = ImGui::GetContentRegionAvail().x;
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        cdl->AddRectFilled(t, ImVec2(t.x + tw, t.y + 48), app::kRaised, 8);
        ui::TextAt(cdl, ImVec2(t.x + 14, t.y + 8), Face::PlexSemiBold, 13, app::kText, "Include telemetry in run exports");
        ui::TextAt(cdl, ImVec2(t.x + 14, t.y + 27), Face::PlexRegular, 11, app::kMuted,
                   "Same as the Settings option. Makes files larger.");
        ImGui::SetCursorScreenPos(ImVec2(t.x + tw - 14 - 40, t.y + 13));
        ui::Toggle("##tel", &app.prefs.telemetry_in_exports);
        ImGui::SetCursorScreenPos(ImVec2(t.x, t.y + 48));
        ImGui::Dummy(ImVec2(tw, 0));
        ui::Space(12);
        std::string where = "Folder: " + app.ExportFolder();
        ui::TextWrapped(Face::PlexRegular, 11, app::kMuted, where.c_str());
    }
    ui::EndCard();
    const float export_h = ImGui::GetItemRectSize().y;
    ImGui::SetCursorScreenPos(ImVec2(row.x, row.y + std::max(import_h, export_h)));
    ImGui::Dummy(ImVec2(W, 0));
    ui::Space(20);

    // Run history + activity
    row = ImGui::GetCursorScreenPos();
    const float hist_w = std::floor((W - gap) * 0.62f);
    ui::BeginCard("##history", app::kCard, app::kCardBorder, 12, ImVec2(24, 20), hist_w);
    {
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Run history");
        ui::Space(4);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Sample runs on this track. Compare any of them with the latest run.");
        ui::Space(14);
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        const float w = ImGui::GetContentRegionAvail().x;
        const float cols[] = {0, 70, 170, 400, 520};
        const char* heads[] = {"RUN", "DATE", "CAR", "WEATHER", "LAP"};
        ImVec2 p = ImGui::GetCursorScreenPos();
        for (int i = 0; i < 5; ++i) ui::TextAt(cdl, ImVec2(p.x + cols[i] + 10, p.y), Face::PlexSemiBold, 11, app::kMuted, heads[i]);
        ImGui::Dummy(ImVec2(w, 22));
        if (app.history.empty()) ui::Text(Face::PlexRegular, 13, app::kMuted, "No saved runs yet.");
        const int latest = app.history.empty() ? 0 : app.history.front().run_id;
        for (const RunSummary& r : app.history) {
            ImVec2 q = ImGui::GetCursorScreenPos();
            const float h = 40;
            ImGui::PushID(r.run_id);
            bool hovered = ImGui::IsMouseHoveringRect(q, ImVec2(q.x + w, q.y + h));
            if (hovered) cdl->AddRectFilled(q, ImVec2(q.x + w, q.y + h), app::kRaised, 6);
            char id[16];
            std::snprintf(id, sizeof(id), "#%d", r.run_id);
            ui::TextAt(cdl, ImVec2(q.x + cols[0] + 10, q.y + 12), Face::PlexSemiBold, 13, app::kText, id);
            ui::TextAt(cdl, ImVec2(q.x + cols[1] + 10, q.y + 12), Face::PlexRegular, 13, app::kMuted, r.date.c_str() + 5);
            ui::TextAt(cdl, ImVec2(q.x + cols[2] + 10, q.y + 12), Face::PlexSemiBold, 13, app::kText, r.vehicle_name.c_str());
            ui::TextAt(cdl, ImVec2(q.x + cols[3] + 10, q.y + 12), Face::PlexRegular, 13, app::kMuted, r.weather_preset.c_str());
            std::string lap = FormatLapTime(r.lap_time_s);
            ui::TextAt(cdl, ImVec2(q.x + cols[4] + 10, q.y + 12), Face::PlexSemiBold, 13, app::kText, lap.c_str());
            if (r.run_id == latest) {
                const char* tag = "Latest";
                float tw = ui::TextSize(Face::PlexSemiBold, 11, tag).x;
                ui::TextAt(cdl, ImVec2(q.x + w - 10 - tw, q.y + 13), Face::PlexSemiBold, 11, app::kMuted, tag);
            } else {
                float bw = ui::TextSize(Face::PlexSemiBold, 12, "Compare").x + 24;
                ImGui::SetCursorScreenPos(ImVec2(q.x + w - 8 - bw, q.y + 6));
                if (ui::Button("##cmp", "Compare", ui::ButtonKind::Subtle, 12, 12, 6)) {
                    app.compare_a = latest;
                    app.compare_b = r.run_id;
                    app.GoTo(Screen::Compare);
                }
            }
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(q.x, q.y + h));
            ImGui::Dummy(ImVec2(w, 2));
        }
    }
    ui::EndCard();
    const float hist_h = ImGui::GetItemRectSize().y;

    ImGui::SetCursorScreenPos(ImVec2(row.x + hist_w + gap, row.y));
    ui::BeginCard("##activity", app::kCard, app::kCardBorder, 12, ImVec2(24, 20), W - hist_w - gap);
    {
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Activity");
        ui::Space(4);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Imports and exports in this session.");
        ui::Space(14);
        if (app.import_log.empty()) {
            ui::Text(Face::PlexRegular, 13, app::kMuted, "Nothing imported or exported yet.");
        }
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        const float w = ImGui::GetContentRegionAvail().x;
        int shown = 0;
        for (const App::LogEntry& e : app.import_log) {
            if (++shown > 8) break;
            ImVec2 q = ImGui::GetCursorScreenPos();
            ui::StatusDot(cdl, ImVec2(q.x + 5, q.y + 8), 4, e.ok ? app::kGained : app::kLost);
            float tw = ui::TextSize(Face::PlexRegular, 11, e.when.c_str()).x;
            ui::TextAt(cdl, ImVec2(q.x + w - tw, q.y + 1), Face::PlexRegular, 11, app::kMuted, e.when.c_str());
            ImGui::SetCursorScreenPos(ImVec2(q.x + 18, q.y));
            ImGui::PushTextWrapPos(q.x + w - tw - 12);
            ImGui::PushFont(Font(Face::PlexRegular, 12));
            ImGui::PushStyleColor(ImGuiCol_Text, e.ok ? app::kTextSoft : app::kLost);
            ImGui::TextWrapped("%s", e.text.c_str());
            ImGui::PopStyleColor();
            ImGui::PopFont();
            ImGui::PopTextWrapPos();
            ui::Space(10);
        }
    }
    ui::EndCard();
    const float act_h = ImGui::GetItemRectSize().y;
    ImGui::SetCursorScreenPos(ImVec2(row.x, row.y + std::max(hist_h, act_h)));
    ImGui::Dummy(ImVec2(W, 0));

    ImGui::EndChild();
    ImGui::PopStyleVar();
}
