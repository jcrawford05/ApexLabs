"""Monotonicity and determinism: changing one input moves the output the right way."""

from dataclasses import replace

import pytest

from apexphys import units
from apexphys.atmosphere import air_density
from apexphys.straight_line import acceleration_run, make_case, top_speed

V200 = units.kmh_to_ms(200.0)


def with_drag(car, drag_area_m2):
    return replace(
        car,
        aero=replace(car.aero, drag_area_m2=drag_area_m2),
        setup=replace(car.setup, power_map="Qualifying"),
    )


def test_more_drag_lowers_power_limited_top_speed(car, env, cal):
    speeds = []
    for drag_area in (1.6, 1.8, 2.0, 2.2):
        v_top, limit = top_speed(make_case(with_drag(car, drag_area), env, cal, 30.0))
        assert limit == "power"
        speeds.append(v_top)
    assert speeds == sorted(speeds, reverse=True)
    assert len(set(speeds)) == len(speeds)


def test_less_power_lowers_power_limited_top_speed(car, env, cal):
    speeds = []
    for power_map in ("Qualifying", "Race", "Save"):
        mapped = replace(with_drag(car, 1.9), setup=replace(car.setup, power_map=power_map))
        speeds.append(top_speed(make_case(mapped, env, cal, 30.0))[0])
    assert speeds[0] > speeds[1] > speeds[2]


def test_more_fuel_slows_acceleration(car, env, cal):
    times = [
        acceleration_run(make_case(car, env, cal, fuel), V200).time_to(V200)
        for fuel in (5.0, 30.0, 70.0, 110.0)
    ]
    assert times == sorted(times)
    assert len(set(times)) == len(times)


def test_thinner_air_raises_power_limited_top_speed(car, env, cal):
    hot = replace(env, air_temp_k=units.c_to_k(40.0))
    cold = replace(env, air_temp_k=units.c_to_k(5.0))
    v_hot = top_speed(make_case(with_drag(car, 1.9), hot, cal, 30.0))[0]
    v_cold = top_speed(make_case(with_drag(car, 1.9), cold, cal, 30.0))[0]
    assert v_hot > v_cold


@pytest.mark.parametrize("temp_c", [5.0, 24.0, 40.0])
def test_density_falls_with_humidity_and_temperature(temp_c):
    densities = [
        air_density(101325.0, units.c_to_k(temp_c), rh) for rh in (0.0, 0.3, 0.6, 1.0)
    ]
    assert densities == sorted(densities, reverse=True)
    assert air_density(101325.0, units.c_to_k(temp_c + 5.0), 0.4) < densities[1]


def test_same_inputs_give_identical_output(car, env, cal):
    first = acceleration_run(make_case(car, env, cal, 30.0), V200)
    second = acceleration_run(make_case(car, env, cal, 30.0), V200)
    assert first == second
    assert top_speed(make_case(car, env, cal, 30.0)) == top_speed(
        make_case(car, env, cal, 30.0)
    )
