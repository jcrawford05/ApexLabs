"""Air properties (C2 air density; the rest of T7 arrives in phase P4)."""

import math

from apexphys.types import Environment
from apexphys.units import k_to_c

R_DRY_AIR = 287.05  # J/(kg K)
R_WATER_VAPOR = 461.495  # J/(kg K)


def saturation_vapor_pressure_pa(temp_k: float) -> float:
    """Magnus form."""
    t_c = k_to_c(temp_k)
    return 100.0 * 6.1094 * math.exp(17.625 * t_c / (t_c + 243.04))


def air_density(pressure_pa: float, temp_k: float, relative_humidity: float) -> float:
    """Humid-air density in kg/m^3. ``relative_humidity`` is 0..1."""
    e = relative_humidity * saturation_vapor_pressure_pa(temp_k)
    return (pressure_pa - e) / (R_DRY_AIR * temp_k) + e / (R_WATER_VAPOR * temp_k)


def environment_air_density(env: Environment) -> float:
    """Air density at the track.

    Tracks carry no elevation yet, so ambient pressure is the sea-level value.
    """
    return air_density(env.sea_level_pressure_pa, env.air_temp_k, env.relative_humidity)
