// One completed (or sample) simulation run shown on the Results screen.
// Adapted from ApexLabsDevSpace data.h at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. JSON load/export comes later;
// MakeSampleRun() embeds Run #42 until the physics engine writes real runs.
#pragma once

#include <string>
#include <utility>
#include <vector>

struct Effect {
    std::string setting;
    std::string value;
    std::string description;
    float seconds = 0.0F;
};

struct SectionResult {
    int seq = 0;
    float time_s = 0.0F;
    float delta_s = 0.0F;
    float min_speed_kmh = 0.0F;
    float peak_brake_g = 0.0F;
    std::vector<Effect> effects;
    float other_effects_s = 0.0F;
    std::string tradeoff;
};

struct Telemetry {
    float spacing_m = 5.0F;
    std::vector<float> speed_kmh;
    std::vector<float> throttle_pct;
    std::vector<float> brake_pct;
    std::vector<float> tire_temp_c;
    std::vector<float> reference_speed_kmh;
};

struct RunResult {
    bool sample_data = true;
    int run_id = 0;
    int reference_run_id = 0;
    std::string vehicle_name;
    std::string track_name;
    std::string engine_version;
    std::string weather_preset;
    float air_c = 0.0F;
    float track_c = 0.0F;
    float lap_time_s = 0.0F;
    float lap_delta_s = 0.0F;
    float top_speed_kmh = 0.0F;
    float top_speed_at_m = 0.0F;
    std::string top_speed_note;
    float sector_times_s[3] = {};
    float sector_deltas_s[3] = {};
    std::vector<SectionResult> sections;
    Telemetry telemetry;
};

// One row of run history for Import / Export and Comparison.
struct RunSummary {
    int run_id = 0;
    std::string date;
    std::string vehicle_name;
    std::string track_name;
    std::string weather_preset;
    float air_c = 0.0F;
    float track_c = 0.0F;
    float lap_time_s = 0.0F;
    float top_speed_kmh = 0.0F;
    float sector_times_s[3] = {};
    std::vector<float> section_times_s;
    std::vector<std::pair<std::string, std::string>> setup;
    float telemetry_spacing_m = 5.0F;
    std::vector<float> speed_kmh;
};

RunResult MakeSampleRun();
void AttachSampleTelemetry(RunResult& run);
std::vector<RunSummary> MakeSampleHistory();

std::string FormatLapTime(float seconds);  // 1:18.432
std::string FormatDelta(float seconds);    // +0.22 s, −0.08 s, ±0.00 s
std::string FormatSpeed(float kmh, bool with_unit = true);
std::string FormatTemp(float celsius);
std::string FileSlug(const std::string& name);
