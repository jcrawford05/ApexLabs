import pytest

from apexphys.params import load_track
from apexphys.track import discretise
from conftest import SAMPLE_TRACK


@pytest.fixture(scope="module")
def track():
    return load_track(SAMPLE_TRACK)


def test_demo_circuit_at_two_metres(track):
    grid = discretise(track, 2.0)
    assert grid.n_steps == 2000
    assert grid.ds_m == 2.0
    assert grid.length_m == 4000.0


def test_each_segment_gets_its_length_in_steps(track):
    grid = discretise(track, 2.0)
    for index, seg in enumerate(track.segments):
        steps = [i for i in range(grid.n_steps) if grid.step_segment[i] == index]
        assert len(steps) * grid.ds_m == seg.length_m
        assert steps == list(range(steps[0], steps[-1] + 1))
        assert all(grid.step_curvature[i] == seg.curvature_per_m for i in steps)
        assert all(grid.step_sector[i] == seg.sector for i in steps)


def test_step_is_adjusted_so_the_lap_closes(track):
    grid = discretise(track, 3.0)
    assert grid.n_steps == 1333
    assert grid.ds_m == pytest.approx(4000.0 / 1333)
    assert grid.length_m == pytest.approx(4000.0)


def test_node_curvature_takes_the_tighter_neighbour(track):
    grid = discretise(track, 2.0)
    hairpin_entry = (900 + 110) // 2  # node where T1 braking meets the hairpin
    assert grid.step_curvature[hairpin_entry - 1] == 0.0
    assert grid.step_curvature[hairpin_entry] == pytest.approx(1.0 / 25.0)
    assert grid.node_curvature(hairpin_entry) == pytest.approx(1.0 / 25.0)
    assert grid.node_curvature(hairpin_entry - 1) == 0.0
    # Node 0 looks back across the start line to the last step.
    assert grid.node_curvature(0) == max(grid.step_curvature[-1], grid.step_curvature[0])


def test_step_longer_than_the_lap_still_gives_one_step(track):
    assert discretise(track, 1e6).n_steps == 1


def test_non_positive_step_is_rejected(track):
    with pytest.raises(ValueError):
        discretise(track, 0.0)
