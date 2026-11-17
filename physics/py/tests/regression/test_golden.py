"""The checked-in golden files must match what the current code produces.

A failure here means the model or its data changed. If that was intended,
rerun ``python tools/make_golden.py`` and state the change in the merge request.
"""

import importlib.util
import json
from pathlib import Path

import pytest

TOOL = Path(__file__).resolve().parents[2] / "tools" / "make_golden.py"
spec = importlib.util.spec_from_file_location("make_golden", TOOL)
make_golden = importlib.util.module_from_spec(spec)
spec.loader.exec_module(make_golden)

REL_TOLERANCE = 1e-9


@pytest.fixture(scope="module")
def generated():
    # Through text and back, so the comparison sees exactly what a file holds.
    return {
        name: json.loads(make_golden.to_text(doc))
        for name, doc in make_golden.build_golden().items()
    }


def assert_matches(actual, expected, where):
    if isinstance(expected, dict):
        assert isinstance(actual, dict) and actual.keys() == expected.keys(), where
        for key in expected:
            assert_matches(actual[key], expected[key], f"{where}.{key}")
    elif isinstance(expected, list):
        assert isinstance(actual, list) and len(actual) == len(expected), where
        for i, (a, e) in enumerate(zip(actual, expected)):
            assert_matches(a, e, f"{where}[{i}]")
    elif isinstance(expected, float):
        assert actual == pytest.approx(expected, rel=REL_TOLERANCE, abs=1e-12), where
    else:
        assert actual == expected, where


@pytest.mark.parametrize(
    "name",
    ["functions.json", "demo_circuit_flying.json", "demo_circuit_standing.json"],
)
def test_golden_file_matches_the_current_model(generated, name):
    path = make_golden.GOLDEN_DIR / name
    checked_in = json.loads(path.read_text(encoding="utf-8"))
    assert_matches(generated[name], checked_in, name)


def test_every_golden_file_is_generated():
    on_disk = {p.name for p in make_golden.GOLDEN_DIR.glob("*.json")}
    assert on_disk == set(make_golden.build_golden())
