from pathlib import Path

import pytest

from apexphys.params import load_physics_data

REPO_ROOT = Path(__file__).resolve().parents[3]
SAMPLE_CAR = REPO_ROOT / "database" / "data" / "sample_vehicle.json"
SAMPLE_TRACK = REPO_ROOT / "database" / "data" / "sample_track.json"


@pytest.fixture(scope="session")
def data():
    return load_physics_data()
