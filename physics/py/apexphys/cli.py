"""Command line.

``python -m apexphys run --car CAR.json --track TRACK.json``
``python -m apexphys straight-line --car CAR.json [--plots DIR]``
"""

import argparse
import sys
from pathlib import Path

from apexphys import __version__, units
from apexphys.params import (
    DEFAULT_DATA_DIR,
    ParamError,
    default_environment,
    default_solver_config,
    load_car,
    load_physics_data,
    load_track,
)
from apexphys.readouts import compute_readouts
from apexphys.straight_line import acceleration_run, make_case
from apexphys.types import CarParams, Environment, SolverConfig, TrackModel


def summarize(
    car: CarParams, track: TrackModel, env: Environment, config: SolverConfig
) -> str:
    """Plain-ASCII summary of the loaded inputs, in the units the UI shows."""
    setup = car.setup
    parts = [
        ("chassis", car.chassis.name, car.chassis.m_base_kg),
        ("driver", "", car.chassis.driver_mass_kg),
        ("power unit", car.power_unit.name, car.power_unit.mass_kg),
        ("tires", car.tire.name, car.tire.mass_kg),
        ("brakes", car.brakes.name, car.brakes.mass_kg),
        ("aero", car.aero.name, car.aero.mass_kg),
        ("ballast", "", setup.ballast_kg),
    ]
    lines = [f"apexphys {__version__}", f"Car: {car.name} (era: {car.era.name})"]
    lines += [f"  {label:<11}{name:<24}{mass:7.1f} kg" for label, name, mass in parts]
    lines += [
        f"  dry parts: {sum(mass for _, _, mass in parts):.1f} kg (with driver, no fuel)",
        f"  wings {units.rad_to_deg(setup.front_wing_rad):.1f} / "
        f"{units.rad_to_deg(setup.rear_wing_rad):.1f} deg, "
        f"ride height {units.m_to_mm(setup.front_ride_height_m):.0f} / "
        f"{units.m_to_mm(setup.rear_ride_height_m):.0f} mm, "
        f"floor {units.frac_to_pct(setup.floor_setting):.0f} %",
        f"  pressures {units.pa_to_psi(setup.pressure_front_pa):.1f} / "
        f"{units.pa_to_psi(setup.pressure_rear_pa):.1f} psi, "
        f"map {setup.power_map}, final drive {setup.final_drive:.2f}, "
        f"shift {units.rad_s_to_rpm(setup.shift_omega_rad_s):.0f} rpm",
        f"  brake bias {units.frac_to_pct(setup.brake_bias_front):.0f} % front, "
        f"cooling {setup.brake_cooling}",
        f"Track: {track.name}, {track.length_m:.0f} m, "
        f"{len(track.segments)} segments, {track.corner_count} corners, "
        f"{track.sector_count} sectors",
        f"Environment: {env.preset}, air {units.k_to_c(env.air_temp_k):.1f} C, "
        f"track {units.k_to_c(env.track_temp_k):.1f} C, "
        f"humidity {units.frac_to_pct(env.relative_humidity):.0f} %, "
        f"wind {units.ms_to_kmh(env.wind_speed_m_s):.1f} km/h, "
        f"rain {units.frac_to_pct(env.rain_frac):.0f} %, surface {env.grip_state}",
        f"Solver: ds {config.ds_m:g} m, {config.laps} lap(s), "
        f"{'standing' if config.standing_start else 'flying'} start, "
        f"{config.start_fuel_kg:g} kg fuel",
    ]
    return "\n".join(lines)


def _run(args: argparse.Namespace) -> int:
    data = load_physics_data(args.data)
    car = load_car(args.car, data)
    track = load_track(args.track)
    print(summarize(car, track, default_environment(data), default_solver_config()))
    print("Inputs loaded. The lap solver is not implemented yet (phase P2).")
    return 0


def _straight_line(args: argparse.Namespace) -> int:
    data = load_physics_data(args.data)
    car = load_car(args.car, data)
    env = default_environment(data)
    fuel_kg = default_solver_config().start_fuel_kg if args.fuel_kg is None else args.fuel_kg
    cal = data.calibration

    r = compute_readouts(car, env, cal, fuel_kg)
    case = make_case(car, env, cal, fuel_kg)
    target = min(units.kmh_to_ms(300.0), 0.999 * r.top_speed_m_s)
    run = acceleration_run(case, target)

    print(f"apexphys {__version__}")
    print(f"Car: {car.name} (era: {car.era.name}), {fuel_kg:g} kg fuel")
    print(
        f"  mass {r.car_mass_kg:.1f} kg without fuel, {r.total_mass_kg:.1f} kg with fuel; "
        f"era minimum {r.min_mass_kg:.1f} kg"
    )
    print(
        f"  mandatory ballast {r.mandatory_ballast_kg:.1f} kg, "
        f"overweight {r.overweight_kg:.1f} kg"
    )
    print(
        f"  weight distribution {units.frac_to_pct(r.front_weight_fraction):.1f} % front, "
        f"rake {units.rad_to_deg(r.rake_rad):.2f} deg"
    )
    print(
        f"  drag area {r.drag_area_m2:.2f} m^2, "
        f"air density {r.air_density_kg_m3:.4f} kg/m^3"
    )
    print(
        f"  peak power {units.w_to_kw(r.peak_power_w):.0f} kW ({car.setup.power_map} map), "
        f"top-gear limit {units.ms_to_kmh(r.top_gear_speed_m_s):.1f} km/h"
    )
    print(
        f"  top speed {units.ms_to_kmh(r.top_speed_m_s):.1f} km/h "
        f"(limited by {r.top_speed_limit})"
    )
    for kmh in (100.0, 200.0, 300.0):
        t = run.time_to(units.kmh_to_ms(kmh))
        result = "not reached" if t is None else f"{t:.2f} s"
        print(f"  0-{kmh:.0f} km/h: {result}")

    if args.plots is not None:
        from apexphys import plots

        args.plots.mkdir(parents=True, exist_ok=True)
        plots.plot_force_envelope(case, args.plots / "force_envelope.png")
        plots.plot_acceleration_run(case, run, args.plots / "acceleration_run.png")
        print(f"Plots written to {args.plots}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="apexphys", description=__doc__)
    parser.add_argument("--version", action="version", version=__version__)
    commands = parser.add_subparsers(dest="command", required=True)

    run = commands.add_parser("run", help="load a car and a track and run the model")
    run.add_argument("--car", type=Path, required=True, help="car JSON file")
    run.add_argument("--track", type=Path, required=True, help="track JSON file")
    run.add_argument(
        "--data",
        type=Path,
        default=DEFAULT_DATA_DIR,
        help="parameter file directory (default: physics/data)",
    )
    run.set_defaults(handler=_run)

    straight = commands.add_parser(
        "straight-line", help="mass, top speed and standing-start acceleration"
    )
    straight.add_argument("--car", type=Path, required=True, help="car JSON file")
    straight.add_argument("--fuel-kg", type=float, help="fuel on board (default: 30)")
    straight.add_argument("--plots", type=Path, help="directory to write PNG plots to")
    straight.add_argument(
        "--data",
        type=Path,
        default=DEFAULT_DATA_DIR,
        help="parameter file directory (default: physics/data)",
    )
    straight.set_defaults(handler=_straight_line)

    args = parser.parse_args(argv)
    try:
        return args.handler(args)
    except ParamError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
