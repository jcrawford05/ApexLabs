"""Physical constants and unit conversions.

The model works in SI internally. This is the only module where psi, km/h,
knots, degrees Celsius, rpm, degrees, millimetres, kilowatts or percent
appear; everything else takes and returns SI values.
"""

import math

G = 9.80665  # gravitational acceleration, m/s^2
PA_PER_PSI = 6894.757
M_S_PER_KNOT = 1852.0 / 3600.0
KELVIN_OFFSET = 273.15
STANDARD_SEA_LEVEL_PRESSURE_PA = 101325.0


def psi_to_pa(psi: float) -> float:
    return psi * PA_PER_PSI


def pa_to_psi(pa: float) -> float:
    return pa / PA_PER_PSI


def gauge_to_abs(p_gauge_pa: float, p_atm_pa: float) -> float:
    return p_gauge_pa + p_atm_pa


def abs_to_gauge(p_abs_pa: float, p_atm_pa: float) -> float:
    return p_abs_pa - p_atm_pa


def kmh_to_ms(kmh: float) -> float:
    return kmh / 3.6


def ms_to_kmh(ms: float) -> float:
    return ms * 3.6


def knots_to_ms(knots: float) -> float:
    return knots * M_S_PER_KNOT


def ms_to_knots(ms: float) -> float:
    return ms / M_S_PER_KNOT


def c_to_k(celsius: float) -> float:
    return celsius + KELVIN_OFFSET


def k_to_c(kelvin: float) -> float:
    return kelvin - KELVIN_OFFSET


def deg_to_rad(deg: float) -> float:
    return math.radians(deg)


def rad_to_deg(rad: float) -> float:
    return math.degrees(rad)


def rpm_to_rad_s(rpm: float) -> float:
    return rpm * 2.0 * math.pi / 60.0


def rad_s_to_rpm(rad_s: float) -> float:
    return rad_s * 60.0 / (2.0 * math.pi)


def mm_to_m(mm: float) -> float:
    return mm / 1000.0


def m_to_mm(m: float) -> float:
    return m * 1000.0


def kw_to_w(kw: float) -> float:
    return kw * 1000.0


def w_to_kw(w: float) -> float:
    return w / 1000.0


def pct_to_frac(pct: float) -> float:
    return pct / 100.0


def frac_to_pct(frac: float) -> float:
    return frac * 100.0
