"""C5. Powertrain and longitudinal drive force.

Gears are numbered from 1. Every function takes the car, so the final drive,
shift point and power map from the setup are always applied.
"""

from apexphys.types import CarParams, PowerUnit


def engine_torque(power_unit: PowerUnit, omega: float) -> float:
    """Full-map torque at engine speed ``omega`` (rad/s).

    Below the first curve point the torque is held at that point's value (the
    clutch slips at launch); above the rev limit it is zero.
    """
    curve = power_unit.torque_curve
    if omega > power_unit.max_omega_rad_s:
        return 0.0
    if omega <= curve[0][0]:
        return curve[0][1]
    for (w0, t0), (w1, t1) in zip(curve, curve[1:]):
        if omega <= w1:
            return t0 + (t1 - t0) * (omega - w0) / (w1 - w0)
    return curve[-1][1]


def map_scale(car: CarParams) -> float:
    return car.power_unit.map_scales[car.setup.power_map]


def peak_power(car: CarParams) -> float:
    """Peak engine power in watts for the selected power map."""
    return map_scale(car) * max(w * t for w, t in car.power_unit.torque_curve)


def gear_count(car: CarParams) -> int:
    return len(car.power_unit.gear_ratios)


def total_ratio(car: CarParams, gear: int) -> float:
    return car.power_unit.gear_ratios[gear - 1] * car.setup.final_drive


def engine_speed(car: CarParams, v: float, gear: int) -> float:
    return v * total_ratio(car, gear) / car.chassis.tire_radius_m


def gear_speed_limit(car: CarParams, gear: int) -> float:
    """Road speed at the rev limit in ``gear``."""
    return (
        car.power_unit.max_omega_rad_s
        * car.chassis.tire_radius_m
        / total_ratio(car, gear)
    )


def select_gear(car: CarParams, v: float) -> int:
    """The gear the car is in at speed ``v``: it upshifts on reaching the shift point."""
    shift_omega = min(car.setup.shift_omega_rad_s, car.power_unit.max_omega_rad_s)
    for gear in range(1, gear_count(car)):
        if engine_speed(car, v, gear) <= shift_omega:
            return gear
    return gear_count(car)


def gear_force(car: CarParams, v: float, gear: int) -> float:
    """Wheel force available in ``gear`` at speed ``v``, before the traction limit."""
    torque = map_scale(car) * engine_torque(car.power_unit, engine_speed(car, v, gear))
    return (
        torque
        * total_ratio(car, gear)
        * car.power_unit.driveline_efficiency
        / car.chassis.tire_radius_m
    )


def drive_force(car: CarParams, v: float) -> float:
    """Wheel force with the setup's shift point."""
    return gear_force(car, v, select_gear(car, v))


def force_envelope(car: CarParams, v: float) -> float:
    """Wheel force with ideal shifting: the best gear at every speed (3A)."""
    return max(gear_force(car, v, gear) for gear in range(1, gear_count(car) + 1))


def effective_mass(car: CarParams, mass_kg: float, gear: int) -> float:
    """Mass plus the rotating inertia of the wheels and engine, referred to the road."""
    pu = car.power_unit
    rotating = (
        pu.wheel_inertia_total_kg_m2
        + pu.engine_inertia_kg_m2 * total_ratio(car, gear) ** 2 * pu.driveline_efficiency
    )
    return mass_kg + rotating / car.chassis.tire_radius_m**2
