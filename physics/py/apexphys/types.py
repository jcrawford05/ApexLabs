"""Solver input and output types. All values are SI (see ``units.py``).

These are the physics-side structures, separate from the desktop UI structs
(``Vehicle``, ``Track``, ``Weather``, ...). ``params.py`` builds them from
JSON. Fields are added by the phase that first needs them, so a field that is
not here yet belongs to a module that has not been written.
"""

from dataclasses import dataclass

# ---------------------------------------------------------------- components


@dataclass(frozen=True)
class PowerUnit:
    name: str
    mass_kg: float
    peak_power_w: float
    max_omega_rad_s: float
    power_maps: tuple[str, ...]


@dataclass(frozen=True)
class TireCompound:
    name: str
    short_name: str
    mass_kg: float  # nominal mass of the full set of four
    mu_ref: float
    t_opt_k: float
    wear_rate_per_m: float


@dataclass(frozen=True)
class AeroPackage:
    name: str
    mass_kg: float
    drag_area_m2: float  # C_D * A
    downforce_area_m2: float  # C_L * A, positive = downforce
    balance_front: float  # fraction of downforce on the front axle


@dataclass(frozen=True)
class Chassis:
    name: str
    m_base_kg: float  # chassis, bodywork, suspension, cooling, electronics (C1)
    driver_mass_kg: float  # driver, seat and kit
    wheelbase_m: float
    cg_height_m: float
    tire_radius_m: float


@dataclass(frozen=True)
class BrakeSet:
    name: str
    mass_kg: float


@dataclass(frozen=True)
class RegulationRule:
    key: str
    value: float | str | bool
    unit: str
    severity: str  # "hard", "warn" or "info"
    source_article: str


@dataclass(frozen=True)
class RegulationEra:
    name: str
    is_sandbox: bool
    source: str  # the regulation text the rule values were read from
    rules: dict[str, RegulationRule]


@dataclass(frozen=True)
class Calibration:
    sea_level_pressure_pa: float


# ----------------------------------------------------------------------- car


@dataclass(frozen=True)
class CarSetup:
    """The settings a user changes on the Car Setup screen."""

    front_wing_rad: float
    rear_wing_rad: float
    front_ride_height_m: float
    rear_ride_height_m: float
    floor_setting: float  # 0..1
    pressure_front_pa: float  # cold, gauge
    pressure_rear_pa: float  # cold, gauge
    power_map: str
    final_drive: float
    shift_omega_rad_s: float
    ballast_kg: float
    ballast_front_frac: float  # 0..1
    brake_bias_front: float  # 0..1
    brake_cooling: str  # "Closed", "Standard" or "Open"


@dataclass(frozen=True)
class CarParams:
    name: str
    setup: CarSetup
    power_unit: PowerUnit
    tire: TireCompound
    aero: AeroPackage
    chassis: Chassis
    brakes: BrakeSet
    era: RegulationEra


# --------------------------------------------------------------------- track

SEGMENT_TYPES = ("straight", "braking", "corner")


@dataclass(frozen=True)
class TrackSegment:
    seq: int
    name: str
    type: str  # one of SEGMENT_TYPES
    sector: int
    length_m: float
    radius_m: float  # 0 unless type == "corner"

    @property
    def curvature_per_m(self) -> float:
        return 1.0 / self.radius_m if self.type == "corner" else 0.0


@dataclass(frozen=True)
class TrackModel:
    """Segment-level track. Phase P2 adds the points every ds that C8 solves on."""

    name: str
    segments: tuple[TrackSegment, ...]

    @property
    def length_m(self) -> float:
        return sum(s.length_m for s in self.segments)

    @property
    def corner_count(self) -> int:
        return sum(1 for s in self.segments if s.type == "corner")

    @property
    def sector_count(self) -> int:
        return len({s.sector for s in self.segments})


# ------------------------------------------------------- environment and run

GRIP_STATES = ("Green", "Rubbered-in", "Dusty")


@dataclass(frozen=True)
class Environment:
    preset: str
    air_temp_k: float
    track_temp_k: float
    relative_humidity: float  # 0..1
    wind_speed_m_s: float
    rain_frac: float  # 0..1
    grip_state: str  # one of GRIP_STATES
    roughness_frac: float  # 0..1
    track_evolution: bool
    sea_level_pressure_pa: float


@dataclass(frozen=True)
class SolverConfig:
    ds_m: float  # C8 distance step
    telemetry_spacing_m: float
    laps: int
    standing_start: bool
    start_fuel_kg: float


@dataclass(frozen=True)
class LapSolution:
    """One solved lap. Further channels (a_x, a_y, gear, rpm, ...) arrive with their modules."""

    distance_m: tuple[float, ...]
    speed_m_s: tuple[float, ...]
    lap_time_s: float
    sector_times_s: tuple[float, ...]
