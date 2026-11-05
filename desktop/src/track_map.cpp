#include "track_map.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace {

void Arc(std::vector<ImVec2>& out, ImVec2 c, float r, float a0_deg, float a1_deg) {
    const int steps = std::max(8, int(std::fabs(a1_deg - a0_deg) / 6.0f));
    for (int i = 0; i <= steps; ++i) {
        float a = (a0_deg + (a1_deg - a0_deg) * i / steps) * 3.14159265f / 180.0f;
        out.push_back(ImVec2(c.x + std::cos(a) * r, c.y + std::sin(a) * r));
    }
}

TrackGeometry Build() {
    TrackGeometry g;
    g.design_size = ImVec2(840, 504);
    auto line = [](ImVec2 a, ImVec2 b) { return std::vector<ImVec2>{a, b}; };
    std::vector<ImVec2> s;
    g.sections.push_back(line({216, 456}, {636, 456}));  // 1 main straight
    g.sections.push_back(line({636, 456}, {720, 456}));  // 2 T1 braking
    s.clear(); Arc(s, {720, 408}, 48, 90, -90); g.sections.push_back(s);  // 3 T1 hairpin
    g.sections.push_back(line({720, 360}, {552, 360}));  // 4 short straight
    s.clear(); Arc(s, {552, 240}, 120, 90, 180); g.sections.push_back(s);  // 5 T2 sweeper
    g.sections.push_back(line({432, 240}, {432, 168}));  // 6 hill straight
    g.sections.push_back(line({432, 168}, {432, 132}));  // 7 T3 braking
    s.clear(); Arc(s, {360, 132}, 72, 0, -90); g.sections.push_back(s);  // 8 T3 left
    g.sections.push_back(line({360, 60}, {192, 60}));    // 9 top straight
    g.sections.push_back(line({192, 60}, {144, 60}));    // 10 T4 braking
    s.clear(); Arc(s, {144, 108}, 48, -90, -180); g.sections.push_back(s);  // 11 T4 left
    s = {{96, 108}}; Arc(s, {216, 336}, 120, 180, 90); g.sections.push_back(s);  // 12 final straight

    const float m = 14.4f;  // marker half size
    const ImVec2 corners[] = {{411.6f, 475.2f}, {663.6f, 475.2f}, {784.8f, 393.6f}, {621.6f, 316.8f},
                              {484.8f, 280.8f}, {448.8f, 192.0f},  {448.8f, 136.8f}, {427.2f, 45.6f},
                              {261.6f, 14.4f},  {153.6f, 14.4f},   {72.0f, 38.4f},   {48.0f, 249.6f}};
    for (ImVec2 c : corners) g.markers.push_back(ImVec2(c.x + m, c.y + m));
    g.start_finish[0] = ImVec2(216, 438);
    g.start_finish[1] = ImVec2(216, 474);
    g.labels = {{{120, 482.4f}, "S/F"}, {{446.4f, 396}, "S1 | S2"}, {{261.6f, 93.6f}, "S2 | S3"}};
    g.sector_ticks = {{552, 360}, {360, 60}};
    return g;
}

float DistToSegment(ImVec2 p, ImVec2 a, ImVec2 b) {
    ImVec2 ab(b.x - a.x, b.y - a.y), ap(p.x - a.x, p.y - a.y);
    float len2 = ab.x * ab.x + ab.y * ab.y;
    float t = len2 > 0 ? std::clamp((ap.x * ab.x + ap.y * ab.y) / len2, 0.0f, 1.0f) : 0.0f;
    float dx = a.x + ab.x * t - p.x, dy = a.y + ab.y * t - p.y;
    return std::sqrt(dx * dx + dy * dy);
}

}  // namespace

const TrackGeometry& DemoCircuitGeometry() {
    static const TrackGeometry g = Build();
    return g;
}

MapTransform FitTrack(const TrackGeometry& g, ImVec2 rect_min, ImVec2 rect_size, float margin) {
    // Fit the drawn track (not the whole design canvas) into the rectangle.
    ImVec2 lo(FLT_MAX, FLT_MAX), hi(-FLT_MAX, -FLT_MAX);
    for (const auto& sec : g.sections)
        for (ImVec2 p : sec) {
            lo = ImVec2(std::min(lo.x, p.x), std::min(lo.y, p.y));
            hi = ImVec2(std::max(hi.x, p.x), std::max(hi.y, p.y));
        }
    float w = hi.x - lo.x, h = hi.y - lo.y;
    float scale = std::min((rect_size.x - margin * 2) / w, (rect_size.y - margin * 2) / h);
    MapTransform t;
    t.scale = scale;
    t.offset = ImVec2(rect_min.x + (rect_size.x - w * scale) * 0.5f - lo.x * scale,
                      rect_min.y + (rect_size.y - h * scale) * 0.5f - lo.y * scale);
    return t;
}

int HitTestSection(const TrackGeometry& g, const MapTransform& t, ImVec2 mouse, float max_dist) {
    int best = -1;
    float best_d = max_dist;
    for (int i = 0; i < int(g.sections.size()); ++i) {
        const auto& sec = g.sections[i];
        for (size_t k = 1; k < sec.size(); ++k) {
            float d = DistToSegment(mouse, t(sec[k - 1]), t(sec[k]));
            if (d < best_d) {
                best_d = d;
                best = i;
            }
        }
        float dm = std::hypot(mouse.x - t(g.markers[i]).x, mouse.y - t(g.markers[i]).y);
        if (dm < 15 * t.scale && dm < best_d) {
            best_d = dm;
            best = i;
        }
    }
    return best;
}
