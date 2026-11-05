// Lean vehicle setup state for Car & Track Setup panels.
// Field defaults match ApexLabsDevSpace sample_vehicle.json / tire_catalog.json
// at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0. JSON load/save comes later.
#pragma once

#include <string>

struct TireCompound {
    const char* name;
    const char* short_name;
    float mass_kg;
    const char* grip;
    const char* durability;
    const char* warm_up;
    const char* degradation;
    const char* peak_window;
    int life_laps;
};

// Soft / Medium / Hard sample catalog from DevSpace tire_catalog.json.
const TireCompound* TireCatalog();
int TireCatalogCount();
const TireCompound& TireAt(int index);

struct Vehicle {
    std::string name = "New Configuration";

    // Aerodynamics
    float front_wing_deg = 8.0F;
    float rear_wing_deg = 12.0F;
    float front_ride_height_mm = 31.0F;
    float floor_load_pct = 62.0F;
    float drag_area_m2 = 1.10F;

    // Tires
    float pressure_front_psi = 22.5F;
    float pressure_rear_psi = 21.8F;
    int compound = 1;  // Medium

    // Powertrain
    std::string power_map = "Race";  // Qualifying, Race, Save
    float final_drive = 3.2F;
    float shift_rpm = 14500.0F;
    float engine_peak_kw = 745.0F;
    float engine_max_rpm = 15000.0F;
    std::string engine_name = "V6 Hybrid Spec B";

    // Chassis
    float ballast_kg = 5.0F;
    float ballast_front_pct = 50.0F;
    float rear_ride_height_mm = 78.0F;
    float wheelbase_m = 3.6F;
    float cg_height_m = 0.30F;

    // Brakes
    float brake_bias_front_pct = 57.0F;
    std::string brake_cooling = "Standard";  // Closed, Standard, Open
    float brakes_max_decel_g = 5.0F;
    std::string brakes_name = "Carbon Disc Set";

    float aero_mass_kg = 18.0F;
    float engine_mass_kg = 150.0F;
    float chassis_mass_kg = 540.0F;
    float brakes_mass_kg = 40.0F;

    float TotalMassKg() const;
    const TireCompound& SelectedTire() const;
    const char* DragTendency() const;
    const char* AeroBias() const;
};
