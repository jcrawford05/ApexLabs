"""Display values for the Setup screen, computed from the model (SI)."""

import math
from dataclasses import dataclass

from apexphys import powertrain
from apexphys.straight_line import make_case, top_speed
from apexphys.types import Calibration, CarParams, Environment


@dataclass(frozen=True)
class Readouts:
    car_mass_kg: float  # without fuel
    total_mass_kg: float  # with fuel
    min_mass_kg: float
    mandatory_ballast_kg: float
    overweight_kg: float
    front_weight_fraction: float
    rake_rad: float
    drag_area_m2: float
    air_density_kg_m3: float
    peak_power_w: float
    top_gear_speed_m_s: float  # at the rev limit in top gear
    top_speed_m_s: float
    top_speed_limit: str  # "power" or "gearing"


def rake_rad(car: CarParams) -> float:
    setup = car.setup
    return math.atan(
        (setup.rear_ride_height_m - setup.front_ride_height_m) / car.chassis.wheelbase_m
    )


def compute_readouts(
    car: CarParams, env: Environment, cal: Calibration, fuel_kg: float
) -> Readouts:
    case = make_case(car, env, cal, fuel_kg)
    v_top, limit = top_speed(case)
    return Readouts(
        car_mass_kg=case.mass.car_mass_kg,
        total_mass_kg=case.mass.total_kg,
        min_mass_kg=case.mass.min_mass_kg,
        mandatory_ballast_kg=case.mass.mandatory_ballast_kg,
        overweight_kg=case.mass.overweight_kg,
        front_weight_fraction=case.mass.front_fraction,
        rake_rad=rake_rad(car),
        drag_area_m2=car.aero.drag_area_m2,
        air_density_kg_m3=case.rho,
        peak_power_w=powertrain.peak_power(car),
        top_gear_speed_m_s=powertrain.gear_speed_limit(car, powertrain.gear_count(car)),
        top_speed_m_s=v_top,
        top_speed_limit=limit,
    )
