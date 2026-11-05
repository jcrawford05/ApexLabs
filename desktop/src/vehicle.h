// Lean vehicle setup state for Car & Track Setup panels.
// Field defaults match ApexLabsDevSpace sample_vehicle.json at commit
// c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. JSON load/save comes later.
#pragma once

#include <string>

struct Vehicle {
    std::string name = "New Configuration";

    float front_wing_deg = 8.0F;
    float rear_wing_deg = 12.0F;
    float front_ride_height_mm = 31.0F;
    float floor_load_pct = 62.0F;
    float ballast_kg = 5.0F;
    float drag_area_m2 = 1.10F;

    float aero_mass_kg = 18.0F;
    float engine_mass_kg = 150.0F;
    float chassis_mass_kg = 540.0F;
    float brakes_mass_kg = 40.0F;
    float tire_mass_kg = 46.0F;

    float TotalMassKg() const;
    const char* DragTendency() const;
    const char* AeroBias() const;
};
