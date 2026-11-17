import math
from dataclasses import replace

import pytest

from apexphys import powertrain, straight_line, units
from apexphys.params import default_solver_config, load_track
from apexphys.solver import (
    corner_speed_limit,
    lateral_use,
    max_acceleration,
    max_brake_force,
    max_deceleration,
    normal_load,
    resample,
    solve_lap,
)
from apexphys.straight_line import make_case
from apexphys.types import TrackModel, TrackSegment
from apexphys.units import G
from conftest import SAMPLE_TRACK

MU = 1.65  # Medium compound
FUEL_KG = 30.0


def segment(seq, seg_type, length_m, radius_m=0.0, sector=1):
    return TrackSegment(seq, f"{seg_type} {seq}", seg_type, sector, length_m, radius_m)


def circle(radius_m):
    return TrackModel("Circle", (segment(1, "corner", 2.0 * math.pi * radius_m, radius_m),))


def no_aero(car):
    return replace(car, aero=replace(car.aero, drag_area_m2=0.0, downforce_area_m2=0.0))


def no_inertia(car):
    return replace(
        car,
        power_unit=replace(
            car.power_unit, engine_inertia_kg_m2=0.0, wheel_inertia_total_kg_m2=0.0
        ),
    )


def no_rolling(cal):
    return replace(cal, c_rr0=0.0, c_rr2_s2_per_m2=0.0)


@pytest.fixture(scope="module")
def track():
    return load_track(SAMPLE_TRACK)


@pytest.fixture(scope="module")
def config():
    return default_solver_config()


@pytest.fixture(scope="module")
def case(car, env, cal):
    return make_case(car, env, cal, FUEL_KG)


@pytest.fixture(scope="module")
def lap(car, track, env, cal, config):
    return solve_lap(car, track, env, cal, config)


# ---------------------------------------------------------------- corner limit


@pytest.mark.parametrize("radius_m", [25.0, 60.0, 180.0])
def test_corner_limit_without_aero_is_sqrt_mu_g_r(car, env, cal, radius_m):
    case = make_case(no_aero(car), env, cal, FUEL_KG)
    assert corner_speed_limit(case, 1.0 / radius_m) == pytest.approx(
        math.sqrt(MU * G * radius_m)
    )


def test_corner_limit_with_aero_balances_grip_and_demand(case):
    kappa = 1.0 / 60.0
    v = corner_speed_limit(case, kappa)
    assert v > math.sqrt(MU * G * 60.0)
    assert case.mass.total_kg * v * v * kappa == pytest.approx(MU * normal_load(case, v))
    assert lateral_use(case, v, kappa) == pytest.approx(1.0)


def test_corner_is_not_grip_limited_when_downforce_outgrows_demand(case):
    # Unlimited when mu * 0.5 * rho * ClA / m >= curvature (Core doc sanity check).
    threshold = MU * 0.5 * case.rho * 2.9 / case.mass.total_kg
    assert math.isinf(corner_speed_limit(case, 0.99 * threshold))
    assert math.isfinite(corner_speed_limit(case, 1.01 * threshold))
    assert math.isinf(corner_speed_limit(case, 0.0))


def test_lap_of_a_circle_without_aero_runs_at_the_corner_limit(car, env, cal, config):
    radius = 50.0
    lap = solve_lap(no_aero(car), circle(radius), env, no_rolling(cal), config)
    v_limit = math.sqrt(MU * G * radius)
    assert min(lap.speed_m_s) == pytest.approx(v_limit)
    assert max(lap.speed_m_s) == pytest.approx(v_limit)
    assert lap.lap_time_s == pytest.approx(2.0 * math.pi * radius / v_limit)
    assert all(a == pytest.approx(MU * G) for a in lap.a_y_m_s2)


def test_lap_of_an_aero_unlimited_circle_is_not_held_by_grip(car, env, cal, config):
    radius = 400.0
    lap = solve_lap(car, circle(radius), env, cal, config)
    assert min(lap.speed_m_s) > math.sqrt(MU * G * radius)
    assert max(lap.speed_m_s) <= powertrain.gear_speed_limit(car, 8) + 1e-9


# --------------------------------------------------------------- force limits


def test_friction_ellipse_shares_grip_between_cornering_and_braking(case):
    kappa = 1.0 / 60.0
    v = 0.6**0.5 * corner_speed_limit(no_downforce_case(case), kappa)
    bare = no_downforce_case(case)
    assert lateral_use(bare, v, kappa) == pytest.approx(0.6)
    assert max_brake_force(bare, v, kappa) == pytest.approx(
        0.8 * MU * bare.mass.total_kg * G
    )


def no_downforce_case(case):
    return replace(case, car=no_aero(case.car))


def test_cornering_at_the_limit_leaves_no_grip_for_braking_or_traction(case):
    kappa = 1.0 / 25.0
    v = corner_speed_limit(case, kappa)
    assert max_brake_force(case, v, kappa) == pytest.approx(0.0, abs=1e-3)
    assert max_acceleration(case, v, kappa) < 0.0


def test_max_acceleration_on_a_straight_matches_the_straight_line_model(case):
    for v in (0.0, 10.0, 45.0, 80.0):
        assert max_acceleration(case, v, 0.0) == straight_line.acceleration(case, v)


def test_braking_without_aero_is_mu_g(car, env, cal):
    case = make_case(no_inertia(no_aero(car)), env, no_rolling(cal), FUEL_KG)
    for v in (10.0, 50.0, 90.0):
        assert max_deceleration(case, v, 0.0) == pytest.approx(MU * G)


def test_braking_zone_in_a_lap_without_aero_decelerates_at_mu_g(car, env, cal, config):
    track = TrackModel(
        "Stop and go",
        (
            segment(1, "straight", 800.0),
            segment(2, "corner", 100.0, radius_m=30.0),
            segment(3, "straight", 100.0),
        ),
    )
    lap = solve_lap(no_inertia(no_aero(car)), track, env, no_rolling(cal), config)
    braking = [i for i in range(len(lap.brake)) if lap.brake[i] > 0.999 and lap.segment[i] == 0]
    assert len(braking) > 20
    for i in braking[1:]:
        assert lap.a_x_m_s2[i] == pytest.approx(-MU * G)
        assert lap.throttle[i] == 0.0
    # v^2 = v_corner^2 + 2 mu g d, measured back from the corner entry.
    corner_entry = braking[-1] + 1
    v_corner = lap.speed_m_s[corner_entry]
    assert v_corner == pytest.approx(math.sqrt(MU * G * 30.0))
    i = braking[5]
    distance = (corner_entry - i) * lap.ds_m
    assert lap.speed_m_s[i] ** 2 == pytest.approx(v_corner**2 + 2.0 * MU * G * distance)


# ------------------------------------------------------------ Demo Circuit lap


def test_demo_lap_totals_are_consistent(lap, track):
    assert 55.0 < lap.lap_time_s < 80.0
    assert len(lap.sector_times_s) == 3
    assert sum(lap.sector_times_s) == pytest.approx(lap.lap_time_s)
    assert len(lap.segment_times_s) == len(track.segments)
    assert sum(lap.segment_times_s) == pytest.approx(lap.lap_time_s)
    assert lap.time_s[-1] == lap.lap_time_s
    assert lap.top_speed_m_s == max(lap.speed_m_s)
    assert lap.speed_m_s[round(lap.top_speed_at_m / lap.ds_m)] == lap.top_speed_m_s


def test_demo_lap_channel_lengths(lap):
    n = 2000
    assert len(lap.distance_m) == len(lap.speed_m_s) == len(lap.time_s) == n + 1
    for channel in (
        lap.a_x_m_s2, lap.a_y_m_s2, lap.gear, lap.engine_omega_rad_s, lap.throttle,
        lap.brake, lap.drive_force_n, lap.brake_force_n, lap.resistance_force_n,
        lap.downforce_n, lap.segment, lap.sector,
    ):
        assert len(channel) == n
    assert lap.distance_m[-1] == 4000.0


def test_flying_lap_is_periodic(lap):
    assert lap.standing_start is False
    assert lap.speed_m_s[0] == lap.speed_m_s[-1]
    assert lap.speed_m_s[0] > 50.0


def test_demo_lap_never_exceeds_the_corner_or_rev_limits(lap, case, track):
    v_rev = powertrain.gear_speed_limit(case.car, 8)
    for i, v in enumerate(lap.speed_m_s[:-1]):
        seg = track.segments[lap.segment[i]]
        assert v <= v_rev + 1e-9
        assert v <= corner_speed_limit(case, seg.curvature_per_m) + 1e-9


def test_slow_corners_are_taken_at_the_grip_limit(lap, case, track):
    for index, seg in enumerate(track.segments):
        if seg.type == "corner" and seg.radius_m <= 60.0:
            limit = corner_speed_limit(case, seg.curvature_per_m)
            assert lap.segment_min_speed_m_s[index] == pytest.approx(limit, rel=0.01)


def test_every_step_is_within_what_the_car_can_do(lap, case):
    kappa = [0.0] * len(lap.a_x_m_s2)
    track = load_track(SAMPLE_TRACK)
    for i, seg_index in enumerate(lap.segment):
        kappa[i] = track.segments[seg_index].curvature_per_m
    for i, a in enumerate(lap.a_x_m_s2):
        v0, v1 = lap.speed_m_s[i], lap.speed_m_s[i + 1]
        assert a <= max_acceleration(case, v0, kappa[i]) + 1e-9
        assert -a <= max_deceleration(case, v1, kappa[i]) + 1e-9


def test_pedal_channels(lap):
    assert all(0.0 <= x <= 1.0 for x in lap.throttle)
    assert all(0.0 <= x <= 1.0 for x in lap.brake)
    assert all(t == 0.0 or b == 0.0 for t, b in zip(lap.throttle, lap.brake))
    assert max(lap.throttle) == 1.0
    assert max(lap.brake) == pytest.approx(1.0)
    assert all(1 <= g <= 8 for g in lap.gear)
    max_omega = units.rpm_to_rad_s(15000.0)
    assert all(w <= max_omega * (1.0 + 1e-9) for w in lap.engine_omega_rad_s)


def test_segment_readouts(lap, track):
    by_name = {seg.name: i for i, seg in enumerate(track.segments)}
    hairpin = by_name["T1 hairpin"]
    assert lap.segment_min_speed_m_s[hairpin] == min(lap.segment_min_speed_m_s)
    # The car is still on the brakes when it enters each braking segment.
    for name in ("T1 braking", "T3 braking", "T4 braking"):
        assert lap.segment_peak_brake_m_s2[by_name[name]] > 2.0 * G
    assert lap.segment_peak_brake_m_s2[by_name["Final straight"]] == 0.0


def test_energy_balance_over_a_flying_lap(car, track, env, cal, config):
    # With no engine inertia the effective mass is the same in every gear, so
    # kinetic energy returns to its start value and the work done by the drive
    # force equals the work lost to drag, rolling resistance and the brakes.
    car = replace(car, power_unit=replace(car.power_unit, engine_inertia_kg_m2=0.0))
    lap = solve_lap(car, track, env, cal, config)
    assert lap.speed_m_s[0] == lap.speed_m_s[-1]
    drive = sum(lap.drive_force_n) * lap.ds_m
    brakes = sum(lap.brake_force_n) * lap.ds_m
    resistance = sum(lap.resistance_force_n) * lap.ds_m
    assert drive > 1e6
    assert drive == pytest.approx(resistance + brakes, rel=1e-6)


def test_lap_time_has_converged_in_step_size(car, track, env, cal, config):
    coarse = solve_lap(car, track, env, cal, replace(config, ds_m=2.0)).lap_time_s
    fine = solve_lap(car, track, env, cal, replace(config, ds_m=0.5)).lap_time_s
    assert coarse == pytest.approx(fine, rel=1e-3)


# -------------------------------------------------------------- standing start


def test_standing_start(car, track, env, cal, config, lap):
    standing = solve_lap(car, track, env, cal, replace(config, standing_start=True))
    assert standing.standing_start is True
    assert standing.speed_m_s[0] == 0.0
    assert all(s <= f + 1e-9 for s, f in zip(standing.speed_m_s, lap.speed_m_s))
    # Once it has caught the flying lap it follows it, so only sector 1 is slower.
    assert standing.speed_m_s[-1] == lap.speed_m_s[-1]
    assert standing.sector_times_s[0] > lap.sector_times_s[0] + 1.0
    assert standing.sector_times_s[1:] == pytest.approx(lap.sector_times_s[1:])
    assert 2.0 < standing.lap_time_s - lap.lap_time_s < 10.0


def test_standing_start_launch_matches_the_acceleration_run(car, track, env, cal, config):
    standing = solve_lap(car, track, env, cal, replace(config, standing_start=True))
    run = straight_line.acceleration_run(
        make_case(car, env, cal, FUEL_KG), units.kmh_to_ms(250.0), ds=config.ds_m
    )
    assert 100 < len(run.speed_m_s) < 400  # reaches 250 km/h on the main straight
    for i in range(1, len(run.speed_m_s) - 1):
        assert standing.speed_m_s[i] == pytest.approx(run.speed_m_s[i])
        assert standing.time_s[i] == pytest.approx(run.time_s[i])


# ------------------------------------------------------------------ telemetry


def test_resample_at_five_metres(lap):
    telemetry = resample(lap, 5.0)
    assert telemetry.spacing_m == 5.0
    assert len(telemetry.speed_m_s) == 800
    assert telemetry.distance_m[0] == 0.0
    assert telemetry.distance_m[-1] == 3995.0
    assert telemetry.speed_m_s[0] == lap.speed_m_s[0]
    # 10 m is node 5 at a 2 m step; 5 m is half way between nodes 2 and 3.
    assert telemetry.speed_m_s[2] == pytest.approx(lap.speed_m_s[5])
    assert telemetry.speed_m_s[1] == pytest.approx(0.5 * (lap.speed_m_s[2] + lap.speed_m_s[3]))
    assert telemetry.throttle[1] == lap.throttle[2]
    assert telemetry.brake[190] == lap.brake[475]
