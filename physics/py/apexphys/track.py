"""Track discretisation: the segment list becomes equal steps of length ds."""

from dataclasses import dataclass

from apexphys.types import TrackModel


@dataclass(frozen=True)
class TrackGrid:
    """A lap cut into ``n_steps`` equal steps. Step i runs from node i to node i + 1.

    The lap is closed: node ``n_steps`` is node 0.
    """

    ds_m: float
    step_curvature: tuple[float, ...]  # 1/m, 0 on straights and braking zones
    step_segment: tuple[int, ...]  # index into TrackModel.segments
    step_sector: tuple[int, ...]

    @property
    def n_steps(self) -> int:
        return len(self.step_curvature)

    @property
    def length_m(self) -> float:
        return self.ds_m * self.n_steps

    def node_curvature(self, node: int) -> float:
        """The tighter of the two steps that meet at ``node``."""
        n = self.n_steps
        return max(self.step_curvature[(node - 1) % n], self.step_curvature[node % n])


def discretise(track: TrackModel, ds_m: float) -> TrackGrid:
    """Cut the lap into steps as close to ``ds_m`` as divides the lap length exactly.

    Each step takes the curvature, segment and sector of the segment its
    midpoint lies in, so segment boundaries land on nodes whenever the segment
    lengths are multiples of the step.
    """
    if ds_m <= 0.0:
        raise ValueError("ds_m must be positive")
    length = track.length_m
    n_steps = max(1, round(length / ds_m))
    ds = length / n_steps

    curvature, segment, sector = [], [], []
    index, segment_end = 0, track.segments[0].length_m
    for step in range(n_steps):
        midpoint = (step + 0.5) * ds
        while midpoint >= segment_end and index < len(track.segments) - 1:
            index += 1
            segment_end += track.segments[index].length_m
        seg = track.segments[index]
        curvature.append(seg.curvature_per_m)
        segment.append(index)
        sector.append(seg.sector)
    return TrackGrid(ds, tuple(curvature), tuple(segment), tuple(sector))
