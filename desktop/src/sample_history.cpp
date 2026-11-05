// Sample run history metadata from ApexLabsDevSpace sample_history.json.
// Telemetry / setup detail deferred until Compare / JSON I/O.
#include "run_result.h"

#include <cctype>

std::vector<RunSummary> MakeSampleHistory() {
    return {
        {
            42,
            "2026-10-27",
            "Low Drag Monza Setup",
            "Demo Circuit",
            "Dry",
            78.432F,
        },
        {
            41,
            "2026-10-25",
            "Baseline High Downforce",
            "Demo Circuit",
            "Dry",
            78.212F,
        },
        {
            40,
            "2026-10-24",
            "High Downforce",
            "Demo Circuit",
            "Dry",
            78.332F,
        },
        {
            39,
            "2026-10-22",
            "Soft Sprint Trial",
            "Demo Circuit",
            "Dry",
            77.859F,
        },
        {
            38,
            "2026-10-21",
            "Baseline Wet Test",
            "Demo Circuit",
            "Light rain",
            79.49F,
        },
        {
            37,
            "2026-10-20",
            "Baseline",
            "Demo Circuit",
            "Dry",
            78.714F,
        },
    };
}

std::string FileSlug(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (!out.empty() && out.back() != '_') {
            out += '_';
        }
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out.empty() ? "export" : out;
}

