from dataclasses import replace

import pytest

from apexphys import aero, powertrain, resistance, units
from apexphys.readouts import compute_readouts, rake_rad
from apexphys.straight_line import (
    acceleration_run,
    make_case,
    net_force,
    resistance_force,
    top_speed,
    traction_limit,
)
from apexphys.units import G

FUEL_KG = 30.0


def high_drag(car, drag_area_m2=1.8, power_map="Qualifying"):
    return replace(
        car,
        aero=replace(car.aero, drag_area_m2=drag_area_m2),
        setup=replace(car.setup, power_map=power_map),
    )


# ------------------------------------------------------------------ top speed


def test_stock_car_runs_out_of_gearing(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    v_top, limit = top_speed(case)
    assert limit == "gearing"
    assert v_top == pytest.approx(powertrain.gear_speed_limit(car, 8))
    assert net_force(case, 0.999 * v_top) > 0.0


def test_terminal_speed_is_where_drive_equals_drag_plus_rolling_resistance(car, env, cal):
    case = make_case(high_drag(car), env, cal, FUEL_KG)
    v_top, limit = top_speed(case)
    assert limit == "power"
    assert v_top < powertrain.gear_speed_limit(case.car, 8)

    drive = powertrain.drive_force(case.car, v_top)
    drag = aero.drag_force(case.car.aero, case.rho, v_top)
    normal_load = case.mass.total_kg * G + aero.downforce(case.car.aero, case.rho, v_top)
    rolling = resistance.rolling_resistance_force(cal, v_top, normal_load)
    assert drive < traction_limit(case, v_top)
    assert drive == pytest.approx(drag + rolling, abs=0.5)


def test_terminal_speed_closed_form_without_rolling_resistance(car, env, cal):
    # With no rolling resistance and flat power: P * eta = 0.5 * rho * CdA * v^3.
    no_rolling = replace(cal, c_rr0=0.0, c_rr2_s2_per_m2=0.0)
    case = make_case(high_drag(car), env, no_rolling, FUEL_KG)
    v_top, limit = top_speed(case)
    expected = (0.95 * 723000.0 / (0.5 * case.rho * 1.8)) ** (1.0 / 3.0)
    assert limit == "power"
    assert v_top == pytest.approx(expected, rel=2e-3)


def test_resistance_force_is_drag_plus_rolling_resistance(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    v = 70.0
    q = 0.5 * case.rho * v * v
    expected = q * 1.10 + (cal.c_rr0 + cal.c_rr2_s2_per_m2 * v * v) * (911.0 * G + q * 2.9)
    assert resistance_force(case, v) == pytest.approx(expected)


def test_traction_limit_is_mu_times_rear_load(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    assert traction_limit(case, 0.0) == pytest.approx(1.65 * case.mass.static_rear_load_n)
    v = 60.0
    rear_downforce = 0.58 * 0.5 * case.rho * v * v * 2.9
    assert traction_limit(case, v) == pytest.approx(
        1.65 * (case.mass.static_rear_load_n + rear_downforce)
    )


# ----------------------------------------------------------- acceleration run


def test_traction_limited_launch_matches_constant_acceleration(car, env, cal):
    # No aero and no rolling resistance: the launch is a constant mu * rear load.
    bare = replace(car, aero=replace(car.aero, drag_area_m2=0.0, downforce_area_m2=0.0))
    case = make_case(bare, env, replace(cal, c_rr0=0.0, c_rr2_s2_per_m2=0.0), FUEL_KG)
    v_target = 15.0
    assert powertrain.drive_force(bare, v_target) > traction_limit(case, v_target)

    a = traction_limit(case, 0.0) / powertrain.effective_mass(bare, case.mass.total_kg, 1)
    run = acceleration_run(case, v_target)
    assert run.time_to(v_target) == pytest.approx(v_target / a, rel=1e-9)
    assert run.time_to(10.0) == pytest.approx(10.0 / a, rel=1e-9)


def test_zero_to_300_time(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    run = acceleration_run(case, units.kmh_to_ms(300.0))
    t100, t200, t300 = (run.time_to(units.kmh_to_ms(k)) for k in (100.0, 200.0, 300.0))
    assert 0.0 < t100 < t200 < t300
    # Plausibility band for a 911 kg, 700 kW car with no load transfer yet.
    assert 9.0 < t300 < 13.0


def test_acceleration_run_has_converged_in_step_size(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    v300 = units.kmh_to_ms(300.0)
    coarse = acceleration_run(case, v300, ds=0.5).time_to(v300)
    fine = acceleration_run(case, v300, ds=0.125).time_to(v300)
    assert coarse == pytest.approx(fine, rel=2e-3)


def test_acceleration_run_is_monotonic_and_uses_every_gear_in_order(car, env, cal):
    case = make_case(car, env, cal, FUEL_KG)
    run = acceleration_run(case, units.kmh_to_ms(330.0))
    assert all(b > a for a, b in zip(run.speed_m_s, run.speed_m_s[1:]))
    assert all(b > a for a, b in zip(run.time_s, run.time_s[1:]))
    assert all(b - a in (0, 1) for a, b in zip(run.gear, run.gear[1:]))
    assert run.gear[0] == 1
    assert run.gear[-1] == 8


def test_unreachable_speed_is_reported_as_not_reached(car, env, cal):
    case = make_case(high_drag(car), env, cal, FUEL_KG)
    v_top, _ = top_speed(case)
    run = acceleration_run(case, v_top + 5.0, ds=1.0)
    assert run.time_to(v_top + 5.0) is None
    assert run.speed_m_s[-1] < v_top + 0.01


# ------------------------------------------------------------------- readouts


def test_rake_matches_the_core_doc_check(car):
    # Core doc: arctan((78 - 31) mm / 3600 mm) = 0.75 degrees.
    doc_car = replace(car, chassis=replace(car.chassis, wheelbase_m=3.6))
    assert units.rad_to_deg(rake_rad(doc_car)) == pytest.approx(0.75, abs=0.005)
    # The stock car now has the 3.40 m wheelbase.
    assert units.rad_to_deg(rake_rad(car)) == pytest.approx(0.79, abs=0.005)


def test_readouts_for_the_stock_car(car, env, cal):
    r = compute_readouts(car, env, cal, FUEL_KG)
    assert r.car_mass_kg == pytest.approx(881.0)
    assert r.total_mass_kg == pytest.approx(911.0)
    assert r.min_mass_kg == pytest.approx(770.0)
    assert r.overweight_kg == pytest.approx(111.0)
    assert r.front_weight_fraction == pytest.approx(0.445, abs=0.001)
    assert r.drag_area_m2 == 1.10
    assert units.w_to_kw(r.peak_power_w) == pytest.approx(0.97 * 723.0, abs=0.5)
    assert units.ms_to_kmh(r.top_gear_speed_m_s) == pytest.approx(340.0, abs=0.5)
    assert r.top_speed_limit == "gearing"
    assert r.top_speed_m_s == pytest.approx(r.top_gear_speed_m_s)
