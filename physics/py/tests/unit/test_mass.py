from dataclasses import replace

import pytest

from apexphys.mass import mass_properties
from apexphys.units import G


def test_stock_car_mass_totals(car):
    m = mass_properties(car, fuel_kg=30.0)
    # 540 base + 82 driver + 150 engine + 46 tires + 40 brakes + 18 aero + 5 ballast
    assert m.dry_parts_kg == pytest.approx(881.0)
    assert m.min_mass_kg == pytest.approx(724.0 + 46.0)
    assert m.mandatory_ballast_kg == 0.0
    assert m.overweight_kg == pytest.approx(111.0)
    assert m.car_mass_kg == pytest.approx(881.0)
    assert m.total_kg == pytest.approx(911.0)


def test_stock_car_cg_and_static_axle_loads(car):
    m = mass_properties(car, fuel_kg=30.0)
    moment = (
        540 * 0.55 + 82 * 0.42 + 150 * 0.68 + 46 * 0.55 + 40 * 0.45 + 18 * 0.58
        + 5 * 0.50 + 30 * 0.52
    )
    assert m.x_cg_m == pytest.approx(moment / 911.0 * 3.40)
    assert m.front_fraction == pytest.approx(1.0 - moment / 911.0)
    assert m.static_front_load_n + m.static_rear_load_n == pytest.approx(911.0 * G)
    assert m.static_front_load_n == pytest.approx(911.0 * G * m.front_fraction)


def test_stock_weight_distribution_is_inside_the_era_limits(car):
    m = mass_properties(car, fuel_kg=0.0)
    rules = car.era.rules
    assert m.front_fraction >= rules["front_axle_mass_fraction_min"].value
    assert 1.0 - m.front_fraction >= rules["rear_axle_mass_fraction_min"].value


def test_light_car_is_ballasted_up_to_the_minimum(car):
    light = replace(car, chassis=replace(car.chassis, m_base_kg=400.0))
    m = mass_properties(light, fuel_kg=10.0)
    assert m.dry_parts_kg == pytest.approx(741.0)
    assert m.mandatory_ballast_kg == pytest.approx(29.0)
    assert m.overweight_kg == 0.0
    assert m.car_mass_kg == pytest.approx(770.0)
    assert m.total_kg == pytest.approx(780.0)


def test_minimum_mass_follows_the_tire_set_mass(car, data):
    soft = replace(car, tire=data.tire_compounds["Soft Compound"])
    assert mass_properties(soft, 0.0).min_mass_kg == pytest.approx(724.0 + 44.0)


def test_driver_below_the_reference_mass_counts_as_the_reference_mass(car):
    light_driver = replace(car, chassis=replace(car.chassis, driver_mass_kg=65.0))
    assert mass_properties(light_driver, 0.0).dry_parts_kg == pytest.approx(881.0)
    heavy_driver = replace(car, chassis=replace(car.chassis, driver_mass_kg=90.0))
    assert mass_properties(heavy_driver, 0.0).dry_parts_kg == pytest.approx(889.0)


def test_sandbox_era_has_no_minimum(car, data):
    sandbox = replace(
        car,
        era=data.eras["Open (no limits)"],
        chassis=replace(car.chassis, m_base_kg=300.0, driver_mass_kg=60.0),
    )
    m = mass_properties(sandbox, 0.0)
    assert m.dry_parts_kg == pytest.approx(619.0)
    assert m.min_mass_kg == 0.0
    assert m.mandatory_ballast_kg == 0.0
    assert m.overweight_kg == 0.0


def test_ballast_position_moves_the_cg(car):
    def front_fraction(front_frac):
        setup = replace(car.setup, ballast_kg=20.0, ballast_front_frac=front_frac)
        return mass_properties(replace(car, setup=setup), 0.0).front_fraction

    forward, rearward = front_fraction(1.0), front_fraction(0.0)
    # 20 kg moved one wheelbase changes the front fraction by 20 / total mass.
    assert forward - rearward == pytest.approx(20.0 / 896.0)


def test_fuel_adds_mass_but_not_car_mass(car):
    empty, full = mass_properties(car, 0.0), mass_properties(car, 100.0)
    assert full.total_kg - empty.total_kg == pytest.approx(100.0)
    assert full.car_mass_kg == pytest.approx(empty.car_mass_kg)
