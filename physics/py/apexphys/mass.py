"""C1. Mass, CG and weight distribution."""

from dataclasses import dataclass

from apexphys.types import CarParams
from apexphys.units import G


@dataclass(frozen=True)
class MassProperties:
    dry_parts_kg: float  # selected parts + driver + user ballast
    min_mass_kg: float  # era minimum without fuel; 0 when the era has none
    mandatory_ballast_kg: float  # added to reach the minimum
    overweight_kg: float  # dry parts above the minimum
    fuel_kg: float
    total_kg: float  # dry parts + mandatory ballast + fuel
    wheelbase_m: float
    x_cg_m: float  # CG distance behind the front axle (a)

    @property
    def car_mass_kg(self) -> float:
        """Mass without fuel, which is what the regulations weigh."""
        return self.total_kg - self.fuel_kg

    @property
    def front_fraction(self) -> float:
        return 1.0 - self.x_cg_m / self.wheelbase_m

    @property
    def static_front_load_n(self) -> float:
        return self.total_kg * G * self.front_fraction

    @property
    def static_rear_load_n(self) -> float:
        return self.total_kg * G * (1.0 - self.front_fraction)


def _rule(car: CarParams, key: str) -> float:
    rule = car.era.rules.get(key)
    return float(rule.value) if rule else 0.0


def mass_properties(car: CarParams, fuel_kg: float) -> MassProperties:
    """Bottom-up mass and CG for the car carrying ``fuel_kg`` of fuel."""
    chassis = car.chassis
    pos = chassis.position_frac
    driver_kg = max(chassis.driver_mass_kg, _rule(car, "min_driver_mass"))

    # (mass, position as a fraction of the wheelbase behind the front axle)
    dry_parts = [
        (chassis.m_base_kg, pos["base"]),
        (driver_kg, pos["driver"]),
        (car.power_unit.mass_kg, pos["engine"]),
        (car.tire.mass_kg, pos["tires"]),
        (car.brakes.mass_kg, pos["brakes"]),
        (car.aero.mass_kg, pos["aero"]),
        # A ballast "front %" of f puts that share of the ballast weight on
        # the front axle, which is a position of (1 - f) wheelbases.
        (car.setup.ballast_kg, 1.0 - car.setup.ballast_front_frac),
    ]
    dry_parts_kg = sum(m for m, _ in dry_parts)

    # The era minimum is "724 kg plus the Nominal Tyre Mass" (FIA C4.1); the
    # selected compound's set mass stands in for the nominal tyre mass.
    min_mass_kg = 0.0
    if "min_mass_no_fuel" in car.era.rules:
        min_mass_kg = _rule(car, "min_mass_no_fuel") + car.tire.mass_kg
    mandatory_ballast_kg = max(0.0, min_mass_kg - dry_parts_kg)
    overweight_kg = max(0.0, dry_parts_kg - min_mass_kg) if min_mass_kg else 0.0

    parts = dry_parts + [
        (mandatory_ballast_kg, pos["mandatory_ballast"]),
        (fuel_kg, pos["fuel"]),
    ]
    total_kg = sum(m for m, _ in parts)
    x_cg_frac = sum(m * x for m, x in parts) / total_kg

    return MassProperties(
        dry_parts_kg=dry_parts_kg,
        min_mass_kg=min_mass_kg,
        mandatory_ballast_kg=mandatory_ballast_kg,
        overweight_kg=overweight_kg,
        fuel_kg=fuel_kg,
        total_kg=total_kg,
        wheelbase_m=chassis.wheelbase_m,
        x_cg_m=x_cg_frac * chassis.wheelbase_m,
    )
