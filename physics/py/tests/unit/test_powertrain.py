from dataclasses import replace

import pytest

from apexphys import powertrain, units


def with_setup(car, **changes):
    return replace(car, setup=replace(car.setup, **changes))


def test_torque_curve_interpolation(car):
    pu = car.power_unit
    assert powertrain.engine_torque(pu, units.rpm_to_rad_s(10500.0)) == pytest.approx(657.5)
    assert powertrain.engine_torque(pu, units.rpm_to_rad_s(10750.0)) == pytest.approx(
        0.5 * (657.5 + 627.6)
    )


def test_torque_outside_the_curve(car):
    pu = car.power_unit
    assert powertrain.engine_torque(pu, 0.0) == 869.5
    assert powertrain.engine_torque(pu, units.rpm_to_rad_s(15000.0)) == pytest.approx(460.3)
    assert powertrain.engine_torque(pu, units.rpm_to_rad_s(15001.0)) == 0.0


def test_peak_power_matches_the_2026_target(car):
    # Decision D3: 723 kW on the full (Qualifying) map.
    qualifying = with_setup(car, power_map="Qualifying")
    assert units.w_to_kw(powertrain.peak_power(qualifying)) == pytest.approx(723.0, abs=0.5)


def test_power_map_scales_power(car):
    full = powertrain.peak_power(with_setup(car, power_map="Qualifying"))
    assert powertrain.peak_power(with_setup(car, power_map="Race")) == pytest.approx(0.97 * full)
    assert powertrain.peak_power(with_setup(car, power_map="Save")) == pytest.approx(0.92 * full)


def test_top_gear_speed_limit(car):
    # v = omega_max * r_w / (i_top * i_fd); the UI shows 340 km/h at a 3.20 final drive.
    limit = powertrain.gear_speed_limit(car, 8)
    assert limit == pytest.approx(units.rpm_to_rad_s(15000.0) * 0.33 / (1.715 * 3.2))
    assert units.ms_to_kmh(limit) == pytest.approx(340.0, abs=0.5)


def test_shorter_final_drive_lowers_every_gear_limit(car):
    short = with_setup(car, final_drive=3.6)
    for gear in range(1, 9):
        assert powertrain.gear_speed_limit(short, gear) == pytest.approx(
            powertrain.gear_speed_limit(car, gear) * 3.2 / 3.6
        )


def test_engine_speed_and_gear_limit_are_consistent(car):
    for gear in range(1, 9):
        v = powertrain.gear_speed_limit(car, gear)
        assert powertrain.engine_speed(car, v, gear) == pytest.approx(
            car.power_unit.max_omega_rad_s
        )


def test_gear_selection_upshifts_at_the_shift_point(car):
    assert powertrain.select_gear(car, 0.0) == 1
    for gear in range(1, 8):
        v_shift = car.setup.shift_omega_rad_s * 0.33 / powertrain.total_ratio(car, gear)
        assert powertrain.select_gear(car, 0.999 * v_shift) == gear
        assert powertrain.select_gear(car, 1.001 * v_shift) == gear + 1
    assert powertrain.select_gear(car, 200.0) == 8


def test_lower_shift_point_upshifts_earlier(car):
    early = with_setup(car, shift_omega_rad_s=units.rpm_to_rad_s(11000.0))
    v = 0.9 * powertrain.gear_speed_limit(car, 1)
    assert powertrain.select_gear(car, v) == 1
    assert powertrain.select_gear(early, v) == 2


def test_wheel_force_is_power_over_speed_in_the_flat_power_band(car):
    qualifying = with_setup(car, power_map="Qualifying")
    v = units.kmh_to_ms(300.0)
    rpm = units.rad_s_to_rpm(powertrain.engine_speed(qualifying, v, 8))
    assert 10500.0 < rpm < 15000.0
    assert powertrain.gear_force(qualifying, v, 8) * v == pytest.approx(
        0.95 * 723000.0, rel=2e-3
    )


def test_wheel_force_formula(car):
    v, gear = 30.0, 2
    omega = v * 3.837 * 3.2 / 0.33
    expected = 0.97 * powertrain.engine_torque(car.power_unit, omega) * 3.837 * 3.2 * 0.95 / 0.33
    assert powertrain.gear_force(car, v, gear) == pytest.approx(expected)


def test_no_drive_force_beyond_the_rev_limit(car):
    assert powertrain.gear_force(car, 1.01 * powertrain.gear_speed_limit(car, 3), 3) == 0.0


def test_envelope_is_never_below_the_shifted_force(car):
    for kmh in range(5, 340, 5):
        v = units.kmh_to_ms(kmh)
        assert powertrain.force_envelope(car, v) >= powertrain.drive_force(car, v)


def test_effective_mass(car):
    m = 900.0
    ratio = 4.720 * 3.2
    expected = m + (4.0 + 0.06 * ratio**2 * 0.95) / 0.33**2
    assert powertrain.effective_mass(car, m, 1) == pytest.approx(expected)
    assert powertrain.effective_mass(car, m, 1) > powertrain.effective_mass(car, m, 8) > m
