import math

import pytest

from apexphys import units


@pytest.mark.parametrize(
    ("func", "value", "expected"),
    [
        (units.psi_to_pa, 22.5, 155132.0325),
        (units.kmh_to_ms, 300.0, 83.3333333333),
        (units.knots_to_ms, 1.0, 0.5144444444),
        (units.c_to_k, 24.0, 297.15),
        (units.deg_to_rad, 180.0, math.pi),
        (units.rpm_to_rad_s, 15000.0, 1570.7963267949),
        (units.mm_to_m, 31.0, 0.031),
        (units.kw_to_w, 745.0, 745000.0),
        (units.pct_to_frac, 57.0, 0.57),
    ],
)
def test_known_values(func, value, expected):
    assert func(value) == pytest.approx(expected, rel=1e-10)


@pytest.mark.parametrize(
    ("forward", "back"),
    [
        (units.psi_to_pa, units.pa_to_psi),
        (units.kmh_to_ms, units.ms_to_kmh),
        (units.knots_to_ms, units.ms_to_knots),
        (units.c_to_k, units.k_to_c),
        (units.deg_to_rad, units.rad_to_deg),
        (units.rpm_to_rad_s, units.rad_s_to_rpm),
        (units.mm_to_m, units.m_to_mm),
        (units.kw_to_w, units.w_to_kw),
        (units.pct_to_frac, units.frac_to_pct),
    ],
)
def test_round_trips(forward, back):
    for value in (0.0, 1.0, 21.8, 14500.0):
        assert back(forward(value)) == pytest.approx(value, rel=1e-12, abs=1e-12)


def test_gauge_and_absolute_pressure():
    p_atm = units.STANDARD_SEA_LEVEL_PRESSURE_PA
    p_gauge = units.psi_to_pa(22.0)
    p_abs = units.gauge_to_abs(p_gauge, p_atm)
    assert p_abs == pytest.approx(253009.654)
    assert units.abs_to_gauge(p_abs, p_atm) == pytest.approx(p_gauge)


def test_hydroplaning_coefficient_from_integration_plan():
    # Integration plan section 3: V_p = 9 sqrt(p) knots is about 4.63 sqrt(p) m/s,
    # and 22 psi gives about 42 knots, about 78 km/h.
    assert units.knots_to_ms(9.0) == pytest.approx(4.63, abs=0.005)
    v_p = units.knots_to_ms(9.0 * math.sqrt(22.0))
    assert units.ms_to_knots(v_p) == pytest.approx(42.0, abs=0.5)
    assert units.ms_to_kmh(v_p) == pytest.approx(78.0, abs=0.5)
