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
