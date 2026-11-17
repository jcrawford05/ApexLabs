import subprocess
import sys

from apexphys.cli import main
from conftest import SAMPLE_CAR, SAMPLE_TRACK


def test_run_loads_the_sample_car_and_demo_circuit(capsys):
    code = main(["run", "--car", str(SAMPLE_CAR), "--track", str(SAMPLE_TRACK)])
    out = capsys.readouterr().out
    assert code == 0
    assert "Car: Low Drag Monza Setup (era: Apex 2026 Default)" in out
    # Vehicle::TotalMassKg() in the desktop app (799 kg) plus the 82 kg driver.
    assert "dry parts: 881.0 kg (with driver, no fuel)" in out
    assert "Track: Demo Circuit, 4000 m, 12 segments, 4 corners, 3 sectors" in out
    assert "air 24.0 C, track 38.0 C" in out
    assert "flying start" in out
    assert "Lap time: 1:0" in out
    assert "T1 hairpin" in out
    out.encode("ascii")  # stays printable on any Windows console code page


def test_bad_input_prints_one_error_line_and_exits_2(tmp_path, capsys):
    code = main(["run", "--car", str(tmp_path / "nope.json"), "--track", str(SAMPLE_TRACK)])
    captured = capsys.readouterr()
    assert code == 2
    assert captured.out == ""
    assert captured.err.startswith("error: cannot read")
    assert captured.err.count("\n") == 1


def test_python_dash_m_entry_point():
    result = subprocess.run(
        [sys.executable, "-m", "apexphys", "run", "--car", str(SAMPLE_CAR), "--track", str(SAMPLE_TRACK)],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    assert "Demo Circuit" in result.stdout


def test_straight_line_reports_readouts_and_writes_plots(tmp_path, capsys):
    code = main(["straight-line", "--car", str(SAMPLE_CAR), "--plots", str(tmp_path / "plots")])
    out = capsys.readouterr().out
    assert code == 0
    assert "mass 881.0 kg without fuel, 911.0 kg with fuel; era minimum 770.0 kg" in out
    assert "top speed 340.0 km/h (limited by gearing)" in out
    assert "0-300 km/h:" in out and "not reached" not in out
    out.encode("ascii")
    for name in ("force_envelope.png", "acceleration_run.png"):
        assert (tmp_path / "plots" / name).stat().st_size > 10_000


def test_straight_line_fuel_option(capsys):
    assert main(["straight-line", "--car", str(SAMPLE_CAR), "--fuel-kg", "100"]) == 0
    assert "981.0 kg with fuel" in capsys.readouterr().out


def test_run_options(tmp_path, capsys):
    args = ["run", "--car", str(SAMPLE_CAR), "--track", str(SAMPLE_TRACK)]
    assert main(args + ["--ds", "5", "--fuel-kg", "100", "--standing-start",
                        "--plots", str(tmp_path)]) == 0
    out = capsys.readouterr().out
    assert "Solver: ds 5 m, 1 lap(s), standing start, 100 kg fuel" in out
    assert (tmp_path / "speed_trace.png").stat().st_size > 10_000


def test_run_rejects_a_non_positive_step(capsys):
    assert main(["run", "--car", str(SAMPLE_CAR), "--track", str(SAMPLE_TRACK), "--ds", "0"]) == 2
    assert "error: --ds must be positive" in capsys.readouterr().err
