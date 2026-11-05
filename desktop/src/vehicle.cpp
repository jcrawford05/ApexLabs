#include "vehicle.h"

#include <algorithm>

namespace {

constexpr TireCompound kTires[] = {
    {"Soft Compound",
     "Soft",
     44.0F,
     "High",
     "Low",
     "Fast",
     "High",
     "Laps 2-7",
     18},
    {"Medium Compound",
     "Medium",
     46.0F,
     "Medium",
     "Medium",
     "Medium",
     "Medium",
     "Laps 3-14",
     30},
    {"Hard Compound",
     "Hard",
     48.0F,
     "Low",
     "High",
     "Slow",
     "Low",
     "Laps 5-25",
     45},
};

}  // namespace

const TireCompound* TireCatalog() {
    return kTires;
}

int TireCatalogCount() {
    return static_cast<int>(sizeof(kTires) / sizeof(kTires[0]));
}

const TireCompound& TireAt(int index) {
    const int clamped = std::clamp(index, 0, TireCatalogCount() - 1);
    return kTires[clamped];
}

const TireCompound& Vehicle::SelectedTire() const {
    return TireAt(compound);
}

float Vehicle::TotalMassKg() const {
    return chassis_mass_kg + engine_mass_kg + aero_mass_kg + brakes_mass_kg +
           SelectedTire().mass_kg + ballast_kg;
}

const char* Vehicle::DragTendency() const {
    const float score =
        rear_wing_deg + front_wing_deg * 0.5F + (drag_area_m2 - 1.1F) * 10.0F;
    if (score < 17.0F) {
        return "Low";
    }
    if (score < 22.0F) {
        return "Medium";
    }
    return "High";
}

const char* Vehicle::AeroBias() const {
    if (rear_wing_deg <= 13.0F) {
        return "HIGH-SPEED BIAS";
    }
    if (rear_wing_deg >= 16.0F) {
        return "DOWNFORCE BIAS";
    }
    return "BALANCED";
}
