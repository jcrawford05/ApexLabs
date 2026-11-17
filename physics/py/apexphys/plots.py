"""Development plots. Not part of the model; nothing here is ported to C++."""

from pathlib import Path

from apexphys import powertrain, units
from apexphys.types import CarParams, LapSolution, TrackModel
from apexphys.straight_line import (
    AccelerationRun,
    StraightLineCase,
    resistance_force,
    traction_limit,
)


def _pyplot():
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    return plt


def plot_force_envelope(case: StraightLineCase, path: Path) -> None:
    """Tractive-force diagram: one curve per gear, traction limit and resistance (3A plot 2)."""
    plt = _pyplot()
    car = case.car
    gears = range(1, powertrain.gear_count(car) + 1)
    v_max = powertrain.gear_speed_limit(car, powertrain.gear_count(car))
    idle_omega = car.power_unit.torque_curve[0][0]
    speeds = [v_max * i / 600.0 for i in range(1, 601)]

    fig, ax = plt.subplots(figsize=(9.0, 5.5))
    for gear in gears:
        points = [
            (v, powertrain.gear_force(car, v, gear))
            for v in speeds
            if idle_omega <= powertrain.engine_speed(car, v, gear)
            <= car.power_unit.max_omega_rad_s
        ]
        ax.plot(
            [units.ms_to_kmh(v) for v, _ in points],
            [f / 1000.0 for _, f in points],
            label=f"Gear {gear}",
        )
    kmh = [units.ms_to_kmh(v) for v in speeds]
    ax.plot(
        kmh,
        [traction_limit(case, v) / 1000.0 for v in speeds],
        "k--",
        label="Traction limit",
    )
    ax.plot(
        kmh,
        [resistance_force(case, v) / 1000.0 for v in speeds],
        "k:",
        label="Drag + rolling resistance",
    )
    ax.set_xlabel("Speed (km/h)")
    ax.set_ylabel("Wheel force (kN)")
    ax.set_title(f"{car.name}: tractive force by gear ({car.setup.power_map} map)")
    ax.set_xlim(0.0, units.ms_to_kmh(v_max) * 1.02)
    ax.set_ylim(bottom=0.0)
    ax.grid(True, alpha=0.3)
    ax.legend(ncol=2, fontsize=8)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def plot_acceleration_run(case: StraightLineCase, run: AccelerationRun, path: Path) -> None:
    """Speed and gear against time for a standing-start run."""
    plt = _pyplot()
    fig, (ax_v, ax_g) = plt.subplots(
        2, 1, figsize=(9.0, 6.0), sharex=True, height_ratios=(3, 1)
    )
    ax_v.plot(run.time_s, [units.ms_to_kmh(v) for v in run.speed_m_s])
    ax_v.set_ylabel("Speed (km/h)")
    ax_v.set_title(f"{case.car.name}: standing-start acceleration")
    ax_v.grid(True, alpha=0.3)
    ax_g.step(run.time_s, run.gear, where="post")
    ax_g.set_ylabel("Gear")
    ax_g.set_xlabel("Time (s)")
    ax_g.set_yticks(range(1, powertrain.gear_count(case.car) + 1))
    ax_g.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def plot_speed_trace(
    car: CarParams, track: TrackModel, lap: LapSolution, path: Path
) -> None:
    """Speed, throttle and brake against distance, with segment boundaries."""
    plt = _pyplot()
    step_start = lap.distance_m[:-1]
    fig, (ax_v, ax_p) = plt.subplots(
        2, 1, figsize=(11.0, 6.0), sharex=True, height_ratios=(3, 1)
    )
    ax_v.plot(lap.distance_m, [units.ms_to_kmh(v) for v in lap.speed_m_s])
    ax_v.set_ylabel("Speed (km/h)")
    ax_v.set_title(f"{car.name} on {track.name}: lap {lap.lap_time_s:.3f} s")
    ax_p.step(step_start, [100.0 * x for x in lap.throttle], where="post", label="Throttle")
    ax_p.step(step_start, [100.0 * x for x in lap.brake], where="post", label="Brake")
    ax_p.set_ylabel("Pedal (%)")
    ax_p.set_xlabel("Distance (m)")
    ax_p.legend(loc="center right", fontsize=8)
    boundary = 0.0
    for seg in track.segments:
        for ax in (ax_v, ax_p):
            ax.axvline(boundary, color="0.8", linewidth=0.8)
        ax_v.annotate(
            str(seg.seq),
            (boundary + 0.5 * seg.length_m, 1.0),
            xycoords=("data", "axes fraction"),
            xytext=(0, -12),
            textcoords="offset points",
            ha="center",
            fontsize=8,
            color="0.4",
        )
        boundary += seg.length_m
    ax_v.set_xlim(0.0, lap.distance_m[-1])
    ax_v.set_ylim(bottom=0.0)
    ax_v.grid(True, axis="y", alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
