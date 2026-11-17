import pytest

from apexphys import aero, resistance
from apexphys.atmosphere import (
    air_density,
    environment_air_density,
    saturation_vapor_pressure_pa,
)
from apexphys.units import c_to_k


def test_dry_air_at_standard_conditions():
    assert air_density(101325.0, c_to_k(15.0), 0.0) == pytest.approx(1.225, abs=5e-4)


def test_dry_air_is_the_ideal_gas_law():
    assert air_density(90000.0, 300.0, 0.0) == pytest.approx(90000.0 / (287.05 * 300.0))


def test_saturation_vapor_pressure_against_tables():
    # Steam-table values: 2339 Pa at 20 C, 4246 Pa at 30 C.
    assert saturation_vapor_pressure_pa(c_to_k(20.0)) == pytest.approx(2339.0, rel=0.01)
    assert saturation_vapor_pressure_pa(c_to_k(30.0)) == pytest.approx(4246.0, rel=0.01)


def test_humid_air_is_less_dense():
    dry = air_density(101325.0, c_to_k(30.0), 0.0)
    humid = air_density(101325.0, c_to_k(30.0), 1.0)
    assert humid < dry
    assert humid == pytest.approx(dry, rel=0.03)


def test_default_environment_density(env):
    rho = environment_air_density(env)
    assert rho == pytest.approx(air_density(101325.0, c_to_k(24.0), 0.42))
    assert 1.17 < rho < 1.19


def test_aero_forces(car):
    rho, v = 1.2, 80.0
    q = 0.5 * rho * v * v
    assert aero.drag_force(car.aero, rho, v) == pytest.approx(q * 1.10)
    assert aero.downforce(car.aero, rho, v) == pytest.approx(q * 2.9)
    front, rear = aero.axle_downforce(car.aero, rho, v)
    assert front == pytest.approx(0.42 * q * 2.9)
    assert front + rear == pytest.approx(q * 2.9)


def test_aero_forces_scale_with_speed_squared(car):
    assert aero.drag_force(car.aero, 1.2, 60.0) == pytest.approx(
        4.0 * aero.drag_force(car.aero, 1.2, 30.0)
    )


def test_rolling_resistance(cal):
    assert resistance.rolling_resistance_coeff(cal, 0.0) == cal.c_rr0
    assert resistance.rolling_resistance_coeff(cal, 50.0) == pytest.approx(
        cal.c_rr0 + cal.c_rr2_s2_per_m2 * 2500.0
    )
    assert resistance.rolling_resistance_force(cal, 0.0, 10000.0) == pytest.approx(
        cal.c_rr0 * 10000.0
    )
