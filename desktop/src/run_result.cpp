// Sample Run #42 from ApexLabsDevSpace data/sample_run.json (telemetry deferred).
#include "run_result.h"

#include <cmath>
#include <cstdio>

RunResult MakeSampleRun() {
    RunResult run;
    run.sample_data = true;
    run.run_id = 42;
    run.reference_run_id = 41;
    run.vehicle_name = "Low Drag Monza Setup";
    run.track_name = "Demo Circuit";
    run.engine_version = "0.1.0";
    run.weather_preset = "Dry";
    run.air_c = 24.0F;
    run.track_c = 38.0F;
    run.lap_time_s = 78.432F;
    run.lap_delta_s = 0.22F;
    run.top_speed_kmh = 317.0F;
    run.top_speed_at_m = 930.0F;
    run.top_speed_note = "End of section 1, main straight";
    run.sector_times_s[0] = 26.912F;
    run.sector_times_s[1] = 24.305F;
    run.sector_times_s[2] = 27.215F;
    run.sector_deltas_s[0] = 0.11F;
    run.sector_deltas_s[1] = 0.13F;
    run.sector_deltas_s[2] = -0.02F;
    run.sections = {
        {
            1,
            12.0F,
            -0.08F,
            266.0F,
            0.0F,
            {
                {"Rear wing angle", "12° (low)", "Lower drag adds about 4 km/h by the end of the straight.", -0.07F},
                {"Ballast", "5 kg", "Extra mass slows acceleration slightly.", 0.01F},
            },
            -0.02F,
            {},
        },
        {
            2,
            2.1F,
            0.03F,
            70.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce means less grip under braking, so the car brakes earlier.", 0.04F},
                {"Brakes", "Carbon disc set, 5.0 g max", "Stronger braking let the car brake about 5 m later.", -0.02F},
                {"Ballast", "5 kg", "Extra mass needs a slightly longer braking distance.", 0.01F},
            },
            0.0F,
            {},
        },
        {
            3,
            4.5F,
            0.21F,
            70.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce means the car carries about 6 km/h less through the apex.", 0.14F},
                {"Tires", "Medium compound, grip 1.65", "Tires ran at 104°C, 4°C above their best temperature, so grip dropped on exit.", 0.04F},
                {"Ballast", "5 kg", "Extra mass makes the car slower to pick up speed out of the corner.", 0.02F},
                {"Brakes", "Carbon disc set, 5.0 g max", "Stronger braking let the car brake about 6 m later.", -0.01F},
            },
            0.02F,
            "A rear wing of 16° would save about 0.12 s here but lose about 0.18 s on the straights (sections 1, 6 and 9).",
        },
        {
            4,
            8.312F,
            -0.05F,
            79.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Lower drag helps the car pull away on the short run to T2.", -0.04F},
                {"Tires", "Medium compound, grip 1.65", "Grip dropped slightly on exit while the tires cooled.", 0.01F},
            },
            -0.02F,
            {},
        },
        {
            5,
            7.1F,
            0.12F,
            223.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce lowers the speed the car can hold through the long corner by about 5 km/h.", 0.09F},
                {"Tires", "Medium compound, grip 1.65", "Tires ran 3°C over their best temperature through the corner.", 0.03F},
            },
            0.0F,
            "A rear wing of 15° would save about 0.07 s here but lose about 0.10 s on sections 6 and 9.",
        },
        {
            6,
            9.4F,
            -0.1F,
            224.0F,
            0.0F,
            {
                {"Rear wing angle", "12° (low)", "Lower drag adds about 3 km/h on the climb.", -0.08F},
                {"Ballast", "5 kg", "Extra mass costs a little on the uphill run.", 0.01F},
            },
            -0.03F,
            {},
        },
        {
            7,
            2.2F,
            0.02F,
            114.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce means the car brakes about 3 m earlier.", 0.03F},
                {"Brakes", "Carbon disc set, 5.0 g max", "Stronger braking won back about 2 m.", -0.01F},
            },
            0.0F,
            {},
        },
        {
            8,
            5.605F,
            0.09F,
            114.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce costs about 3 km/h at the apex.", 0.06F},
                {"Tires", "Medium compound, grip 1.65", "Tires were still hot from T2, so grip was slightly down.", 0.02F},
                {"Ballast", "5 kg", "Extra mass makes the car slower out of the corner.", 0.01F},
            },
            0.0F,
            {},
        },
        {
            9,
            10.1F,
            -0.11F,
            119.0F,
            0.0F,
            {
                {"Rear wing angle", "12° (low)", "Lower drag adds about 5 km/h on the longest flat-out run.", -0.09F},
            },
            -0.02F,
            {},
        },
        {
            10,
            2.15F,
            0.0F,
            90.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce means the car brakes slightly earlier.", 0.02F},
                {"Brakes", "Carbon disc set, 5.0 g max", "Stronger braking cancels out the earlier braking point.", -0.02F},
            },
            0.0F,
            {},
        },
        {
            11,
            5.8F,
            0.15F,
            90.0F,
            4.8F,
            {
                {"Rear wing angle", "12° (low)", "Less downforce costs about 4 km/h through the left-hander.", 0.1F},
                {"Tires", "Medium compound, grip 1.65", "Tires were 2°C over their best temperature.", 0.03F},
                {"Ballast", "5 kg", "Extra mass makes the car slower out of the corner.", 0.01F},
            },
            0.01F,
            "A rear wing of 14° would save about 0.05 s here but lose about 0.06 s on section 12.",
        },
        {
            12,
            9.165F,
            -0.06F,
            97.0F,
            0.0F,
            {
                {"Rear wing angle", "12° (low)", "Lower drag adds about 3 km/h before the line.", -0.05F},
            },
            -0.01F,
            {},
        },
    };
    AttachSampleTelemetry(run);
    return run;
}

std::string FormatLapTime(float seconds) {
    const int minutes = static_cast<int>(seconds / 60.0F);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d:%06.3f", minutes, seconds - minutes * 60.0F);
    return buffer;
}

std::string FormatDelta(float seconds) {
    char buffer[32];
    const float rounded = std::round(seconds * 100.0F) / 100.0F;
    if (std::fabs(rounded) < 0.005F) {
        std::snprintf(buffer, sizeof(buffer), "\xC2\xB1" "0.00 s");
    } else if (rounded > 0.0F) {
        std::snprintf(buffer, sizeof(buffer), "+%.2f s", rounded);
    } else {
        std::snprintf(buffer, sizeof(buffer), "\xE2\x88\x92%.2f s", -rounded);
    }
    return buffer;
}

std::string FormatSpeed(float kmh, bool with_unit) {
    char buffer[32];
    if (with_unit) {
        std::snprintf(buffer, sizeof(buffer), "%.0f km/h", kmh);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f", kmh);
    }
    return buffer;
}

std::string FormatTemp(float celsius) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f\xC2\xB0" "C", celsius);
    return buffer;
}
