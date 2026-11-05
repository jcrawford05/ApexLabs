// Comparison Tool (no Figma frame yet; follows the Results page layout).
// Adapted from ApexLabsDevSpace compare_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. Uses Space Grotesk / IBM Plex Sans.
// Two runs from history side by side. Every difference is Run A minus Run B,
// so a negative number means Run A was faster there.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "widgets.h"

using namespace theme;

namespace {

ImU32 GapColor(float gap, float band) {
    float d = std::round(gap * 100.0f) / 100.0f;
    if (d <= -band + 1e-4f) return app::kGained;
    if (d >= band - 1e-4f) return app::kLost;
    return app::kNeutral;
}

std::string RunLabel(const RunSummary& r) {
    return "Run #" + std::to_string(r.run_id) + "  \xC2\xB7  " + r.vehicle_name + "  \xC2\xB7  " + FormatLapTime(r.lap_time_s);
}

// Card with a big gap value, the two raw values under it.
void GapCard(const char* id, const char* label, const std::string& gap, ImU32 gap_color, const std::string& detail,
             float width) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12, ImVec2(18, 16), width);
    ui::Text(Face::PlexSemiBold, 11, app::kMuted, label);
    ui::Space(6);
    ui::Text(Face::SpaceBold, 28, gap_color, gap.c_str());
    ui::Space(6);
    ui::Text(Face::PlexRegular, 12, app::kMuted, detail.c_str());
    ui::EndCard();
}

// Running time gap (A − B) at each telemetry sample. Section times are exact at
// section ends, so the gap is built from them and spread linearly across each
// section.
std::vector<float> GapTrace(const Track& track, const RunSummary& a, const RunSummary& b) {
    std::vector<float> gap;
    const size_t samples = std::min(a.speed_kmh.size(), b.speed_kmh.size());
    const size_t sections = std::min({a.section_times_s.size(), b.section_times_s.size(), track.segments.size()});
    const float ds = a.telemetry_spacing_m;
    gap.reserve(samples);
    for (size_t i = 0; i < samples; ++i) {
        const float d = i * ds;
        float before = 0, g = 0;
        for (size_t k = 0; k < sections; ++k) {
            const float start = track.SegmentStartM(int(k)), len = track.segments[k].length_m;
            const float delta = a.section_times_s[k] - b.section_times_s[k];
            if (d < start + len || k + 1 == sections) {
                g = before + delta * std::clamp((d - start) / len, 0.0f, 1.0f);
                break;
            }
            before += delta;
        }
        gap.push_back(g);
    }
    return gap;
}

void SetupDiffTable(const RunSummary& a, const RunSummary& b, bool only_diffs) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = ImGui::GetContentRegionAvail().x;
    const float col_a = w * 0.46f, col_b = w * 0.73f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ui::TextAt(dl, ImVec2(p.x + 12, p.y), Face::PlexSemiBold, 11, app::kMuted, "SETTING");
    char ha[24], hb[24];
    std::snprintf(ha, sizeof(ha), "RUN #%d", a.run_id);
    std::snprintf(hb, sizeof(hb), "RUN #%d", b.run_id);
    ui::TextAt(dl, ImVec2(p.x + col_a, p.y), Face::PlexSemiBold, 11, kRed, ha);
    ui::TextAt(dl, ImVec2(p.x + col_b, p.y), Face::PlexSemiBold, 11, app::kMuted, hb);
    ImGui::Dummy(ImVec2(w, 22));
    int shown = 0;
    for (const auto& [key, va] : a.setup) {
        if (key == "Car") continue;  // the car name is in the run pickers already
        std::string vb = "\xE2\x80\x94";
        for (const auto& [kb, v] : b.setup)
            if (kb == key) vb = v;
        const bool differs = va != vb;
        if (only_diffs && !differs) continue;
        ImVec2 r = ImGui::GetCursorScreenPos();
        const float h = 34;
        if (differs) {
            dl->AddRectFilled(r, ImVec2(r.x + w, r.y + h), app::kRaised, 6);
            dl->AddRectFilled(ImVec2(r.x, r.y + 9), ImVec2(r.x + 3, r.y + h - 9), kRed, 2);
        }
        ui::TextAt(dl, ImVec2(r.x + 12, r.y + 9), Face::PlexSemiBold, 13, differs ? app::kText : app::kMuted, key.c_str());
        ui::TextAt(dl, ImVec2(r.x + col_a, r.y + 9), differs ? Face::PlexSemiBold : Face::PlexSemiBold, 13,
                   differs ? app::kText : app::kMuted, va.c_str());
        ui::TextAt(dl, ImVec2(r.x + col_b, r.y + 9), Face::PlexSemiBold, 13, differs ? app::kTextSoft : app::kMuted,
                   vb.c_str());
        ImGui::Dummy(ImVec2(w, h));
        ui::Space(2);
        ++shown;
    }
    if (shown == 0) {
        ui::Space(8);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Both runs used the same setup.");
    }
}

// Diverging bars: one row per section, centred on zero.
int SectionGaps(App& app, const RunSummary& a, const RunSummary& b, int highlight) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Track& track = app.track;
    const float w = ImGui::GetContentRegionAvail().x;
    const float band = app.prefs.neutral_band_s;
    const size_t n = std::min({a.section_times_s.size(), b.section_times_s.size(), track.segments.size()});
    float max_abs = 0.05f;
    for (size_t i = 0; i < n; ++i) max_abs = std::max(max_abs, std::fabs(a.section_times_s[i] - b.section_times_s[i]));
    const float name_w = 150, value_w = 64;
    const float bar_x0 = name_w + 8, bar_w = w - name_w - value_w - 16;
    const float mid = bar_x0 + bar_w * 0.5f;
    int hovered_row = -1;

    ImVec2 top = ImGui::GetCursorScreenPos();
    for (size_t i = 0; i < n; ++i) {
        const float gap = a.section_times_s[i] - b.section_times_s[i];
        ImVec2 r = ImGui::GetCursorScreenPos();
        const float h = 26;
        ImGui::PushID(int(i));
        bool hovered = false;
        ui::HitArea("##row", ImVec2(w, h), &hovered);
        ImGui::PopID();
        if (hovered) hovered_row = int(i);
        if (hovered || highlight == int(i)) dl->AddRectFilled(r, ImVec2(r.x + w, r.y + h), app::kRaised, 6);
        char label[64];
        std::snprintf(label, sizeof(label), "%zu  %s", i + 1, track.segments[i].name.c_str());
        ui::TextAt(dl, ImVec2(r.x + 8, r.y + 6), Face::PlexSemiBold, 12, app::kTextSoft, label);
        const float len = bar_w * 0.5f * std::fabs(gap) / max_abs;
        const ImU32 c = GapColor(gap, band);
        if (gap < 0) dl->AddRectFilled(ImVec2(r.x + mid - len, r.y + 8), ImVec2(r.x + mid, r.y + h - 8), c, 3);
        else dl->AddRectFilled(ImVec2(r.x + mid, r.y + 8), ImVec2(r.x + mid + std::max(len, 1.0f), r.y + h - 8), c, 3);
        std::string d = FormatDelta(gap);
        float dw = ui::TextSize(Face::PlexSemiBold, 12, d.c_str()).x;
        ui::TextAt(dl, ImVec2(r.x + w - 8 - dw, r.y + 6), Face::PlexSemiBold, 12, c, d.c_str());
        ImGui::Dummy(ImVec2(w, 0));
    }
    ImVec2 bottom = ImGui::GetCursorScreenPos();
    dl->AddLine(ImVec2(top.x + mid, top.y - 4), ImVec2(top.x + mid, bottom.y + 2), app::kControlBorder, 1);
    ui::Space(8);
    ImVec2 lp = ImGui::GetCursorScreenPos();
    const char* faster = "\xE2\x86\x90 Run A faster";
    const char* slower = "Run A slower \xE2\x86\x92";
    float fw = ui::TextSize(Face::PlexRegular, 11, faster).x;
    ui::TextAt(dl, ImVec2(lp.x + mid - 10 - fw, lp.y), Face::PlexRegular, 11, app::kMuted, faster);
    ui::TextAt(dl, ImVec2(lp.x + mid + 10, lp.y), Face::PlexRegular, 11, app::kMuted, slower);
    ImGui::Dummy(ImVec2(w, LineHeight(Face::PlexRegular, 11)));
    return hovered_row;
}

// Speed of both runs (top) and the running gap (bottom), with sector bands.
int SpeedAndGapChart(App& app, const RunSummary& a, const RunSummary& b, ImVec2 p, ImVec2 size, int highlight) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Track& track = app.track;
    const float lap_m = track.LengthM();
    const float left = 44;
    const float speed_h = size.y * 0.58f, gap_top = p.y + speed_h + 22, gap_h = size.y - speed_h - 22 - 30;
    const float x0 = p.x + left, x1 = p.x + size.x;
    auto X = [&](float d) { return x0 + d / lap_m * (x1 - x0); };

    // Sector band and highlighted section
    float pos = 0, s2_start = 0, s2_end = 0;
    for (const Segment& s : track.segments) {
        if (s.sector == 2 && s2_end == 0 && s2_start == 0) s2_start = pos;
        pos += s.length_m;
        if (s.sector == 2) s2_end = pos;
    }
    dl->AddRectFilled(ImVec2(X(s2_start), p.y), ImVec2(X(s2_end), gap_top + gap_h), Hex(0xFFFFFF, 8));
    if (highlight >= 0) {
        float s0 = track.SegmentStartM(highlight), s1 = s0 + track.segments[highlight].length_m;
        dl->AddRectFilled(ImVec2(X(s0), p.y), ImVec2(X(s1), gap_top + gap_h), Hex(0xE10600, 40));
    }

    // Speed panel
    auto Ys = [&](float v) { return p.y + speed_h - v / 350.0f * speed_h; };
    for (float v : {100.0f, 200.0f, 300.0f}) {
        dl->AddLine(ImVec2(x0, Ys(v)), ImVec2(x1, Ys(v)), Hex(0x2A2A30), 1);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f", v);
        ui::TextAt(dl, ImVec2(p.x, Ys(v) - 7), Face::PlexRegular, 10, app::kMuted, buf);
    }
    ui::TextAt(dl, ImVec2(p.x, p.y + speed_h - 8), Face::PlexRegular, 10, app::kMuted, "km/h");
    auto trace = [&](const RunSummary& r, ImU32 col, float thick) {
        std::vector<ImVec2> pts;
        for (size_t i = 0; i < r.speed_kmh.size(); ++i) pts.push_back(ImVec2(X(i * r.telemetry_spacing_m), Ys(r.speed_kmh[i])));
        dl->AddPolyline(pts.data(), int(pts.size()), col, 0, thick);
    };
    trace(b, Hex(0x9A9AA3), 1.5f);
    trace(a, kRed, 2.0f);

    // Gap panel
    const std::vector<float> gap = GapTrace(track, a, b);
    float g_max = 0.1f;
    for (float g : gap) g_max = std::max(g_max, std::fabs(g));
    g_max = std::ceil(g_max * 10.0f) / 10.0f;
    const float zero = gap_top + gap_h * 0.5f;
    auto Yg = [&](float g) { return zero - g / g_max * gap_h * 0.5f; };
    dl->AddLine(ImVec2(x0, zero), ImVec2(x1, zero), app::kControlBorder, 1);
    char gl[24];
    std::snprintf(gl, sizeof(gl), "+%.1f s", g_max);
    ui::TextAt(dl, ImVec2(p.x, gap_top - 6), Face::PlexRegular, 10, app::kMuted, gl);
    std::snprintf(gl, sizeof(gl), "\xE2\x88\x92%.1f s", g_max);
    ui::TextAt(dl, ImVec2(p.x, gap_top + gap_h - 8), Face::PlexRegular, 10, app::kMuted, gl);
    ui::TextAt(dl, ImVec2(p.x, zero - 7), Face::PlexRegular, 10, app::kMuted, "0");
    const ImDrawListFlags saved_flags = dl->Flags;
    dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;  // adjacent quads, so no seams
    for (size_t i = 1; i < gap.size(); ++i) {
        float xa = X((i - 1) * a.telemetry_spacing_m), xb = X(i * a.telemetry_spacing_m);
        ImU32 c = gap[i - 1] + gap[i] > 0 ? Hex(0xFF5A36, 70) : Hex(0x2ECC71, 70);
        dl->AddQuadFilled(ImVec2(xa, zero), ImVec2(xa, Yg(gap[i - 1])), ImVec2(xb, Yg(gap[i])), ImVec2(xb, zero), c);
    }
    dl->Flags = saved_flags;
    {
        std::vector<ImVec2> pts;
        for (size_t i = 0; i < gap.size(); ++i) pts.push_back(ImVec2(X(i * a.telemetry_spacing_m), Yg(gap[i])));
        dl->AddPolyline(pts.data(), int(pts.size()), app::kText, 0, 1.5f);
    }
    ui::TextAt(dl, ImVec2(x0 + 6, gap_top - 16), Face::PlexSemiBold, 11, app::kMuted, "GAP, RUN A \xE2\x88\x92 RUN B (above zero: A behind)");

    // Distance axis
    for (float d = 0; d <= lap_m + 1; d += 1000) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f m", d);
        float tw = ui::TextSize(Face::PlexRegular, 10, buf).x;
        float x = X(d) - (d > 0 ? tw * (d >= lap_m - 1 ? 1.0f : 0.5f) : 0);
        ui::TextAt(dl, ImVec2(x, gap_top + gap_h + 12), Face::PlexRegular, 10, app::kMuted, buf);
    }

    // Hover read-out
    int hovered_section = -1;
    ImGui::SetCursorScreenPos(ImVec2(x0, p.y));
    bool hovered = false;
    ui::HitArea("##cmpchart", ImVec2(x1 - x0, gap_top + gap_h - p.y), &hovered);
    if (hovered && !gap.empty()) {
        float mx = std::clamp(ImGui::GetIO().MousePos.x, x0, x1);
        float d = (mx - x0) / (x1 - x0) * lap_m;
        size_t i = std::min(gap.size() - 1, size_t(d / a.telemetry_spacing_m + 0.5f));
        for (int k = 0; k < int(track.segments.size()); ++k)
            if (d >= track.SegmentStartM(k)) hovered_section = k;
        dl->AddLine(ImVec2(mx, p.y), ImVec2(mx, gap_top + gap_h), Hex(0xF4F4F5, 120), 1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kRaised);
        ImGui::BeginTooltip();
        char line[96];
        std::snprintf(line, sizeof(line), "%.0f m  \xC2\xB7  %d %s", i * a.telemetry_spacing_m, hovered_section + 1,
                      track.segments[hovered_section].name.c_str());
        ui::Text(Face::PlexSemiBold, 12, app::kText, line);
        ui::Space(4);
        std::snprintf(line, sizeof(line), "Run #%d  %s", a.run_id, FormatSpeed(a.speed_kmh[i]).c_str());
        ui::Text(Face::PlexSemiBold, 12, kRed, line);
        std::snprintf(line, sizeof(line), "Run #%d  %s", b.run_id, FormatSpeed(b.speed_kmh[i]).c_str());
        ui::Text(Face::PlexSemiBold, 12, app::kMuted, line);
        ui::Space(4);
        std::string g = "Gap " + FormatDelta(gap[i]);
        ui::Text(Face::PlexSemiBold, 12, gap[i] > 0 ? app::kLost : app::kGained, g.c_str());
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(size);
    return hovered_section;
}

void EmptyState(App& app) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ui::TextAt(dl, ImVec2(p.x, p.y + 40), Face::PlexSemiBold, 18, app::kText, "Not enough runs to compare");
    ui::TextAt(dl, ImVec2(p.x, p.y + 70), Face::PlexRegular, 13, app::kMuted,
               "Run at least two simulations, or import runs on the Import / Export page.");
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 100));
    if (ui::Button("##goimport", "Go to Import / Export", ui::ButtonKind::Outline)) app.GoTo(Screen::ImportExport);
}

}  // namespace

void DrawCompareScreen(App& app) {
    DrawTopNav(app, "Comparison Tool");
    static bool only_diffs = true;
    static int hover_section = -1;
    const std::vector<RunSummary>& runs = app.history;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 24));
    ImGui::BeginChild("##compare", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = ImGui::GetContentRegionAvail().x;
    const float band = app.prefs.neutral_band_s;

    // Header
    ImVec2 h = ImGui::GetCursorScreenPos();
    ui::TextAt(dl, h, Face::SpaceBold, 28, app::kText, "Compare runs");
    {
        float tx = h.x + ui::TextSize(Face::SpaceBold, 28, "Compare runs").x + 12;
        ImVec2 ts = ui::TextSize(Face::PlexSemiBold, 10, "SAMPLE DATA");
        ImVec2 a(tx, h.y + 7), b(tx + ts.x + 18, h.y + 7 + ts.y + 6);
        dl->AddRect(a, b, app::kNeutral, 4);
        ui::TextAt(dl, ImVec2(a.x + 9, a.y + 3), Face::PlexSemiBold, 10, app::kNeutral, "SAMPLE DATA");
    }
    ui::TextAt(dl, ImVec2(h.x, h.y + 40), Face::PlexRegular, 13, app::kMuted,
               "Every difference is Run A minus Run B. Negative means Run A was faster.");
    if (runs.size() < 2) {
        ImGui::SetCursorScreenPos(ImVec2(h.x, h.y + 56));
        EmptyState(app);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        return;
    }

    // Run pickers
    std::vector<std::string> labels;
    std::vector<const char*> items;
    int ia = 0, ib = 1;
    for (size_t i = 0; i < runs.size(); ++i) {
        labels.push_back(RunLabel(runs[i]));
        if (runs[i].run_id == app.compare_a) ia = int(i);
        if (runs[i].run_id == app.compare_b) ib = int(i);
    }
    for (const std::string& l : labels) items.push_back(l.c_str());
    {
        const float dd_w = 330, swap_w = ui::TextSize(Face::PlexSemiBold, 14, "Swap").x + 34;
        float x = h.x + W - swap_w;
        const float y = h.y + 9;
        ImGui::SetCursorScreenPos(ImVec2(x, y - 1));
        if (ui::Button("##swap", "Swap", ui::ButtonKind::Outline)) std::swap(app.compare_a, app.compare_b);
        x -= 12 + dd_w;
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Dropdown("##runb", items.data(), int(items.size()), &ib, dd_w)) app.compare_b = runs[ib].run_id;
        ui::TextAt(dl, ImVec2(x - 26, y + 11), Face::PlexSemiBold, 13, app::kMuted, "vs");
        x -= 36 + dd_w;
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        if (ui::Dropdown("##runa", items.data(), int(items.size()), &ia, dd_w)) app.compare_a = runs[ia].run_id;
        ui::TextAt(dl, ImVec2(x + 2, y - 15), Face::PlexSemiBold, 10, kRed, "RUN A");
        ui::TextAt(dl, ImVec2(x + dd_w + 38, y - 15), Face::PlexSemiBold, 10, app::kMuted, "RUN B");
    }
    ImGui::SetCursorScreenPos(ImVec2(h.x, h.y + 56));
    ImGui::Dummy(ImVec2(W, 0));
    ui::Space(20);

    const RunSummary& a = runs[ia];
    const RunSummary& b = runs[ib];
    if (ia == ib) {
        ui::Text(Face::PlexRegular, 13, app::kNeutral, "Run A and Run B are the same run. Pick a different run for one of them.");
        ui::Space(12);
    }

    // Gap cards
    {
        const float gap = 16, kw = (W - gap * 4) / 5;
        ImVec2 p = ImGui::GetCursorScreenPos();
        float lap_gap = a.lap_time_s - b.lap_time_s;
        GapCard("##g0", "LAP TIME GAP", FormatDelta(lap_gap), GapColor(lap_gap, band),
                FormatLapTime(a.lap_time_s) + "  vs  " + FormatLapTime(b.lap_time_s), kw);
        ImGui::SetCursorScreenPos(ImVec2(p.x + kw + gap, p.y));
        float ts_gap = a.top_speed_kmh - b.top_speed_kmh;
        char tsg[32];
        std::snprintf(tsg, sizeof(tsg), "%s%.0f km/h", ts_gap > 0 ? "+" : ts_gap < 0 ? "\xE2\x88\x92" : "\xC2\xB1", std::fabs(ts_gap));
        GapCard("##g1", "TOP SPEED", tsg, ts_gap > 0 ? app::kGained : ts_gap < 0 ? app::kLost : app::kNeutral,
                FormatSpeed(a.top_speed_kmh) + "  vs  " + FormatSpeed(b.top_speed_kmh), kw);
        for (int s = 0; s < 3; ++s) {
            ImGui::SetCursorScreenPos(ImVec2(p.x + (kw + gap) * (2 + s), p.y));
            char label[24], id[8], detail[48];
            std::snprintf(label, sizeof(label), "SECTOR %d GAP", s + 1);
            std::snprintf(id, sizeof(id), "##g%d", s + 2);
            std::snprintf(detail, sizeof(detail), "%.3f  vs  %.3f", a.sector_times_s[s], b.sector_times_s[s]);
            float g = a.sector_times_s[s] - b.sector_times_s[s];
            GapCard(id, label, FormatDelta(g), GapColor(g, band), detail, kw);
        }
    }
    ui::Space(20);

    // Setup differences + section gaps
    {
        const float gap = 20, left_w = std::floor((W - gap) * 0.45f), right_w = W - gap - left_w;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ui::BeginCard("##setupdiff", app::kCard, app::kCardBorder, 12, ImVec2(20, 20), left_w);
        {
            ImVec2 c = ImGui::GetCursorScreenPos();
            const float cw = ImGui::GetContentRegionAvail().x;
            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ui::TextAt(cdl, c, Face::PlexSemiBold, 18, app::kText, "Setup differences");
            int diffs = 0;
            int settings = 0;
            for (const auto& [k, va] : a.setup) {
                if (k == "Car") continue;
                ++settings;
                for (const auto& [kb, vb] : b.setup)
                    if (k == kb && va != vb) ++diffs;
            }
            char sub[64];
            std::snprintf(sub, sizeof(sub), "%d of %d settings differ", diffs, settings);
            ui::TextAt(cdl, ImVec2(c.x, c.y + 26), Face::PlexRegular, 12, app::kMuted, sub);
            const char* chips[] = {"Differences", "All settings"};
            float w0 = ui::TextSize(Face::PlexSemiBold, 12, chips[0]).x + 24, w1 = ui::TextSize(Face::PlexSemiBold, 12, chips[1]).x + 24;
            ImGui::SetCursorScreenPos(ImVec2(c.x + cw - w0 - 6 - w1, c.y + 6));
            if (ui::Chip("##diffs", chips[0], only_diffs)) only_diffs = true;
            ImGui::SetCursorScreenPos(ImVec2(c.x + cw - w1, c.y + 6));
            if (ui::Chip("##all", chips[1], !only_diffs)) only_diffs = false;
            ImGui::SetCursorScreenPos(ImVec2(c.x, c.y + 56));
            SetupDiffTable(a, b, only_diffs);
        }
        ui::EndCard();
        const float left_h = ImGui::GetItemRectSize().y;

        ImGui::SetCursorScreenPos(ImVec2(p.x + left_w + gap, p.y));
        ui::BeginCard("##sectiongaps", app::kCard, app::kCardBorder, 12, ImVec2(20, 20), right_w);
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Gap by section");
        ui::Space(4);
        ui::Text(Face::PlexRegular, 12, app::kMuted, "Where Run A gained or lost time against Run B.");
        ui::Space(14);
        int row = SectionGaps(app, a, b, hover_section);
        ui::EndCard();
        const float right_h = ImGui::GetItemRectSize().y;
        hover_section = row;

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + std::max(left_h, right_h)));
        ImGui::Dummy(ImVec2(W, 0));
    }
    ui::Space(20);

    // Speed + gap chart
    ui::BeginCard("##speedcmp", app::kCard, app::kCardBorder, 12, ImVec2(20, 20), W);
    {
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        ImVec2 c = ImGui::GetCursorScreenPos();
        const float cw = ImGui::GetContentRegionAvail().x;
        ui::TextAt(cdl, c, Face::PlexSemiBold, 18, app::kText, "Speed and gap along the lap");
        ui::TextAt(cdl, ImVec2(c.x, c.y + 26), Face::PlexRegular, 12, app::kMuted,
                   "The gap line climbs where Run A loses time and falls where it gains.");
        // Legend
        char la[32], lb[32];
        std::snprintf(la, sizeof(la), "Run #%d", a.run_id);
        std::snprintf(lb, sizeof(lb), "Run #%d", b.run_id);
        float lbw = ui::TextSize(Face::PlexSemiBold, 12, lb).x, law = ui::TextSize(Face::PlexSemiBold, 12, la).x;
        float x = c.x + cw - lbw;
        ui::TextAt(cdl, ImVec2(x, c.y + 4), Face::PlexSemiBold, 12, app::kMuted, lb);
        cdl->AddRectFilled(ImVec2(x - 22, c.y + 11), ImVec2(x - 8, c.y + 13), Hex(0x9A9AA3));
        x -= 22 + 18 + law;
        ui::TextAt(cdl, ImVec2(x, c.y + 4), Face::PlexSemiBold, 12, app::kText, la);
        cdl->AddRectFilled(ImVec2(x - 22, c.y + 11), ImVec2(x - 8, c.y + 13), kRed);
        ImGui::SetCursorScreenPos(ImVec2(c.x, c.y + 56));
        if (a.speed_kmh.empty() || b.speed_kmh.empty()) {
            const RunSummary& missing = a.speed_kmh.empty() ? a : b;
            char msg[128];
            std::snprintf(msg, sizeof(msg), "Run #%d has no telemetry saved, so only times can be compared.", missing.run_id);
            ui::Text(Face::PlexRegular, 13, app::kMuted, msg);
        } else {
            int sec = SpeedAndGapChart(app, a, b, ImGui::GetCursorScreenPos(), ImVec2(cw, 300), hover_section);
            if (sec >= 0) hover_section = sec;
        }
    }
    ui::EndCard();

    ImGui::EndChild();
    ImGui::PopStyleVar();
}
