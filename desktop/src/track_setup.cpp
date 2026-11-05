#include "track_setup.h"

#include <algorithm>
#include <cmath>

Track MakeDemoCircuit() {
    Track track;
    track.name = "Demo Circuit";
    track.segments = {
        {1, "Main straight", SegmentType::Straight, 1, 900.0F},
        {2, "T1 braking", SegmentType::Braking, 1, 110.0F},
        {3, "T1 hairpin", SegmentType::Corner, 1, 90.0F, 25.0F},
        {4, "Short straight", SegmentType::Straight, 1, 450.0F},
        {5, "T2 sweeper", SegmentType::Corner, 2, 300.0F, 180.0F},
        {6, "Hill straight", SegmentType::Straight, 2, 600.0F},
        {7, "T3 braking", SegmentType::Braking, 2, 80.0F},
        {8, "T3 left", SegmentType::Corner, 2, 120.0F, 60.0F},
        {9, "Top straight", SegmentType::Straight, 3, 700.0F},
        {10, "T4 braking", SegmentType::Braking, 3, 100.0F},
        {11, "T4 left", SegmentType::Corner, 3, 150.0F, 40.0F},
        {12, "Final straight", SegmentType::Straight, 3, 400.0F},
    };
    return track;
}

float Track::LengthM() const {
    float total = 0.0F;
    for (const Segment& segment : segments) {
        total += segment.length_m;
    }
    return total;
}

float Track::SegmentStartM(int index) const {
    float start = 0.0F;
    const int count = static_cast<int>(segments.size());
    for (int i = 0; i < index && i < count; ++i) {
        start += segments[static_cast<size_t>(i)].length_m;
    }
    return start;
}

int Track::Corners() const {
    return static_cast<int>(std::count_if(
        segments.begin(),
        segments.end(),
        [](const Segment& segment) {
            return segment.type == SegmentType::Corner;
        }));
}

float Track::LongestStraightM() const {
    float best = 0.0F;
    for (const Segment& segment : segments) {
        if (segment.type == SegmentType::Straight) {
            best = std::max(best, segment.length_m);
        }
    }
    return best;
}

const char* Track::Character() const {
    float straight = 0.0F;
    for (const Segment& segment : segments) {
        if (segment.type != SegmentType::Corner) {
            straight += segment.length_m;
        }
    }
    const float share = LengthM() > 0.0F ? straight / LengthM() : 0.0F;
    if (share > 0.65F) {
        return "Power Heavy";
    }
    if (share < 0.45F) {
        return "Corner Heavy";
    }
    return "Balanced";
}

const char* WeatherPresetName(WeatherPreset preset) {
    switch (preset) {
        case WeatherPreset::Dry:
            return "Dry";
        case WeatherPreset::Overcast:
            return "Overcast";
        case WeatherPreset::LightRain:
            return "Light rain";
        case WeatherPreset::Wet:
            return "Wet";
    }
    return "Dry";
}

const char* SegmentTypeName(SegmentType type) {
    switch (type) {
        case SegmentType::Straight:
            return "Straight";
        case SegmentType::Braking:
            return "Braking";
        case SegmentType::Corner:
            return "Corner";
    }
    return "";
}
