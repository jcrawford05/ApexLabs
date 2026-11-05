#include "vehicle.h"

float Vehicle::TotalMassKg() const {
    return chassis_mass_kg + engine_mass_kg + aero_mass_kg + brakes_mass_kg +
           tire_mass_kg + ballast_kg;
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
