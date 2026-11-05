// Track-side setup state for Car & Track Setup panels.
// Defaults match ApexLabsDevSpace sample_track.json / app defaults at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. JSON load/save comes later.
#pragma once

#include <string>
#include <vector>

enum class SegmentType { Straight, Braking, Corner };

struct Segment {
    int seq = 0;
    std::string name;
    SegmentType type = SegmentType::Straight;
    int sector = 1;
    float length_m = 0.0F;
    float radius_m = 0.0F;
};

struct Track {
    std::string name = "Demo Circuit";
    std::vector<Segment> segments;

    float LengthM() const;
    int Corners() const;
    float LongestStraightM() const;
    const char* Character() const;
    float SegmentStartM(int index) const;
};

enum class WeatherPreset { Dry, Overcast, LightRain, Wet };

struct Weather {
    WeatherPreset preset = WeatherPreset::Dry;
    float air_c = 24.0F;
    float track_c = 38.0F;
    float humidity_pct = 42.0F;
    float wind_kmh = 12.0F;
    float rain_pct = 0.0F;
};

struct SurfaceSettings {
    int grip_state = 1;  // 0 = Green, 1 = Rubbered-in, 2 = Dusty
    float roughness_pct = 30.0F;
    bool track_evolution = true;
};

struct SimulationSettings {
    int laps = 1;
    int start = 0;  // 0 = Flying lap, 1 = Standing start
    float fuel_kg = 30.0F;
};

// App-wide defaults the Simulation / Results screens read until Settings is
// migrated.
struct SetupPreferences {
    int time_step = 1;  // 0 = 1 ms, 1 = 5 ms, 2 = 10 ms
    float telemetry_spacing_m = 5.0F;
    float neutral_band_s = 0.05F;
    bool show_section_numbers = true;
};

Track MakeDemoCircuit();
const char* WeatherPresetName(WeatherPreset preset);

// Sample Run #42 lap time used by Surface estimates until results data loads.
inline constexpr float kSampleLapTimeS = 78.432F;
