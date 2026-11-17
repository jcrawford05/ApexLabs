"""Monotonicity and determinism of the lap solver on the Demo Circuit."""

from dataclasses import replace

import pytest

from apexphys.params import default_solver_config, load_track
from apexphys.solver import solve_lap
from conftest import SAMPLE_TRACK


@pytest.fixture(scope="module")
def track():
    return load_track(SAMPLE_TRACK)


@pytest.fixture(scope="module")
def config():
    return replace(default_solver_config(), ds_m=4.0)


def lap_time(car, track, env, cal, config):
    return solve_lap(car, track, env, cal, config).lap_time_s


def test_more_fuel_is_a_slower_lap(car, track, env, cal, config):
    times = [
        lap_time(car, track, env, cal, replace(config, start_fuel_kg=fuel))
        for fuel in (5.0, 30.0, 70.0, 110.0)
    ]
    assert times == sorted(times)
    assert len(set(times)) == len(times)


def test_grippier_compound_is_a_faster_lap(car, data, track, env, cal, config):
    times = [
        lap_time(replace(car, tire=data.tire_compounds[name]), track, env, cal, config)
        for name in ("Soft Compound", "Medium Compound", "Hard Compound")
    ]
    assert times == sorted(times)
    assert len(set(times)) == len(times)


def test_lower_power_map_is_a_slower_lap(car, track, env, cal, config):
    times = [
        lap_time(
            replace(car, setup=replace(car.setup, power_map=power_map)),
            track, env, cal, config,
        )
        for power_map in ("Qualifying", "Race", "Save")
    ]
    assert times == sorted(times)
    assert len(set(times)) == len(times)


def test_more_downforce_raises_corner_speeds(car, track, env, cal, config):
    def corner_speeds(downforce_area):
        aero = replace(car.aero, downforce_area_m2=downforce_area)
        lap = solve_lap(replace(car, aero=aero), track, env, cal, config)
        return [
            lap.segment_min_speed_m_s[i]
            for i, seg in enumerate(track.segments)
            if seg.type == "corner"
        ]

    low, high = corner_speeds(2.0), corner_speeds(4.0)
    assert all(h > l for h, l in zip(high, low))


def test_more_drag_lowers_top_speed_on_a_drag_limited_car(car, track, env, cal, config):
    def top_speed(drag_area):
        aero = replace(car.aero, drag_area_m2=drag_area)
        return solve_lap(replace(car, aero=aero), track, env, cal, config).top_speed_m_s

    assert top_speed(1.8) > top_speed(2.2)


def test_same_inputs_give_identical_output(car, track, env, cal, config):
    assert solve_lap(car, track, env, cal, config) == solve_lap(car, track, env, cal, config)
