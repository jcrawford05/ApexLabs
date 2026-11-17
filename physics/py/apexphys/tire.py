"""C4. Tire grip.

Phase P2 uses a constant friction coefficient per compound and the friction
ellipse. Load sensitivity and the pressure factor arrive in phase P3.
"""

from apexphys.types import CarParams


def grip_coefficient(car: CarParams) -> float:
    """Friction coefficient, the same in both directions for now."""
    return car.tire.mu_ref


def longitudinal_fraction(lateral_use: float) -> float:
    """Share of the grip left for braking or traction (friction ellipse).

    ``lateral_use`` is the lateral force as a fraction of the lateral limit.
    """
    return max(0.0, 1.0 - lateral_use * lateral_use) ** 0.5
