"""Write the golden test vectors in ``physics/golden``.

The Python reference is the source of truth: this script records its outputs
for fixed inputs, and the C++ port must reproduce them (integration plan,
section 5.1). The Python regression test checks the checked-in files against
the current code, so after an intended model or data change, rerun this script
and state the change in the merge request.

    python tools/make_golden.py            (from physics/py)

All values are SI. Paths inside the files are relative to the repository root.
"""

import json
import math
import sys
from dataclasses import replace
from pathlib import Path

from apexphys import __version__, powertrain, solver, straight_line
from apexphys.atmosphere import air_density
from apexphys.mass import mass_properties
from apexphys.params import (
    default_environment,
    default_solver_config,
    load_car,
    load_physics_data,
    load_track,
)

REPO_ROOT = Path(__file__).resolve().parents[3]
GOLDEN_DIR = REPO_ROOT / "physics" / "golden"
CAR = "database/data/sample_vehicle.json"
TRACK = "database/data/sample_track.json"

SPEEDS = (0.0, 5.0, 20.0, 40.0, 60.0, 80.0, 94.0)
CURVATURES = (0.0, 1.0 / 400.0, 1.0 / 180.0, 1.0 / 60.0, 1.0 / 25.0)


def _finite(value: float):
    """JSON has no infinity: an unlimited corner speed is stored as null."""
    return None if math.isinf(value) else value


def _header(env, fuel_kg: float) -> dict:
    return {
        "schema_version": 1,
        "generator": f"apexphys {__version__}",
        "car": CAR,
        "fuel_kg": fuel_kg,
        "environment": {
            "air_temp_k": env.air_temp_k,
            "relative_humidity": env.relative_humidity,
            "sea_level_pressure_pa": env.sea_level_pressure_pa,
        },
    }


def _function_cases(car, case) -> list[dict]:
    cases = []

    def add(function: str, args: dict, expected) -> None:
        cases.append({"function": function, "args": args, "expected": expected})

    for pressure_pa, temp_k, rh in (
        (101325.0, 288.15, 0.0),
        (101325.0, 297.15, 0.42),
        (90000.0, 308.15, 0.9),
    ):
        add(
            "air_density",
            {"pressure_pa": pressure_pa, "temp_k": temp_k, "relative_humidity": rh},
            air_density(pressure_pa, temp_k, rh),
        )

    for fuel_kg in (0.0, 30.0, 110.0):
        m = mass_properties(car, fuel_kg)
        add(
            "mass_properties",
            {"fuel_kg": fuel_kg},
            {
                "dry_parts_kg": m.dry_parts_kg,
                "min_mass_kg": m.min_mass_kg,
                "mandatory_ballast_kg": m.mandatory_ballast_kg,
                "overweight_kg": m.overweight_kg,
                "total_kg": m.total_kg,
                "x_cg_m": m.x_cg_m,
                "front_fraction": m.front_fraction,
            },
        )

    for omega in (0.0, 400.0, 733.0, 1100.0, 1333.3, 1570.0, 1600.0):
        add(
            "engine_torque",
            {"omega_rad_s": omega},
            powertrain.engine_torque(car.power_unit, omega),
        )
    add("peak_power", {}, powertrain.peak_power(car))

    for gear in range(1, powertrain.gear_count(car) + 1):
        add("gear_speed_limit", {"gear": gear}, powertrain.gear_speed_limit(car, gear))
        add(
            "effective_mass",
            {"mass_kg": case.mass.total_kg, "gear": gear},
            powertrain.effective_mass(car, case.mass.total_kg, gear),
        )
        for v in (10.0, 45.0, 90.0):
            add(
                "gear_force",
                {"v_m_s": v, "gear": gear},
                powertrain.gear_force(car, v, gear),
            )

    for v in SPEEDS:
        args = {"v_m_s": v}
        add("select_gear", args, powertrain.select_gear(car, v))
        add("drive_force", args, powertrain.drive_force(car, v))
        add("force_envelope", args, powertrain.force_envelope(car, v))
        add("traction_limit", args, straight_line.traction_limit(case, v))
        add("resistance_force", args, straight_line.resistance_force(case, v))
        add("normal_load", args, solver.normal_load(case, v))

    for curvature in CURVATURES:
        add(
            "corner_speed_limit",
            {"curvature_per_m": curvature},
            _finite(solver.corner_speed_limit(case, curvature)),
        )
        for v in SPEEDS:
            args = {"v_m_s": v, "curvature_per_m": curvature}
            add("lateral_use", args, solver.lateral_use(case, v, curvature))
            add("max_acceleration", args, solver.max_acceleration(case, v, curvature))
            add("max_deceleration", args, solver.max_deceleration(case, v, curvature))

    v_top, limit = straight_line.top_speed(case)
    add("top_speed", {}, {"v_m_s": v_top, "limit": limit})
    return cases


def _lap(car, track, env, cal, config) -> dict:
    lap = solver.solve_lap(car, track, env, cal, config)
    return {
        **_header(env, config.start_fuel_kg),
        "track": TRACK,
        "ds_m": config.ds_m,
        "standing_start": config.standing_start,
        "lap_time_s": lap.lap_time_s,
        "sector_times_s": list(lap.sector_times_s),
        "segment_times_s": list(lap.segment_times_s),
        "segment_min_speed_m_s": list(lap.segment_min_speed_m_s),
        "segment_peak_brake_m_s2": list(lap.segment_peak_brake_m_s2),
        "top_speed_m_s": lap.top_speed_m_s,
        "top_speed_at_m": lap.top_speed_at_m,
        "speed_m_s": list(lap.speed_m_s),
        "gear": list(lap.gear),
        "throttle": list(lap.throttle),
        "brake": list(lap.brake),
    }


def build_golden() -> dict[str, dict]:
    """File name -> document, for every golden file."""
    data = load_physics_data()
    car = load_car(REPO_ROOT / CAR, data)
    track = load_track(REPO_ROOT / TRACK)
    env = default_environment(data)
    cal = data.calibration
    config = default_solver_config()
    case = straight_line.make_case(car, env, cal, config.start_fuel_kg)
    return {
        "functions.json": {
            **_header(env, config.start_fuel_kg),
            "cases": _function_cases(car, case),
        },
        "demo_circuit_flying.json": _lap(car, track, env, cal, config),
        "demo_circuit_standing.json": _lap(
            car, track, env, cal, replace(config, standing_start=True)
        ),
    }


def to_text(doc: dict) -> str:
    """One top-level key per line, and one test case per line."""
    lines = []
    for key, value in doc.items():
        if isinstance(value, list) and value and isinstance(value[0], dict):
            body = ",\n".join("    " + json.dumps(item) for item in value)
            lines.append(f"  {json.dumps(key)}: [\n{body}\n  ]")
        else:
            lines.append(f"  {json.dumps(key)}: {json.dumps(value)}")
    return "{\n" + ",\n".join(lines) + "\n}\n"


def main() -> int:
    GOLDEN_DIR.mkdir(parents=True, exist_ok=True)
    for name, doc in build_golden().items():
        (GOLDEN_DIR / name).write_text(to_text(doc), encoding="utf-8", newline="\n")
        print(f"wrote {GOLDEN_DIR / name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
