"""Straight-line performance: forces, top speed and the acceleration run.

This is the phase P1 end-to-end check of C1, C2, C5 and C6. The forces here
are the ones the C8 forward pass uses on a straight. Until load transfer (C3,
phase P3) the traction limit uses the static rear axle load plus rear
downforce, which understates grip at launch.
"""

from dataclasses import dataclass

from apexphys import aero, powertrain, resistance
from apexphys.atmosphere import environment_air_density
from apexphys.mass import MassProperties, mass_properties
from apexphys.types import Calibration, CarParams, Environment
from apexphys.units import G


@dataclass(frozen=True)
class StraightLineCase:
    car: CarParams
    mass: MassProperties
    rho: float
    cal: Calibration


def make_case(
    car: CarParams, env: Environment, cal: Calibration, fuel_kg: float
) -> StraightLineCase:
    return StraightLineCase(
        car=car,
        mass=mass_properties(car, fuel_kg),
        rho=environment_air_density(env),
        cal=cal,
    )


def traction_limit(case: StraightLineCase, v: float) -> float:
    """Largest drive force the rear tires can transmit (rear-wheel drive)."""
    _, rear_downforce = aero.axle_downforce(case.car.aero, case.rho, v)
    return case.car.tire.mu_ref * (case.mass.static_rear_load_n + rear_downforce)


def resistance_force(case: StraightLineCase, v: float) -> float:
    """Aerodynamic drag plus rolling resistance."""
    normal_load = case.mass.total_kg * G + aero.downforce(case.car.aero, case.rho, v)
    return aero.drag_force(
        case.car.aero, case.rho, v
    ) + resistance.rolling_resistance_force(case.cal, v, normal_load)


def net_force(case: StraightLineCase, v: float) -> float:
    drive = min(powertrain.drive_force(case.car, v), traction_limit(case, v))
    return drive - resistance_force(case, v)


def acceleration(case: StraightLineCase, v: float) -> float:
    gear = powertrain.select_gear(case.car, v)
    return net_force(case, v) / powertrain.effective_mass(
        case.car, case.mass.total_kg, gear
    )


def top_speed(case: StraightLineCase) -> tuple[float, str]:
    """Top speed in m/s and what limits it: ``"power"`` or ``"gearing"``.

    Power limited means drive force equals drag plus rolling resistance.
    Gearing limited means the engine reaches the rev limit in top gear first.
    """
    v_gearing = powertrain.gear_speed_limit(case.car, powertrain.gear_count(case.car))
    step = 0.05
    v = step
    while v < v_gearing:
        if net_force(case, v) <= 0.0:
            lo, hi = v - step, v
            for _ in range(60):
                mid = 0.5 * (lo + hi)
                if net_force(case, mid) > 0.0:
                    lo = mid
                else:
                    hi = mid
            return 0.5 * (lo + hi), "power"
        v += step
    return v_gearing, "gearing"


@dataclass(frozen=True)
class AccelerationRun:
    distance_m: tuple[float, ...]
    speed_m_s: tuple[float, ...]
    time_s: tuple[float, ...]
    gear: tuple[int, ...]

    def time_to(self, v_target: float) -> float | None:
        """Elapsed time on reaching ``v_target``, or None if the run never gets there."""
        for i in range(1, len(self.speed_m_s)):
            v0, v1 = self.speed_m_s[i - 1], self.speed_m_s[i]
            if v1 >= v_target:
                frac = (v_target - v0) / (v1 - v0)
                return self.time_s[i - 1] + frac * (self.time_s[i] - self.time_s[i - 1])
        return None


def acceleration_run(
    case: StraightLineCase,
    v_target: float,
    ds: float = 0.25,
    max_distance_m: float = 5000.0,
) -> AccelerationRun:
    """Standing-start run in distance steps (the C8 forward-pass update).

    Stops on reaching ``v_target``, when the car stops accelerating, or at
    ``max_distance_m``.
    """
    s, v, t = 0.0, 0.0, 0.0
    distance, speed, time = [s], [v], [t]
    gear = [powertrain.select_gear(case.car, v)]
    while v < v_target and s < max_distance_m:
        a = acceleration(case, v)
        if a <= 0.0:
            break
        v_next = (v * v + 2.0 * a * ds) ** 0.5
        t += 2.0 * ds / (v + v_next)
        s += ds
        v = v_next
        distance.append(s)
        speed.append(v)
        time.append(t)
        gear.append(powertrain.select_gear(case.car, v))
    return AccelerationRun(tuple(distance), tuple(speed), tuple(time), tuple(gear))
