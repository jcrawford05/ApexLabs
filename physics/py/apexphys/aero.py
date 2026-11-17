"""C2. Aerodynamic forces.

Phase P1 uses the aero package's constant lumped coefficients. Wing angles,
ride height and the floor setting start to matter in phase P3.
"""

from apexphys.types import AeroPackage


def drag_force(aero: AeroPackage, rho: float, v_air: float) -> float:
    return 0.5 * rho * v_air * v_air * aero.drag_area_m2


def downforce(aero: AeroPackage, rho: float, v_air: float) -> float:
    return 0.5 * rho * v_air * v_air * aero.downforce_area_m2


def axle_downforce(aero: AeroPackage, rho: float, v_air: float) -> tuple[float, float]:
    """(front, rear) downforce in newtons."""
    total = downforce(aero, rho, v_air)
    return aero.balance_front * total, (1.0 - aero.balance_front) * total
