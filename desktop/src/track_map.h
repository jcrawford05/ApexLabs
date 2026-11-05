// Drawing geometry for the Demo Circuit track map.
// Adapted from ApexLabsDevSpace at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
//
// The track schema stores segment lengths and radii but no x/y layout, so the
// map shape is hard-coded here (traced from the Figma Results map, in its
// 840 x 504 design space). Once tracks carry coordinates, build this from data.
#pragma once

#include <vector>

#include "imgui.h"

struct TrackGeometry {
    ImVec2 design_size;                       // 840 x 504
    std::vector<std::vector<ImVec2>> sections;  // one polyline per section, in lap order
    std::vector<ImVec2> markers;              // numbered marker centres
    ImVec2 start_finish[2];                   // S/F tick
    struct Label { ImVec2 pos; const char* text; };
    std::vector<Label> labels;                // "S/F", "S1 | S2", "S2 | S3"
    std::vector<ImVec2> sector_ticks;         // where sectors change
};

const TrackGeometry& DemoCircuitGeometry();

// Maps design coordinates into a screen rectangle, keeping the aspect ratio.
struct MapTransform {
    ImVec2 offset;
    float scale = 1;
    ImVec2 operator()(ImVec2 p) const { return ImVec2(offset.x + p.x * scale, offset.y + p.y * scale); }
};
MapTransform FitTrack(const TrackGeometry& g, ImVec2 rect_min, ImVec2 rect_size, float margin);

// Returns the section under the mouse, or -1.
int HitTestSection(const TrackGeometry& g, const MapTransform& t, ImVec2 mouse, float max_dist);
