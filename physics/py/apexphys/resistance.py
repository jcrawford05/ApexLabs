"""C6. Rolling resistance. Grade is zero until tracks carry elevation."""

from apexphys.types import Calibration


def rolling_resistance_coeff(cal: Calibration, v: float) -> float:
    return cal.c_rr0 + cal.c_rr2_s2_per_m2 * v * v


def rolling_resistance_force(cal: Calibration, v: float, normal_load_n: float) -> float:
    """``normal_load_n`` is the total tire load: weight plus downforce."""
    return rolling_resistance_coeff(cal, v) * normal_load_n
