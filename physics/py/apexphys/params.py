"""JSON loading: parameter files, car files and track files.

The files use the units people edit them in (psi, mm, degrees, rpm, kW,
percent, degrees Celsius). Everything is converted to SI here, so the rest of
the package never sees a non-SI value.
"""

import json
from dataclasses import dataclass
from pathlib import Path

from apexphys import units
from apexphys.types import (
    GRIP_STATES,
    SEGMENT_TYPES,
    AeroPackage,
    BrakeSet,
    Calibration,
    CarParams,
    CarSetup,
    Chassis,
    Environment,
    PowerUnit,
    RegulationEra,
    RegulationRule,
    SolverConfig,
    TireCompound,
    TrackModel,
    TrackSegment,
)

# physics/py/apexphys/params.py -> physics/data
DEFAULT_DATA_DIR = Path(__file__).resolve().parents[2] / "data"

SCHEMA_VERSION = 1
BRAKE_COOLING = ("Closed", "Standard", "Open")
RULE_SEVERITIES = ("hard", "warn", "info")

# Desktop UI defaults (desktop/src/vehicle.h) for the setup fields. A car file
# only has to list the ones it changes.
UI_DEFAULT_SETUP = {
    "front_wing_angle_deg": 8.0,
    "rear_wing_angle_deg": 12.0,
    "front_ride_height_mm": 31.0,
    "rear_ride_height_mm": 78.0,
    "floor_load_pct": 62.0,
    "tire_pressure_front_psi": 22.5,
    "tire_pressure_rear_psi": 21.8,
    "power_map": "Race",
    "final_drive": 3.2,
    "shift_rpm": 14500.0,
    "ballast_kg": 5.0,
    "ballast_front_pct": 50.0,
    "brake_bias_front_pct": 57.0,
    "brake_cooling": "Standard",
}


class ParamError(ValueError):
    """A parameter, car or track file is missing, malformed or inconsistent."""


@dataclass(frozen=True)
class PhysicsData:
    """Everything in ``physics/data``, keyed by component name."""

    power_units: dict[str, PowerUnit]
    tire_compounds: dict[str, TireCompound]
    aero_packages: dict[str, AeroPackage]
    chassis: dict[str, Chassis]
    brake_sets: dict[str, BrakeSet]
    eras: dict[str, RegulationEra]
    default_era: RegulationEra
    calibration: Calibration


# ------------------------------------------------------------------- helpers


def _read_json(path: Path) -> dict:
    path = Path(path)
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise ParamError(f"cannot read {path}: {exc.strerror}") from exc
    try:
        doc = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ParamError(f"{path}: invalid JSON ({exc})") from exc
    if not isinstance(doc, dict):
        raise ParamError(f"{path}: top level must be a JSON object")
    version = doc.get("schema_version")
    if version != SCHEMA_VERSION:
        raise ParamError(
            f"{path}: schema_version is {version!r}, expected {SCHEMA_VERSION}"
        )
    return doc


def _get(obj: dict, key: str, where: str):
    if not isinstance(obj, dict) or key not in obj:
        raise ParamError(f"{where}: missing '{key}'")
    return obj[key]


def _num(obj: dict, key: str, where: str) -> float:
    value = _get(obj, key, where)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ParamError(f"{where}: '{key}' must be a number, got {value!r}")
    return float(value)


def _choice(value, options: tuple[str, ...], key: str, where: str) -> str:
    if value not in options:
        raise ParamError(
            f"{where}: '{key}' is {value!r}, expected one of {', '.join(options)}"
        )
    return value


def _by_name(rows: list, build, where: str) -> dict:
    out = {}
    for i, row in enumerate(rows):
        item = build(row, f"{where}[{i}]")
        if item.name in out:
            raise ParamError(f"{where}: duplicate name '{item.name}'")
        out[item.name] = item
    return out


def _lookup(table: dict, name: str, kind: str, where: str):
    if name not in table:
        known = ", ".join(sorted(table)) or "none"
        raise ParamError(f"{where}: unknown {kind} '{name}' (known: {known})")
    return table[name]


# ----------------------------------------------------------- parameter files


def _power_unit(row: dict, where: str) -> PowerUnit:
    return PowerUnit(
        name=_get(row, "name", where),
        mass_kg=_num(row, "mass_kg", where),
        peak_power_w=units.kw_to_w(_num(row, "peak_power_kw", where)),
        max_omega_rad_s=units.rpm_to_rad_s(_num(row, "max_rpm", where)),
        power_maps=tuple(_get(row, "power_maps", where)),
    )


def _tire_compound(row: dict, where: str) -> TireCompound:
    return TireCompound(
        name=_get(row, "name", where),
        short_name=_get(row, "short", where),
        mass_kg=_num(row, "mass_kg", where),
        mu_ref=_num(row, "mu_ref", where),
        t_opt_k=units.c_to_k(_num(row, "t_opt_c", where)),
        wear_rate_per_m=_num(row, "wear_rate_per_km", where) / 1000.0,
    )


def _aero_package(row: dict, where: str) -> AeroPackage:
    return AeroPackage(
        name=_get(row, "name", where),
        mass_kg=_num(row, "mass_kg", where),
        drag_area_m2=_num(row, "drag_area_m2", where),
        downforce_area_m2=_num(row, "downforce_area_m2", where),
        balance_front=_num(row, "balance_front", where),
    )


def _chassis(row: dict, where: str) -> Chassis:
    return Chassis(
        name=_get(row, "name", where),
        m_base_kg=_num(row, "m_base_kg", where),
        driver_mass_kg=_num(row, "driver_mass_kg", where),
        wheelbase_m=_num(row, "wheelbase_m", where),
        cg_height_m=_num(row, "cg_height_m", where),
        tire_radius_m=_num(row, "tire_radius_m", where),
    )


def _brake_set(row: dict, where: str) -> BrakeSet:
    return BrakeSet(name=_get(row, "name", where), mass_kg=_num(row, "mass_kg", where))


def _era(row: dict, where: str) -> RegulationEra:
    rules = {}
    for i, r in enumerate(_get(row, "rules", where)):
        rule_where = f"{where}.rules[{i}]"
        key = _get(r, "rule_key", rule_where)
        if key in rules:
            raise ParamError(f"{where}: duplicate rule '{key}'")
        rules[key] = RegulationRule(
            key=key,
            value=_get(r, "value", rule_where),
            unit=_get(r, "unit", rule_where),
            severity=_choice(
                _get(r, "severity", rule_where), RULE_SEVERITIES, "severity", rule_where
            ),
            source_article=_get(r, "source_article", rule_where),
        )
    return RegulationEra(
        name=_get(row, "name", where),
        is_sandbox=bool(_get(row, "is_sandbox", where)),
        source=row.get("source", ""),
        rules=rules,
    )


def load_physics_data(data_dir: Path | str = DEFAULT_DATA_DIR) -> PhysicsData:
    data_dir = Path(data_dir)

    def table(filename: str, key: str, build) -> dict:
        path = data_dir / filename
        return _by_name(_get(_read_json(path), key, str(path)), build, f"{path}: {key}")

    eras_path = data_dir / "regulation_eras.json"
    eras = table("regulation_eras.json", "eras", _era)
    default_era = _lookup(
        eras,
        _get(_read_json(eras_path), "default_era", str(eras_path)),
        "regulation era",
        f"{eras_path}: default_era",
    )
    calibration_path = data_dir / "calibration.json"
    return PhysicsData(
        power_units=table("power_units.json", "power_units", _power_unit),
        tire_compounds=table("tire_compounds.json", "compounds", _tire_compound),
        aero_packages=table("aero_packages.json", "aero_packages", _aero_package),
        chassis=table("chassis.json", "chassis", _chassis),
        brake_sets=table("chassis.json", "brake_sets", _brake_set),
        eras=eras,
        default_era=default_era,
        calibration=Calibration(
            sea_level_pressure_pa=_num(
                _read_json(calibration_path),
                "sea_level_pressure_pa",
                str(calibration_path),
            )
        ),
    )


# ----------------------------------------------------------------- car files


def _setup(raw: dict, power_unit: PowerUnit, where: str) -> CarSetup:
    if not isinstance(raw, dict):
        raise ParamError(f"{where}: must be a JSON object")
    unknown = sorted(set(raw) - set(UI_DEFAULT_SETUP))
    if unknown:
        raise ParamError(f"{where}: unknown setting(s) {', '.join(unknown)}")
    s = {**UI_DEFAULT_SETUP, **raw}
    return CarSetup(
        front_wing_rad=units.deg_to_rad(_num(s, "front_wing_angle_deg", where)),
        rear_wing_rad=units.deg_to_rad(_num(s, "rear_wing_angle_deg", where)),
        front_ride_height_m=units.mm_to_m(_num(s, "front_ride_height_mm", where)),
        rear_ride_height_m=units.mm_to_m(_num(s, "rear_ride_height_mm", where)),
        floor_setting=units.pct_to_frac(_num(s, "floor_load_pct", where)),
        pressure_front_pa=units.psi_to_pa(_num(s, "tire_pressure_front_psi", where)),
        pressure_rear_pa=units.psi_to_pa(_num(s, "tire_pressure_rear_psi", where)),
        power_map=_choice(s["power_map"], power_unit.power_maps, "power_map", where),
        final_drive=_num(s, "final_drive", where),
        shift_omega_rad_s=units.rpm_to_rad_s(_num(s, "shift_rpm", where)),
        ballast_kg=_num(s, "ballast_kg", where),
        ballast_front_frac=units.pct_to_frac(_num(s, "ballast_front_pct", where)),
        brake_bias_front=units.pct_to_frac(_num(s, "brake_bias_front_pct", where)),
        brake_cooling=_choice(s["brake_cooling"], BRAKE_COOLING, "brake_cooling", where),
    )


def load_car(path: Path | str, data: PhysicsData) -> CarParams:
    """Load a car file in the app's vehicle format (database/data/sample_vehicle.json).

    Components are resolved by name against ``physics/data``. The property
    blocks embedded in the car file are ignored.
    """
    path = Path(path)
    doc = _read_json(path)
    components = _get(doc, "components", str(path))

    def component(category: str, table: dict):
        where = f"{path}: components.{category}"
        name = _get(_get(components, category, f"{path}: components"), "name", where)
        return _lookup(table, name, category, where)

    power_unit = component("engine", data.power_units)
    return CarParams(
        name=_get(doc, "name", str(path)),
        setup=_setup(doc.get("setup", {}), power_unit, f"{path}: setup"),
        power_unit=power_unit,
        tire=component("tire", data.tire_compounds),
        aero=component("aero", data.aero_packages),
        chassis=component("chassis", data.chassis),
        brakes=component("brakes", data.brake_sets),
        era=_lookup(
            data.eras, _get(doc, "regulation", str(path)), "regulation era", str(path)
        ),
    )


# --------------------------------------------------------------- track files


def load_track(path: Path | str) -> TrackModel:
    """Load a track file in the app's format (database/data/sample_track.json)."""
    path = Path(path)
    doc = _read_json(path)
    rows = _get(doc, "segments", str(path))
    if not rows:
        raise ParamError(f"{path}: track has no segments")

    segments = []
    for i, row in enumerate(rows):
        where = f"{path}: segments[{i}]"
        seq = _get(row, "seq", where)
        if seq != i + 1:
            raise ParamError(f"{where}: 'seq' is {seq!r}, expected {i + 1}")
        seg_type = _choice(_get(row, "type", where), SEGMENT_TYPES, "type", where)
        length_m = _num(row, "length_m", where)
        if length_m <= 0.0:
            raise ParamError(f"{where}: 'length_m' must be positive")
        radius_m = 0.0
        if seg_type == "corner":
            radius_m = _num(row, "radius_m", where)
            if radius_m <= 0.0:
                raise ParamError(f"{where}: 'radius_m' must be positive")
        sector = _get(row, "sector", where)
        if isinstance(sector, bool) or not isinstance(sector, int) or sector < 1:
            raise ParamError(f"{where}: 'sector' must be a positive integer")
        if segments and sector < segments[-1].sector:
            raise ParamError(f"{where}: 'sector' goes backwards")
        segments.append(
            TrackSegment(
                seq=seq,
                name=_get(row, "name", where),
                type=seg_type,
                sector=sector,
                length_m=length_m,
                radius_m=radius_m,
            )
        )
    return TrackModel(name=_get(doc, "name", str(path)), segments=tuple(segments))


# ------------------------------------------------- environment and run setup


def default_environment(data: PhysicsData) -> Environment:
    """Desktop UI defaults for Weather and Surface (desktop/src/track_setup.h)."""
    return Environment(
        preset="Dry",
        air_temp_k=units.c_to_k(24.0),
        track_temp_k=units.c_to_k(38.0),
        relative_humidity=units.pct_to_frac(42.0),
        wind_speed_m_s=units.kmh_to_ms(12.0),
        rain_frac=0.0,
        grip_state=GRIP_STATES[1],
        roughness_frac=units.pct_to_frac(30.0),
        track_evolution=True,
        sea_level_pressure_pa=data.calibration.sea_level_pressure_pa,
    )


def default_solver_config() -> SolverConfig:
    """Desktop UI defaults for Simulation, with the middle solver resolution (2 m)."""
    return SolverConfig(
        ds_m=2.0,
        telemetry_spacing_m=5.0,
        laps=1,
        standing_start=False,
        start_fuel_kg=30.0,
    )
