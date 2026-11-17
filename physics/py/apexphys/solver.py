"""C8. Cornering limit and the three-pass quasi-steady-state lap solver.

The car is a point mass on the friction ellipse (C4) with aero (C2), the
powertrain (C5) and rolling resistance (C6). Axle load transfer and brake bias
(C3, C7) arrive in phase P3, so braking uses the grip of all four tires and
traction uses the static rear axle load plus rear downforce.

One call solves one lap at a fixed fuel load. Multi-lap runs with tire, fuel
and brake state carried between laps arrive in phase P4.
"""

import math
from dataclasses import dataclass

from apexphys import aero, powertrain, tire
from apexphys.straight_line import (
    StraightLineCase,
    make_case,
    resistance_force,
    traction_limit,
)
from apexphys.track import TrackGrid, discretise
from apexphys.types import (
    Calibration,
    CarParams,
    Environment,
    LapSolution,
    SolverConfig,
    TrackModel,
)
from apexphys.units import G

MAX_PASSES = 50
CONVERGED_M_S = 1e-9


# ------------------------------------------------------------- grip limits


def normal_load(case: StraightLineCase, v: float) -> float:
    """Total tire load: weight plus downforce."""
    return case.mass.total_kg * G + aero.downforce(case.car.aero, case.rho, v)


def corner_speed_limit(case: StraightLineCase, curvature: float) -> float:
    """Highest speed the grip allows at ``curvature`` (1/m).

    Returns infinity on a straight, and in a corner where downforce grows
    faster than the grip needed, so the corner is not grip limited.
    """
    if curvature <= 0.0:
        return math.inf
    mu = tire.grip_coefficient(case.car)
    m = case.mass.total_kg
    denominator = m * curvature - 0.5 * case.rho * case.car.aero.downforce_area_m2 * mu
    if denominator <= 0.0:
        return math.inf
    return math.sqrt(mu * m * G / denominator)


def lateral_use(case: StraightLineCase, v: float, curvature: float) -> float:
    """Cornering force as a fraction of the lateral grip limit, 0..1."""
    if curvature <= 0.0:
        return 0.0
    demand = case.mass.total_kg * v * v * curvature
    return min(1.0, demand / (tire.grip_coefficient(case.car) * normal_load(case, v)))


def max_acceleration(case: StraightLineCase, v: float, curvature: float) -> float:
    """Largest forward acceleration at ``v``. Negative when resistance exceeds the drive."""
    grip_left = tire.longitudinal_fraction(lateral_use(case, v, curvature))
    drive = min(powertrain.drive_force(case.car, v), traction_limit(case, v) * grip_left)
    gear = powertrain.select_gear(case.car, v)
    return (drive - resistance_force(case, v)) / powertrain.effective_mass(
        case.car, case.mass.total_kg, gear
    )


def max_brake_force(case: StraightLineCase, v: float, curvature: float) -> float:
    grip_left = tire.longitudinal_fraction(lateral_use(case, v, curvature))
    return tire.grip_coefficient(case.car) * normal_load(case, v) * grip_left


def max_deceleration(case: StraightLineCase, v: float, curvature: float) -> float:
    """Largest deceleration at ``v``, as a positive number."""
    gear = powertrain.select_gear(case.car, v)
    return (
        max_brake_force(case, v, curvature) + resistance_force(case, v)
    ) / powertrain.effective_mass(case.car, case.mass.total_kg, gear)


# ------------------------------------------------------------ speed profile


def _flying_profile(case: StraightLineCase, grid: TrackGrid) -> list[float]:
    """Node speeds for a flying lap: corner pass, forward pass, backward pass."""
    n, ds = grid.n_steps, grid.ds_m
    kappa = grid.step_curvature
    v_rev_limit = powertrain.gear_speed_limit(case.car, powertrain.gear_count(case.car))

    # 1. Corner pass.
    limit = [
        min(v_rev_limit, corner_speed_limit(case, grid.node_curvature(i)))
        for i in range(n)
    ]
    # The lap is periodic, so start where the car is slowest and go round
    # until the speed back at that node stops changing.
    start = min(range(n), key=limit.__getitem__)
    v = list(limit)

    # 2. Forward pass: accelerate as hard as possible, capped by the corner pass.
    for _ in range(MAX_PASSES):
        before = v[start]
        for k in range(n):
            i = (start + k) % n
            j = (i + 1) % n
            v2 = v[i] * v[i] + 2.0 * max_acceleration(case, v[i], kappa[i]) * ds
            v[j] = min(limit[j], math.sqrt(max(0.0, v2)))
        if abs(v[start] - before) <= CONVERGED_M_S:
            break

    # 3. Backward pass: brake as late as possible, capped by the forward pass.
    forward = list(v)
    for _ in range(MAX_PASSES):
        before = v[start]
        for k in range(n):
            j = (start - k) % n
            i = (j - 1) % n
            v2 = v[j] * v[j] + 2.0 * max_deceleration(case, v[j], kappa[i]) * ds
            v[i] = min(forward[i], math.sqrt(v2))
        if abs(v[start] - before) <= CONVERGED_M_S:
            break
    return v


def _standing_profile(
    case: StraightLineCase, grid: TrackGrid, flying: list[float]
) -> list[float]:
    """Node speeds from rest at the start line, never above the flying lap."""
    n, ds = grid.n_steps, grid.ds_m
    v = [0.0]
    for i in range(n):
        v2 = v[i] * v[i] + 2.0 * max_acceleration(case, v[i], grid.step_curvature[i]) * ds
        v.append(min(flying[(i + 1) % n], math.sqrt(max(0.0, v2))))
    return v


# ----------------------------------------------------------------- solution


def _build_solution(
    case: StraightLineCase,
    track: TrackModel,
    grid: TrackGrid,
    v: list[float],
    standing_start: bool,
) -> LapSolution:
    car, m = case.car, case.mass.total_kg
    n, ds = grid.n_steps, grid.ds_m
    idle_omega = car.power_unit.torque_curve[0][0]

    time = [0.0]
    a_x, a_y, gears, omega, throttle, brake = [], [], [], [], [], []
    drive_n, brake_n, resist_n, downforce_n = [], [], [], []
    for i in range(n):
        v0, v1, kappa = v[i], v[i + 1], grid.step_curvature[i]
        a = (v1 * v1 - v0 * v0) / (2.0 * ds)
        time.append(time[-1] + 2.0 * ds / (v0 + v1))
        gear = powertrain.select_gear(car, v0)

        # Tire force needed for this step. The forward pass evaluates forces
        # at the step's first node and the backward pass at its second, so
        # the channels use the same points.
        resist = resistance_force(case, v0)
        force = powertrain.effective_mass(car, m, gear) * a + resist
        if force >= 0.0:
            available = powertrain.drive_force(car, v0)
            drive_n.append(force)
            brake_n.append(0.0)
            throttle.append(min(1.0, force / available) if available > 0.0 else 0.0)
            brake.append(0.0)
        else:
            resist = resistance_force(case, v1)
            gear_1 = powertrain.select_gear(car, v1)
            force = max(0.0, -(powertrain.effective_mass(car, m, gear_1) * a + resist))
            available = max_brake_force(case, v1, kappa)
            drive_n.append(0.0)
            brake_n.append(force)
            throttle.append(0.0)
            brake.append(min(1.0, force / available) if available > 0.0 else 1.0)

        a_x.append(a)
        a_y.append(v0 * v0 * kappa)
        gears.append(gear)
        omega.append(max(idle_omega, powertrain.engine_speed(car, v0, gear)))
        resist_n.append(resist)
        downforce_n.append(aero.downforce(car.aero, case.rho, v0))

    segment_count = len(track.segments)
    segment_times = [0.0] * segment_count
    segment_min_speed = [math.inf] * segment_count
    segment_peak_brake = [0.0] * segment_count
    sector_times: dict[int, float] = {}
    for i in range(n):
        seg, dt = grid.step_segment[i], time[i + 1] - time[i]
        segment_times[seg] += dt
        segment_min_speed[seg] = min(segment_min_speed[seg], v[i], v[i + 1])
        if brake_n[i] > 0.0:
            segment_peak_brake[seg] = max(segment_peak_brake[seg], -a_x[i])
        sector_times[grid.step_sector[i]] = sector_times.get(grid.step_sector[i], 0.0) + dt

    top_node = max(range(n + 1), key=v.__getitem__)
    return LapSolution(
        ds_m=ds,
        standing_start=standing_start,
        distance_m=tuple(i * ds for i in range(n + 1)),
        speed_m_s=tuple(v),
        time_s=tuple(time),
        a_x_m_s2=tuple(a_x),
        a_y_m_s2=tuple(a_y),
        gear=tuple(gears),
        engine_omega_rad_s=tuple(omega),
        throttle=tuple(throttle),
        brake=tuple(brake),
        drive_force_n=tuple(drive_n),
        brake_force_n=tuple(brake_n),
        resistance_force_n=tuple(resist_n),
        downforce_n=tuple(downforce_n),
        segment=grid.step_segment,
        sector=grid.step_sector,
        lap_time_s=time[-1],
        sector_times_s=tuple(sector_times[s] for s in sorted(sector_times)),
        segment_times_s=tuple(segment_times),
        segment_min_speed_m_s=tuple(segment_min_speed),
        segment_peak_brake_m_s2=tuple(segment_peak_brake),
        top_speed_m_s=v[top_node],
        top_speed_at_m=top_node * ds,
    )


def solve_lap(
    car: CarParams,
    track: TrackModel,
    env: Environment,
    cal: Calibration,
    config: SolverConfig,
) -> LapSolution:
    """Solve one lap. Deterministic: the same inputs always give the same output."""
    case = make_case(car, env, cal, config.start_fuel_kg)
    grid = discretise(track, config.ds_m)
    flying = _flying_profile(case, grid)
    if config.standing_start:
        nodes = _standing_profile(case, grid, flying)
    else:
        nodes = flying + [flying[0]]
    return _build_solution(case, track, grid, nodes, config.standing_start)


# ---------------------------------------------------------------- telemetry


@dataclass(frozen=True)
class Telemetry:
    """A lap resampled at a fixed spacing, for the Results screen and exports."""

    spacing_m: float
    distance_m: tuple[float, ...]
    speed_m_s: tuple[float, ...]
    throttle: tuple[float, ...]
    brake: tuple[float, ...]


def resample(solution: LapSolution, spacing_m: float) -> Telemetry:
    """Sample every ``spacing_m`` from the start line (4000 m at 5 m gives 800 points).

    Speed is interpolated between nodes; throttle and brake take the value of
    the step the sample lies in.
    """
    n = len(solution.a_x_m_s2)
    length = solution.ds_m * n
    count = max(1, round(length / spacing_m))
    distance, speed, throttle, brake = [], [], [], []
    for k in range(count):
        s = k * spacing_m
        step = min(n - 1, int(s / solution.ds_m))
        frac = s / solution.ds_m - step
        v0, v1 = solution.speed_m_s[step], solution.speed_m_s[step + 1]
        distance.append(s)
        speed.append(v0 + frac * (v1 - v0))
        throttle.append(solution.throttle[step])
        brake.append(solution.brake[step])
    return Telemetry(
        spacing_m, tuple(distance), tuple(speed), tuple(throttle), tuple(brake)
    )
