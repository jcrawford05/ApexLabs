from pathlib import Path

import pytest

from apexphys.params import default_environment, load_car, load_physics_data

REPO_ROOT = Path(__file__).resolve().parents[3]
SAMPLE_CAR = REPO_ROOT / "database" / "data" / "sample_vehicle.json"
SAMPLE_TRACK = REPO_ROOT / "database" / "data" / "sample_track.json"


@pytest.fixture(scope="session")
def data():
    return load_physics_data()


@pytest.fixture(scope="session")
def car(data):
    return load_car(SAMPLE_CAR, data)


@pytest.fixture(scope="session")
def env(data):
    return default_environment(data)


@pytest.fixture(scope="session")
def cal(data):
    return data.calibration
