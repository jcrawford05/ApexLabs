// Results (Figma: "Results – Overview" and "Results – Section detail pop-up").
// Adapted from ApexLabsDevSpace results_screen.cpp at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. Uses Space Grotesk / IBM Plex Sans
// (shared with Start / Setup) and the Results "app" palette.
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "app.h"
#include "imgui.h"
#include "theme.h"
#include "track_map.h"
#include "widgets.h"

using namespace theme;

namespace {

enum class DeltaClass { Gained, Neutral, Lost };

DeltaClass Classify(float delta, float band) {
    // Round to the displayed precision first so "−0.05 s" with a 0.05 s band
    // counts as gained, matching the legend wording ("0.05 s or more").
    float d = std::round(delta * 100.0f) / 100.0f;
    if (d <= -band + 1e-4f) return DeltaClass::Gained;
    if (d >= band - 1e-4f) return DeltaClass::Lost;
    return DeltaClass::Neutral;
}

ImU32 DeltaColor(float delta, float band) {
    switch (Classify(delta, band)) {
        case DeltaClass::Gained: return app::kGained;
        case DeltaClass::Neutral: return app::kNeutral;
        case DeltaClass::Lost: return app::kLost;
    }
    return app::kMuted;
}

ImU32 Lerp(ImU32 a, ImU32 b, float t) {
    ImVec4 x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t));
}

// Slow (deep red) to fast (pale yellow), for the speed view of the map.
ImU32 SpeedRamp(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) return Lerp(Hex(0x8A0400), Hex(0xFF5A36), t * 2);
    return Lerp(Hex(0xFF5A36), Hex(0xFFE9A8), (t - 0.5f) * 2);
}

std::string SegmentKind(const Segment& s) {
    if (s.type == SegmentType::Corner) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Corner R%.0f", s.radius_m);
        return buf;
    }
    return SegmentTypeName(s.type);
}

// Position along a polyline at fraction t of its length.
ImVec2 PointAlong(const std::vector<ImVec2>& pts, float t) {
    float total = 0;
    for (size_t i = 1; i < pts.size(); ++i) total += std::hypot(pts[i].x - pts[i - 1].x, pts[i].y - pts[i - 1].y);
    float target = std::clamp(t, 0.0f, 1.0f) * total;
    for (size_t i = 1; i < pts.size(); ++i) {
        float len = std::hypot(pts[i].x - pts[i - 1].x, pts[i].y - pts[i - 1].y);
        if (target <= len || i + 1 == pts.size()) {
            float k = len > 0 ? std::clamp(target / len, 0.0f, 1.0f) : 0;
            return ImVec2(pts[i - 1].x + (pts[i].x - pts[i - 1].x) * k, pts[i - 1].y + (pts[i].y - pts[i - 1].y) * k);
        }
        target -= len;
    }
    return pts.back();
}

// Value of the chosen channel at telemetry sample i.
float ChannelValue(const Telemetry& t, TraceChannel c, size_t i) {
    switch (c) {
        case TraceChannel::Speed: return i < t.speed_kmh.size() ? t.speed_kmh[i] : 0;
        case TraceChannel::Throttle: return i < t.throttle_pct.size() ? t.throttle_pct[i] : 0;
        case TraceChannel::Brake: return i < t.brake_pct.size() ? t.brake_pct[i] : 0;
        case TraceChannel::TireTemp: return i < t.tire_temp_c.size() ? t.tire_temp_c[i] : 0;
    }
    return 0;
}

void BeginContentCard(const char* id, float width, ImVec2 pad = ImVec2(20, 20)) {
    ui::BeginCard(id, app::kCard, app::kCardBorder, 12, pad, width);
}

// ---- KPI row -----------------------------------------------------------------

void Kpi(const char* id, const char* label, const char* value, const char* delta, ImU32 delta_color, const char* note,
         float width) {
    BeginContentCard(id, width, ImVec2(18, 16));
    ui::Text(Face::PlexSemiBold, 11, app::kMuted, label);
    ui::Space(6);
    ui::Text(Face::SpaceBold, 28, app::kText, value);
    ui::Space(6);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (delta) {
        ui::StatusDot(dl, ImVec2(p.x + 4, p.y + 7.5f), 4, delta_color);
        ui::TextAt(dl, ImVec2(p.x + 14, p.y), Face::PlexSemiBold, 12, delta_color, delta);
        float dw = ui::TextSize(Face::PlexSemiBold, 12, delta).x;
        ui::TextAt(dl, ImVec2(p.x + 14 + dw + 6, p.y), Face::PlexRegular, 12, app::kMuted, "vs reference");
    } else if (note) {
        // One line; cut with an ellipsis when it doesn't fit the card.
        const float avail = ImGui::GetContentRegionAvail().x;
        std::string text = note;
        if (ui::TextSize(Face::PlexRegular, 12, note).x > avail) {
            while (!text.empty() && ui::TextSize(Face::PlexRegular, 12, (text + "\xE2\x80\xA6").c_str()).x > avail)
                text.pop_back();
            while (!text.empty() && text.back() == ' ') text.pop_back();
            text += "\xE2\x80\xA6";
        }
        ui::TextAt(dl, p, Face::PlexRegular, 12, app::kMuted, text.c_str());
        if (text != note && ImGui::IsMouseHoveringRect(p, ImVec2(p.x + avail, p.y + 16))) ImGui::SetTooltip("%s", note);
    }
    ImGui::Dummy(ImVec2(1, LineHeight(Face::PlexRegular, 12)));
    ui::EndCard();
}

// ---- Track map ---------------------------------------------------------------

struct MapState {
    int hovered = -1;
    int clicked = -1;
};

MapState DrawTrackMap(App& app, ImVec2 origin, float scale) {
    const TrackGeometry& g = DemoCircuitGeometry();
    const RunResult& run = app.run;
    const Telemetry& tel = run.telemetry;
    const Track& track = app.track;
    const float band = app.prefs.neutral_band_s;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    MapTransform t;
    t.scale = scale;
    t.offset = origin;
    const int n = int(std::min(g.sections.size(), run.sections.size()));

    // Hit testing over the whole canvas
    ImGui::SetCursorScreenPos(origin);
    bool canvas_hovered = false;
    ui::HitArea("##map", ImVec2(840 * scale, 504 * scale), &canvas_hovered);
    MapState st;
    if (canvas_hovered) st.hovered = HitTestSection(g, t, ImGui::GetIO().MousePos, 16 * scale);
    if (st.hovered >= 0) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemClicked()) st.clicked = st.hovered;
    }
    const int highlight = st.hovered >= 0 ? st.hovered : app.hover_section;

    const float line_w = 10 * scale;
    // Grey casing around the coloured line, as in Figma.
    for (int i = 0; i < n; ++i) {
        std::vector<ImVec2> pts;
        for (ImVec2 p : g.sections[i]) pts.push_back(t(p));
        dl->AddPolyline(pts.data(), int(pts.size()), Hex(0x2E2E34), 0, line_w + 9 * scale);
    }
    for (int i = 0; i < n; ++i) {
        const auto& sec = g.sections[i];
        if (app.map_coloring == MapColoring::DeltaVsReference) {
            std::vector<ImVec2> pts;
            for (ImVec2 p : sec) pts.push_back(t(p));
            dl->AddPolyline(pts.data(), int(pts.size()), DeltaColor(run.sections[i].delta_s, band), 0, line_w);
        } else {
            // Colour sample by sample along the section.
            const float start = track.SegmentStartM(i), len = track.segments[i].length_m;
            const int steps = std::max(2, int(len / tel.spacing_m));
            ImVec2 prev = t(PointAlong(sec, 0));
            for (int k = 1; k <= steps; ++k) {
                float f = float(k) / steps;
                ImVec2 cur = t(PointAlong(sec, f));
                size_t idx = size_t((start + f * len) / tel.spacing_m);
                ImU32 col;
                if (app.map_coloring == MapColoring::Speed) {
                    float v = idx < tel.speed_kmh.size() ? tel.speed_kmh[idx] : 0;
                    col = SpeedRamp((v - 60.0f) / (320.0f - 60.0f));
                } else {
                    float br = idx < tel.brake_pct.size() ? tel.brake_pct[idx] : 0;
                    float th = idx < tel.throttle_pct.size() ? tel.throttle_pct[idx] : 0;
                    col = br > 1 ? app::kLost : (th >= 99 ? app::kGained : app::kNeutral);
                }
                dl->AddLine(prev, cur, col, line_w);
                dl->AddCircleFilled(cur, line_w * 0.5f, col, 12);  // round joints
                prev = cur;
            }
        }
    }
    if (highlight >= 0 && highlight < n) {
        std::vector<ImVec2> pts;
        for (ImVec2 p : g.sections[highlight]) pts.push_back(t(p));
        dl->AddPolyline(pts.data(), int(pts.size()), Hex(0xFFFFFF, 90), 0, line_w + 8 * scale);
        ImU32 c = app.map_coloring == MapColoring::DeltaVsReference ? DeltaColor(run.sections[highlight].delta_s, band) : kWhite;
        dl->AddPolyline(pts.data(), int(pts.size()), c, 0, line_w);
    }

    // Start/finish and sector ticks
    dl->AddRectFilled(t(ImVec2(212.4f, 436.8f)), t(ImVec2(219.6f, 475.2f)), kWhite, 1);
    dl->AddRectFilled(t(ImVec2(549.6f, 340.8f)), t(ImVec2(554.4f, 379.2f)), Hex(0x9A9AA3, 200), 2);
    dl->AddRectFilled(t(ImVec2(357.6f, 40.8f)), t(ImVec2(362.4f, 79.2f)), Hex(0x9A9AA3, 200), 2);
    for (const auto& l : g.labels) ui::TextAt(dl, t(l.pos), Face::PlexSemiBold, 13, app::kMuted, l.text);

    if (app.prefs.show_section_numbers) {
        for (int i = 0; i < n && i < int(g.markers.size()); ++i) {
            ImVec2 c = t(g.markers[i]);
            float r = 14.4f * scale;
            ImU32 ring = DeltaColor(run.sections[i].delta_s, band);
            dl->AddCircleFilled(c, r + (i == highlight ? 2 : 0), app::kBg, 32);
            dl->AddCircle(c, r - 1.2f * scale, ring, 32, 2.4f * scale);
            char num[16];
            std::snprintf(num, sizeof(num), "%d", i + 1);
            ImVec2 ns = ui::TextSize(Face::SpaceBold, 13, num);
            ui::TextAt(dl, ImVec2(c.x - ns.x * 0.5f, c.y - ns.y * 0.5f), Face::SpaceBold, 13, app::kText, num);
        }
    }

    // Hover tooltip
    if (st.hovered >= 0) {
        const SectionResult& s = run.sections[st.hovered];
        const Segment& seg = track.segments[st.hovered];
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kRaised);
        ImGui::BeginTooltip();
        char line[96];
        std::snprintf(line, sizeof(line), "%d  %s", st.hovered + 1, seg.name.c_str());
        ui::Text(Face::PlexSemiBold, 13, app::kText, line);
        ui::Space(4);
        std::snprintf(line, sizeof(line), "%.3f s   ", s.time_s);
        std::string d = std::string(line) + FormatDelta(s.delta_s);
        ui::Text(Face::PlexSemiBold, 12, DeltaColor(s.delta_s, band), d.c_str());
        ui::Space(4);
        ui::Text(Face::PlexRegular, 11, app::kMuted, "Click for setup effects");
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }
    return st;
}

void MapLegend(App& app, float width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    auto item = [&](float& x, ImU32 col, const char* label) {
        ui::StatusDot(dl, ImVec2(x + 5, p.y + 7.5f), 5, col);
        ui::TextAt(dl, ImVec2(x + 16, p.y), Face::PlexSemiBold, 12, app::kMuted, label);
        x += 16 + ui::TextSize(Face::PlexSemiBold, 12, label).x + 18;
    };
    float x = p.x;
    char a[48], b[48], c[48];
    switch (app.map_coloring) {
        case MapColoring::DeltaVsReference:
            std::snprintf(a, sizeof(a), "Gained %.2f s or more", app.prefs.neutral_band_s);
            std::snprintf(b, sizeof(b), "Within \xC2\xB1%.2f s", app.prefs.neutral_band_s);
            std::snprintf(c, sizeof(c), "Lost %.2f s or more", app.prefs.neutral_band_s);
            item(x, app::kGained, a);
            item(x, app::kNeutral, b);
            item(x, app::kLost, c);
            break;
        case MapColoring::Speed: {
            std::string lo = "Slow (" + FormatSpeed(60) + ")";
            std::string hi = "Fast (" + FormatSpeed(320) + ")";
            ui::TextAt(dl, ImVec2(x, p.y), Face::PlexSemiBold, 12, app::kMuted, lo.c_str());
            x += ui::TextSize(Face::PlexSemiBold, 12, lo.c_str()).x + 10;
            for (int i = 0; i < 120; ++i)
                dl->AddRectFilled(ImVec2(x + i, p.y + 4), ImVec2(x + i + 1, p.y + 11), SpeedRamp(i / 119.0f));
            x += 130;
            ui::TextAt(dl, ImVec2(x, p.y), Face::PlexSemiBold, 12, app::kMuted, hi.c_str());
            break;
        }
        case MapColoring::ThrottleBrake:
            item(x, app::kGained, "Full throttle");
            item(x, app::kNeutral, "Partial throttle");
            item(x, app::kLost, "Braking");
            break;
    }
    const char* hint = "Click any section to see how your setup affected it";
    float hw = ui::TextSize(Face::PlexSemiBold, 12, hint).x;
    ui::TextAt(dl, ImVec2(p.x + width - hw, p.y), Face::PlexSemiBold, 12, app::kText, hint);
    ImGui::Dummy(ImVec2(width, LineHeight(Face::PlexSemiBold, 12)));
}

// ---- Section table -----------------------------------------------------------

int SectionTable(App& app) {
    const RunResult& run = app.run;
    const Track& track = app.track;
    const float band = app.prefs.neutral_band_s;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = ImGui::GetContentRegionAvail().x;
    int clicked = -1;
    int hovered_row = -1;
    int sector = 0;
    for (int i = 0; i < int(run.sections.size()); ++i) {
        const Segment& seg = track.segments[i];
        const SectionResult& s = run.sections[i];
        if (seg.sector != sector) {
            sector = seg.sector;
            ui::Space(4);
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 29), app::kRaised, 6);
            char label[16], time[24];
            std::snprintf(label, sizeof(label), "SECTOR %d", sector);
            std::snprintf(time, sizeof(time), "%.3f s", run.sector_times_s[sector - 1]);
            ui::TextAt(dl, ImVec2(p.x + 10, p.y + 8), Face::SpaceBold, 11, app::kMuted, label);
            float tw = ui::TextSize(Face::PlexSemiBold, 12, time).x;
            ui::TextAt(dl, ImVec2(p.x + w - 76 - tw, p.y + 7), Face::PlexSemiBold, 12, app::kText, time);
            std::string d = FormatDelta(run.sector_deltas_s[sector - 1]);
            float dw = ui::TextSize(Face::PlexSemiBold, 12, d.c_str()).x;
            ui::TextAt(dl, ImVec2(p.x + w - 10 - dw, p.y + 7), Face::PlexSemiBold, 12,
                       DeltaColor(run.sector_deltas_s[sector - 1], band), d.c_str());
            ImGui::Dummy(ImVec2(w, 29));
        }
        ui::Space(4);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID(i);
        bool hovered = false;
        if (ui::HitArea("##row", ImVec2(w, 40), &hovered)) clicked = i;
        ui::SetHandCursorIfHovered();
        ImGui::PopID();
        if (hovered) hovered_row = i;
        bool lit = hovered || app.hover_section == i || app.open_section == i;
        if (app.open_section == i) {
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 40), Hex(0x2A1512), 6);
            dl->AddRect(p, ImVec2(p.x + w, p.y + 40), Hex(0xE10600, 160), 6);
        } else if (lit) {
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 40), app::kRaised, 6);
        }
        ImU32 dc = DeltaColor(s.delta_s, band);
        ui::StatusDot(dl, ImVec2(p.x + 15, p.y + 20), 5, dc);
        char num[16];
        std::snprintf(num, sizeof(num), "%d", i + 1);
        ui::TextAt(dl, ImVec2(p.x + 30, p.y + 12.5f), Face::PlexSemiBold, 12, app::kMuted, num);
        ui::TextAt(dl, ImVec2(p.x + 58, p.y + 5), Face::PlexSemiBold, 13, app::kText, seg.name.c_str());
        ui::TextAt(dl, ImVec2(p.x + 58, p.y + 22), Face::PlexRegular, 11, app::kMuted, SegmentKind(seg).c_str());
        char time[24];
        std::snprintf(time, sizeof(time), "%.3f s", s.time_s);
        float tw = ui::TextSize(Face::PlexSemiBold, 13, time).x;
        ui::TextAt(dl, ImVec2(p.x + w - 76 - tw, p.y + 12), Face::PlexSemiBold, 13, app::kText, time);
        std::string d = FormatDelta(s.delta_s);
        float dw = ui::TextSize(Face::PlexSemiBold, 12, d.c_str()).x;
        ui::TextAt(dl, ImVec2(p.x + w - 10 - dw, p.y + 12.5f), Face::PlexSemiBold, 12, dc, d.c_str());
    }
    if (hovered_row >= 0) app.hover_section = hovered_row;
    return clicked;
}

// ---- Telemetry chart ---------------------------------------------------------

struct ChartRange {
    float lo, hi;
    std::vector<float> ticks;
    const char* unit;
};

ChartRange RangeFor(TraceChannel c) {
    switch (c) {
        case TraceChannel::Speed:
            return {0, 350, {100, 200, 300}, "km/h"};
        case TraceChannel::Throttle:
            return {0, 110, {25, 50, 75, 100}, "%"};
        case TraceChannel::Brake:
            return {0, 110, {25, 50, 75, 100}, "%"};
        case TraceChannel::TireTemp:
            return {92, 108, {95, 100, 105}, "\xC2\xB0" "C"};
    }
    return {0, 1, {}, ""};
}

// Draws a telemetry line chart for [d0, d1] metres of the lap.
void TelemetryChart(App& app, TraceChannel ch, ImVec2 p, ImVec2 size, float d0, float d1, bool axis, bool reference,
                    int shade_section, bool interactive) {
    const Telemetry& tel = app.run.telemetry;
    const Track& track = app.track;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const std::vector<float>& channel = ch == TraceChannel::Throttle ? tel.throttle_pct
                                        : ch == TraceChannel::Brake  ? tel.brake_pct
                                        : ch == TraceChannel::TireTemp ? tel.tire_temp_c
                                                                       : tel.speed_kmh;
    if (tel.speed_kmh.empty() || channel.empty()) {
        // Runs exported without telemetry only have times.
        dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), Hex(0xFFFFFF, 6), 8);
        const char* msg = "No telemetry in this run file, so only times are shown.";
        ImVec2 ms = ui::TextSize(Face::PlexRegular, 12, msg);
        ui::TextAt(dl, ImVec2(p.x + (size.x - ms.x) * 0.5f, p.y + (size.y - ms.y) * 0.5f), Face::PlexRegular, 12,
                   app::kMuted, msg);
        ImGui::SetCursorScreenPos(p);
        ImGui::Dummy(size);
        return;
    }
    const ChartRange r = RangeFor(ch);
    const float left = axis ? 40.0f : 0.0f;
    const float plot_h = axis ? size.y - 28 : size.y;
    const ImVec2 a(p.x + left, p.y), b(p.x + size.x, p.y + plot_h);
    auto X = [&](float d) { return a.x + (d - d0) / (d1 - d0) * (b.x - a.x); };
    auto Y = [&](float v) { return b.y - (v - r.lo) / (r.hi - r.lo) * (b.y - a.y); };

    // Sector bands (axis chart) or the selected section band (mini chart)
    if (axis) {
        float pos = 0;
        int sector = 1;
        float start = 0;
        for (size_t i = 0; i <= track.segments.size(); ++i) {
            bool end = i == track.segments.size() || track.segments[i].sector != sector;
            if (end) {
                if (sector == 2) dl->AddRectFilled(ImVec2(X(start), a.y), ImVec2(X(pos), b.y), Hex(0xFFFFFF, 8));
                char label[16];
                std::snprintf(label, sizeof(label), "Sector %d", sector);
                ImVec2 ls = ui::TextSize(Face::PlexSemiBold, 11, label);
                ui::TextAt(dl, ImVec2((X(start) + X(pos) - ls.x) * 0.5f, b.y + 8), Face::PlexSemiBold, 11, app::kMuted, label);
                if (i < track.segments.size()) {
                    sector = track.segments[i].sector;
                    start = pos;
                }
            }
            if (i < track.segments.size()) pos += track.segments[i].length_m;
        }
    }
    if (shade_section >= 0) {
        float s0 = track.SegmentStartM(shade_section), s1 = s0 + track.segments[shade_section].length_m;
        dl->AddRectFilled(ImVec2(X(std::max(s0, d0)), a.y), ImVec2(X(std::min(s1, d1)), b.y), Hex(0xE10600, 46));
    }

    // Grid + labels
    for (float tv : r.ticks) {
        float y = Y(tv);
        dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), Hex(0x2A2A30), 1.0f);
        if (axis) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.0f", tv);
            ui::TextAt(dl, ImVec2(p.x, y - 7), Face::PlexRegular, 10, app::kMuted, buf);
        }
    }
    if (axis) {
        ui::TextAt(dl, ImVec2(p.x, b.y - 8), Face::PlexRegular, 10, app::kMuted, r.unit);
        for (float d = 0; d <= d1 + 1; d += 1000) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.0f m", d);
            float tw = ui::TextSize(Face::PlexRegular, 10, buf).x;
            float x = X(d);
            if (d > 0) x -= tw * (d >= d1 - 1 ? 1.0f : 0.5f);
            ui::TextAt(dl, ImVec2(x, b.y + 26), Face::PlexRegular, 10, app::kMuted, buf);
        }
    }

    // Lines
    dl->PushClipRect(ImVec2(a.x, a.y - 2), ImVec2(b.x, b.y + 2), true);
    auto line = [&](auto value_at, ImU32 col, float thick) {
        std::vector<ImVec2> pts;
        size_t i0 = size_t(std::max(0.0f, d0 / tel.spacing_m));
        size_t i1 = std::min(tel.speed_kmh.size() - 1, size_t(d1 / tel.spacing_m) + 1);
        for (size_t i = i0; i <= i1; ++i) pts.push_back(ImVec2(X(i * tel.spacing_m), Y(value_at(i))));
        dl->AddPolyline(pts.data(), int(pts.size()), col, 0, thick);
    };
    if (reference && ch == TraceChannel::Speed && !tel.reference_speed_kmh.empty()) {
        // Dashed reference: draw short pieces.
        size_t i0 = size_t(std::max(0.0f, d0 / tel.spacing_m));
        size_t i1 = std::min(tel.reference_speed_kmh.size() - 1, size_t(d1 / tel.spacing_m) + 1);
        for (size_t i = i0; i + 1 <= i1; i += 2)
            dl->AddLine(ImVec2(X(i * tel.spacing_m), Y(tel.reference_speed_kmh[i])),
                        ImVec2(X((i + 1) * tel.spacing_m), Y(tel.reference_speed_kmh[i + 1])), Hex(0xC9C9D0), 1.5f);
    }
    line([&](size_t i) { return ChannelValue(tel, ch, i); }, kRed, 2.0f);
    dl->PopClipRect();

    // Hover read-out
    if (interactive) {
        ImGui::SetCursorScreenPos(a);
        bool hovered = false;
        ui::HitArea("##chart", ImVec2(b.x - a.x, b.y - a.y), &hovered);
        if (hovered) {
            float mx = std::clamp(ImGui::GetIO().MousePos.x, a.x, b.x);
            float d = d0 + (mx - a.x) / (b.x - a.x) * (d1 - d0);
            size_t i = std::min(tel.speed_kmh.size() - 1, size_t(d / tel.spacing_m + 0.5f));
            float v = ChannelValue(tel, ch, i);
            dl->AddLine(ImVec2(mx, a.y), ImVec2(mx, b.y), Hex(0xF4F4F5, 120), 1.0f);
            dl->AddCircleFilled(ImVec2(mx, Y(v)), 4, kWhite, 16);
            int sec = 0;
            for (int k = 0; k < int(track.segments.size()); ++k)
                if (d >= track.SegmentStartM(k)) sec = k;
            app.hover_section = sec;
            char val[48];
            switch (ch) {
                case TraceChannel::Speed: std::snprintf(val, sizeof(val), "%s", FormatSpeed(v).c_str()); break;
                case TraceChannel::Throttle: std::snprintf(val, sizeof(val), "Throttle %.0f%%", v); break;
                case TraceChannel::Brake: std::snprintf(val, sizeof(val), "Brake %.0f%%", v); break;
                case TraceChannel::TireTemp: std::snprintf(val, sizeof(val), "Tires %s", FormatTemp(v).c_str()); break;
            }
            char where[96];
            std::snprintf(where, sizeof(where), "%.0f m  \xC2\xB7  %d %s", i * tel.spacing_m, sec + 1,
                          track.segments[sec].name.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kRaised);
            ImGui::BeginTooltip();
            ui::Text(Face::PlexSemiBold, 13, app::kText, val);
            ui::Space(3);
            ui::Text(Face::PlexRegular, 11, app::kMuted, where);
            ImGui::EndTooltip();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(size);
}

// ---- Section detail pop-up ---------------------------------------------------

CarPanel PanelForSetting(const std::string& setting) {
    if (setting.find("wing") != std::string::npos) return CarPanel::Aerodynamics;
    if (setting.find("Tire") != std::string::npos) return CarPanel::Tires;
    if (setting.find("Ballast") != std::string::npos) return CarPanel::Chassis;
    if (setting.find("Brake") != std::string::npos) return CarPanel::Brakes;
    return CarPanel::Aerodynamics;
}

void SectionPopup(App& app) {
    const RunResult& run = app.run;
    const Track& track = app.track;
    const int n = int(run.sections.size());
    if (app.open_section < 0) return;
    if (!ImGui::IsPopupOpen("##section")) ImGui::OpenPopup("##section");

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620, 0));
    ImGui::SetNextWindowSizeConstraints(ImVec2(620, 0), ImVec2(620, vp->Size.y - 40));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, app::kCard);
    ImGui::PushStyleColor(ImGuiCol_Border, app::kControlBorder);
    if (ImGui::BeginPopupModal("##section", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize)) {
        const int i = app.open_section;
        const SectionResult& s = run.sections[i];
        const Segment& seg = track.segments[i];
        const float band = app.prefs.neutral_band_s;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float w = ImGui::GetContentRegionAvail().x;
        bool close = false;

        // Header
        ImVec2 p = ImGui::GetCursorScreenPos();
        char eyebrow[128];
        std::string kind = seg.type == SegmentType::Corner ? "CORNER, RADIUS " + std::to_string(int(seg.radius_m)) + " m"
                                                           : (seg.type == SegmentType::Braking ? "BRAKING ZONE" : "STRAIGHT");
        std::snprintf(eyebrow, sizeof(eyebrow), "SECTION %d OF %d  \xC2\xB7  SECTOR %d  \xC2\xB7  %s", i + 1, n, seg.sector, kind.c_str());
        ui::TextAt(dl, p, Face::SpaceBold, 11, kRed, eyebrow);
        ui::TextAt(dl, ImVec2(p.x, p.y + 17), Face::SpaceBold, 24, app::kText, seg.name.c_str());
        float cw = ui::TextSize(Face::PlexSemiBold, 12, "Close").x + 20 + 14;
        ImGui::SetCursorScreenPos(ImVec2(p.x + w - cw, p.y));
        ImVec2 cp = ImGui::GetCursorScreenPos();
        if (ui::Button("##close", "    Close", ui::ButtonKind::Subtle, 12, 10, 6)) close = true;
        ui::IconClose(dl, ImVec2(cp.x + 15, cp.y + 13.5f), 7, app::kMuted);
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 46));
        ui::Space(18);

        // Stats
        {
            ImVec2 sp = ImGui::GetCursorScreenPos();
            const float gap = 10, bw = (w - gap * 3) / 4;
            char v[4][32];
            std::snprintf(v[0], 32, "%.3f s", s.time_s);
            std::snprintf(v[1], 32, "%s", FormatDelta(s.delta_s).c_str());
            std::snprintf(v[2], 32, "%s", FormatSpeed(s.min_speed_kmh).c_str());
            if (s.peak_brake_g > 0) std::snprintf(v[3], 32, "%.1f g", s.peak_brake_g);
            else std::snprintf(v[3], 32, "\xE2\x80\x94");
            const char* labels[4] = {"Section time", "vs reference", "Minimum speed", "Peak braking"};
            for (int k = 0; k < 4; ++k) {
                ImVec2 bp(sp.x + k * (bw + gap), sp.y);
                dl->AddRectFilled(bp, ImVec2(bp.x + bw, bp.y + 59), app::kRaised, 8);
                ui::TextAt(dl, ImVec2(bp.x + 12, bp.y + 10), Face::PlexSemiBold, 11, app::kMuted, labels[k]);
                ui::TextAt(dl, ImVec2(bp.x + 12, bp.y + 27), Face::SpaceBold, 18, k == 1 ? DeltaColor(s.delta_s, band) : app::kText, v[k]);
            }
            ImGui::Dummy(ImVec2(w, 59));
        }
        ui::Space(18);

        // Mini speed trace across the neighbouring sections
        {
            int a = std::max(0, i - 1), b = std::min(n - 1, i + 1);
            float d0 = track.SegmentStartM(a), d1 = track.SegmentStartM(b) + track.segments[b].length_m;
            ImVec2 mp = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(mp, ImVec2(mp.x + w, mp.y + 143), app::kBg, 8);
            char title[64];
            std::snprintf(title, sizeof(title), "Speed through sections %d to %d", a + 1, b + 1);
            ui::TextAt(dl, ImVec2(mp.x + 12, mp.y + 12), Face::PlexSemiBold, 12, app::kText, title);
            float x = mp.x + 12 + ui::TextSize(Face::PlexSemiBold, 12, title).x + 16;
            dl->AddRectFilled(ImVec2(x, mp.y + 19), ImVec2(x + 14, mp.y + 21), kRed);
            ui::TextAt(dl, ImVec2(x + 20, mp.y + 13), Face::PlexRegular, 11, app::kMuted, "This run");
            x += 20 + ui::TextSize(Face::PlexRegular, 11, "This run").x + 16;
            dl->AddRectFilled(ImVec2(x, mp.y + 19), ImVec2(x + 14, mp.y + 21), Hex(0x9A9AA3));
            char ref[48];
            std::snprintf(ref, sizeof(ref), "Reference Run #%d", run.reference_run_id);
            ui::TextAt(dl, ImVec2(x + 20, mp.y + 13), Face::PlexRegular, 11, app::kMuted, ref);
            TelemetryChart(app, TraceChannel::Speed, ImVec2(mp.x + 12, mp.y + 35), ImVec2(w - 24, 96), d0, d1, false, true,
                           i, false);
            ImGui::SetCursorScreenPos(ImVec2(mp.x, mp.y + 143));
            ImGui::Dummy(ImVec2(w, 0));
        }
        ui::Space(18);

        // Effects
        ui::Text(Face::PlexSemiBold, 16, app::kText, "How your setup affected this section");
        ui::Space(4);
        ui::TextWrapped(Face::PlexRegular, 12, app::kMuted,
                        "Estimated time each setting added (+) or saved (\xE2\x88\x92) here compared with the reference car.");
        ui::Space(18);
        float max_abs = 0.01f;
        for (const Effect& e : s.effects) max_abs = std::max(max_abs, std::fabs(e.seconds));
        int strongest = -1;
        for (int k = 0; k < int(s.effects.size()); ++k) {
            const Effect& e = s.effects[k];
            if (strongest < 0 || std::fabs(e.seconds) > std::fabs(s.effects[strongest].seconds)) strongest = k;
            ImVec2 ep = ImGui::GetCursorScreenPos();
            const float text_w = w - 28 - 14 - 72;
            float desc_h = Font(Face::PlexRegular, 12)
                               ->CalcTextSizeA(LineHeight(Face::PlexRegular, 12), FLT_MAX, text_w, e.description.c_str())
                               .y;
            float h = 12 + 16 + 3 + desc_h + 12;
            dl->AddRectFilled(ep, ImVec2(ep.x + w, ep.y + h), app::kRaised, 8);
            ui::TextAt(dl, ImVec2(ep.x + 14, ep.y + 12), Face::PlexSemiBold, 13, app::kText, e.setting.c_str());
            float sw = ui::TextSize(Face::PlexSemiBold, 13, e.setting.c_str()).x;
            ui::TextAt(dl, ImVec2(ep.x + 14 + sw + 8, ep.y + 12.5f), Face::PlexRegular, 12, app::kMuted, e.value.c_str());
            dl->AddText(Font(Face::PlexRegular, 12), LineHeight(Face::PlexRegular, 12), ImVec2(ep.x + 14, ep.y + 31),
                        app::kTextSoft, e.description.c_str(), nullptr, text_w);
            ImU32 c = e.seconds > 0 ? app::kLost : (e.seconds < 0 ? app::kGained : app::kMuted);
            std::string d = FormatDelta(e.seconds);
            float dw = ui::TextSize(Face::SpaceBold, 15, d.c_str()).x;
            float cy = ep.y + h * 0.5f;
            ui::TextAt(dl, ImVec2(ep.x + w - 14 - dw, cy - 14), Face::SpaceBold, 15, c, d.c_str());
            float bar = std::max(4.0f, 72.0f * std::fabs(e.seconds) / max_abs);
            dl->AddRectFilled(ImVec2(ep.x + w - 14 - bar, cy + 10), ImVec2(ep.x + w - 14, cy + 14), c, 2);
            ImGui::Dummy(ImVec2(w, h));
            ui::Space(8);
        }
        {
            ImVec2 op = ImGui::GetCursorScreenPos();
            ui::TextAt(dl, ImVec2(op.x + 14, op.y), Face::PlexRegular, 12, app::kMuted, "Driver line and other effects");
            std::string d = FormatDelta(s.other_effects_s);
            float dw = ui::TextSize(Face::PlexSemiBold, 12, d.c_str()).x;
            ui::TextAt(dl, ImVec2(op.x + w - 14 - dw, op.y), Face::PlexSemiBold, 12, app::kMuted, d.c_str());
            ImGui::Dummy(ImVec2(w, LineHeight(Face::PlexRegular, 12)));
        }

        if (!s.tradeoff.empty()) {
            ui::Space(18);
            ImVec2 tp = ImGui::GetCursorScreenPos();
            float lw = ui::TextSize(Face::SpaceBold, 12, "Trade-off").x;
            float text_w = w - 28 - lw - 10;
            float th = Font(Face::PlexRegular, 12)
                           ->CalcTextSizeA(LineHeight(Face::PlexRegular, 12), FLT_MAX, text_w, s.tradeoff.c_str())
                           .y;
            float h = th + 24;
            dl->AddRectFilled(tp, ImVec2(tp.x + w, tp.y + h), Hex(0xF5B700, 20), 8);
            dl->AddRect(tp, ImVec2(tp.x + w, tp.y + h), app::kNeutral, 8);
            ui::TextAt(dl, ImVec2(tp.x + 15, tp.y + 12), Face::SpaceBold, 12, app::kNeutral, "Trade-off");
            dl->AddText(Font(Face::PlexRegular, 12), LineHeight(Face::PlexRegular, 12), ImVec2(tp.x + 15 + lw + 10, tp.y + 12),
                        app::kText, s.tradeoff.c_str(), nullptr, text_w);
            ImGui::Dummy(ImVec2(w, h));
        }
        ui::Space(18);

        // Footer
        ImVec2 fp = ImGui::GetCursorScreenPos();
        if (i > 0) {
            std::string prev = "\xE2\x86\x90  Section " + std::to_string(i);
            if (ui::Button("##prev", prev.c_str(), ui::ButtonKind::Outline)) app.open_section = i - 1;
            ImGui::SameLine(0, 8);
        }
        if (i + 1 < n) {
            std::string next = "Section " + std::to_string(i + 2) + "  \xE2\x86\x92";
            if (ui::Button("##next", next.c_str(), ui::ButtonKind::Outline)) app.open_section = i + 1;
        }
        float ow = ui::TextSize(Face::PlexSemiBold, 14, "Open in Car Setup").x + 32;
        ImGui::SetCursorScreenPos(ImVec2(fp.x + w - ow, fp.y + 1));
        if (ui::Button("##open", "Open in Car Setup", ui::ButtonKind::Primary)) {
            CarPanel target = strongest >= 0 ? PanelForSetting(s.effects[strongest].setting) : CarPanel::Aerodynamics;
            ImGui::CloseCurrentPopup();
            app.GoTo(Screen::CarSetup);
            app.car_panel = target;
            app.track_focus = false;
        }
        ImGui::SetCursorScreenPos(ImVec2(fp.x, fp.y + 39));
        ImGui::Dummy(ImVec2(w, 0));

        // Keyboard: Esc closes, arrows step through sections.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) close = true;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && app.open_section > 0) app.open_section--;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && app.open_section + 1 < n) app.open_section++;
        // Click outside closes.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
            close = true;
        if (close) {
            app.open_section = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

}  // namespace

void DrawResultsScreen(App& app) {
    const RunResult& run = app.run;
    char nav_label[96];
    std::snprintf(nav_label, sizeof(nav_label), "%s  \xC2\xB7  Run #%d", run.track_name.c_str(), run.run_id);
    DrawTopNav(app, nav_label);
    app.hover_section = -1;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 24));
    ImGui::BeginChild("##results", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = ImGui::GetContentRegionAvail().x;
    const float band = app.prefs.neutral_band_s;

    // Page header
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ui::TextAt(dl, p, Face::SpaceBold, 28, app::kText, "Results");
        if (run.sample_data) {
            float tx = p.x + ui::TextSize(Face::SpaceBold, 28, "Results").x + 12;
            ImVec2 ts = ui::TextSize(Face::PlexSemiBold, 10, "SAMPLE DATA");
            ImVec2 a(tx, p.y + 7), b(tx + ts.x + 18, p.y + 7 + ts.y + 6);
            dl->AddRect(a, b, app::kNeutral, 4);
            ui::TextAt(dl, ImVec2(a.x + 9, a.y + 3), Face::PlexSemiBold, 10, app::kNeutral, "SAMPLE DATA");
        }
        char sub[256];
        std::snprintf(sub, sizeof(sub),
                      "%s  \xC2\xB7  %s  \xC2\xB7  %s, air %s / track %s  \xC2\xB7  Engine v%s  \xC2\xB7  Compared with reference Run #%d",
                      run.vehicle_name.c_str(), run.track_name.c_str(), run.weather_preset.c_str(),
                      FormatTemp(run.air_c).c_str(), FormatTemp(run.track_c).c_str(),
                      run.engine_version.c_str(), run.reference_run_id);
        ui::TextAt(dl, ImVec2(p.x, p.y + 40), Face::PlexRegular, 13, app::kMuted, sub);

        float w_rerun = ui::TextSize(Face::PlexSemiBold, 14, "Re-run simulation").x + 32;
        float w_export = ui::TextSize(Face::PlexSemiBold, 14, "Export JSON").x + 34;
        float w_compare = ui::TextSize(Face::PlexSemiBold, 14, "Compare runs").x + 34;
        float x = p.x + W - w_rerun - 10 - w_export - 10 - w_compare;
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + 8.5f));
        if (ui::Button("##compare", "Compare runs", ui::ButtonKind::Outline))
        {
            app.compare_a = run.run_id;
            app.compare_b = run.reference_run_id;
            app.GoTo(Screen::Compare);
        }
        ImGui::SetCursorScreenPos(ImVec2(x + w_compare + 10, p.y + 8.5f));
        if (ui::Button("##export", "Export JSON", ui::ButtonKind::Outline))
            app.ShowToast("JSON export lands with Import / Export.");
        ImGui::SetCursorScreenPos(ImVec2(x + w_compare + 10 + w_export + 10, p.y + 9.5f));
        if (ui::Button("##rerun", "Re-run simulation", ui::ButtonKind::Primary))
            app.ShowToast("The physics engine isn't connected yet.");
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 56));
        ImGui::Dummy(ImVec2(W, 0));
    }
    ui::Space(20);

    // KPI row
    {
        const float gap = 16, kw = (W - gap * 4) / 5;
        ImVec2 p = ImGui::GetCursorScreenPos();
        std::string lap = FormatLapTime(run.lap_time_s);
        std::string lap_d = FormatDelta(run.lap_delta_s);
        Kpi("##k0", "LAP TIME", lap.c_str(), lap_d.c_str(), DeltaColor(run.lap_delta_s, band), nullptr, kw);
        ImGui::SetCursorScreenPos(ImVec2(p.x + (kw + gap), p.y));
        std::string top = FormatSpeed(run.top_speed_kmh);
        Kpi("##k1", "TOP SPEED", top.c_str(), nullptr, 0, run.top_speed_note.c_str(), kw);
        for (int s = 0; s < 3; ++s) {
            ImGui::SetCursorScreenPos(ImVec2(p.x + (kw + gap) * (2 + s), p.y));
            char label[16], value[16], id[8];
            std::snprintf(label, sizeof(label), "SECTOR %d", s + 1);
            std::snprintf(value, sizeof(value), "%.3f", run.sector_times_s[s]);
            std::snprintf(id, sizeof(id), "##k%d", s + 2);
            std::string d = FormatDelta(run.sector_deltas_s[s]);
            Kpi(id, label, value, d.c_str(), DeltaColor(run.sector_deltas_s[s], band), nullptr, kw);
        }
    }
    ui::Space(20);

    // Main row: track map + section times
    int clicked = -1;
    {
        const float table_w = 420, gap = 20;
        const float map_w = W - table_w - gap;
        const float scale = std::max(0.5f, (map_w - 96) / 840.0f);
        const float card_h = 121 + 504 * scale + 95;
        ImVec2 p = ImGui::GetCursorScreenPos();

        ui::BeginCard("##mapcard", app::kCard, app::kCardBorder, 12, ImVec2(0, 0), map_w);
        {
            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ImVec2 c = ImGui::GetCursorScreenPos();
            ui::TextAt(cdl, ImVec2(c.x + 21, c.y + 21), Face::PlexSemiBold, 18, app::kText, "Track map");
            ui::TextAt(cdl, ImVec2(c.x + 21, c.y + 47), Face::PlexRegular, 12, app::kMuted,
                       app.map_coloring == MapColoring::DeltaVsReference ? "Colored by time gained or lost against the reference lap"
                       : app.map_coloring == MapColoring::Speed           ? "Colored by speed along this lap"
                                                                          : "Colored by throttle and brake along this lap");
            const char* chips[] = {"\xCE\x94 vs reference", "Speed", "Throttle / brake"};
            float cw[3], total = 0;
            for (int k = 0; k < 3; ++k) {
                cw[k] = ui::TextSize(Face::PlexSemiBold, 12, chips[k]).x + 24;
                total += cw[k] + (k ? 6 : 0);
            }
            float x = c.x + map_w - 21 - total;
            for (int k = 0; k < 3; ++k) {
                ImGui::SetCursorScreenPos(ImVec2(x, c.y + 28));
                ImGui::PushID(k);
                if (ui::Chip("##chip", chips[k], int(app.map_coloring) == k)) app.map_coloring = MapColoring(k);
                ImGui::PopID();
                x += cw[k] + 6;
            }
            MapState ms = DrawTrackMap(app, ImVec2(c.x + (map_w - 840 * scale) * 0.5f, c.y + 121), scale);
            if (ms.hovered >= 0) app.hover_section = ms.hovered;
            if (ms.clicked >= 0) clicked = ms.clicked;
            ImGui::SetCursorScreenPos(ImVec2(c.x + 21, c.y + card_h - 36));
            MapLegend(app, map_w - 42);
            ImGui::SetCursorScreenPos(ImVec2(c.x, c.y + card_h - 1));
            ImGui::Dummy(ImVec2(map_w, 0));
        }
        ui::EndCard();

        ImGui::SetCursorScreenPos(ImVec2(p.x + map_w + gap, p.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, app::kCard);
        ImGui::PushStyleColor(ImGuiCol_Border, app::kCardBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12);
        ImGui::BeginChild("##table", ImVec2(table_w, card_h), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ui::Text(Face::PlexSemiBold, 18, app::kText, "Section times");
        ui::Space(4);
        char sub[96];
        std::snprintf(sub, sizeof(sub), "%zu sections in 3 sectors. Select a row to open details.", run.sections.size());
        ui::Text(Face::PlexRegular, 12, app::kMuted, sub);
        ui::Space(6);
        int row = SectionTable(app);
        if (row >= 0) clicked = row;
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + card_h));
        ImGui::Dummy(ImVec2(W, 0));
    }
    ui::Space(20);

    // Speed trace
    {
        ui::BeginCard("##trace", app::kCard, app::kCardBorder, 12, ImVec2(20, 20), W);
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        ImVec2 c = ImGui::GetCursorScreenPos();
        const float cw = ImGui::GetContentRegionAvail().x;
        static const char* const titles[] = {"Speed along the lap", "Throttle along the lap", "Brake along the lap",
                                             "Tire temperature along the lap"};
        ui::TextAt(cdl, ImVec2(c.x + 1, c.y + 1), Face::PlexSemiBold, 18, app::kText, titles[int(app.trace_channel)]);
        char sub[96];
        std::snprintf(sub, sizeof(sub), "From telemetry samples, every %.0f m. Shaded bands are sectors.",
                      run.telemetry.spacing_m);
        ui::TextAt(cdl, ImVec2(c.x + 1, c.y + 27), Face::PlexRegular, 12, app::kMuted, sub);
        const char* chips[] = {"Speed", "Throttle", "Brake", "Tire temp"};
        float total = 0, chw[4];
        for (int k = 0; k < 4; ++k) {
            chw[k] = ui::TextSize(Face::PlexSemiBold, 12, chips[k]).x + 24;
            total += chw[k] + (k ? 6 : 0);
        }
        float x = c.x + cw - total;
        for (int k = 0; k < 4; ++k) {
            ImGui::SetCursorScreenPos(ImVec2(x, c.y + 7));
            ImGui::PushID(100 + k);
            if (ui::Chip("##ch", chips[k], int(app.trace_channel) == k)) app.trace_channel = TraceChannel(k);
            ImGui::PopID();
            x += chw[k] + 6;
        }
        ImGui::SetCursorScreenPos(ImVec2(c.x, c.y + 53));
        float lap_m = app.track.LengthM();
        TelemetryChart(app, app.trace_channel, ImVec2(c.x, c.y + 53), ImVec2(cw, 178), 0, lap_m, true, false, -1, true);
        ui::EndCard();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    if (clicked >= 0) app.open_section = clicked;
    SectionPopup(app);
}
