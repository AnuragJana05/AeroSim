"""
AeroSim - Python Analysis Layer

Phase 8

This module orchestrates calls to the C physics engine and converts
individual C performance snapshots into Python-friendly datasets.

IMPORTANT
---------
No aircraft physics equations are implemented here.

The C engine remains responsible for:

    - atmosphere
    - aerodynamics
    - propulsion
    - aircraft performance
    - mission integration

Python is responsible for:

    - parameter sweeps
    - organizing results
    - engineering analysis
    - preparing data for visualization
"""

from __future__ import annotations

from dataclasses import dataclass

from ctypes_bridge import (
    AeroSimAircraft,
    AeroSimFlightCondition,
    AeroSimPerformanceResult,
    calculate_scenario,
)


# ============================================================================
# Python result containers
# ============================================================================

@dataclass(frozen=True)
class PerformancePoint:
    """
    Python representation of one C performance snapshot.

    Values remain in SI units.
    """

    altitude_m: float
    mach: float

    temperature_k: float
    pressure_pa: float
    density_kg_m3: float
    speed_of_sound_m_s: float

    true_airspeed_m_s: float
    dynamic_pressure_pa: float

    weight_n: float
    aspect_ratio: float

    lift_coefficient: float
    drag_coefficient: float
    lift_n: float
    drag_n: float
    lift_to_drag_ratio: float

    thrust_available_n: float
    fuel_mass_flow_kg_s: float

    excess_thrust_n: float
    excess_power_w: float
    rate_of_climb_m_s: float


def _convert_result(
    altitude_m: float,
    mach: float,
    result: AeroSimPerformanceResult,
) -> PerformancePoint:
    """
    Convert the ctypes structure returned by C into a Python
    dataclass.

    No physics is performed here.
    """

    return PerformancePoint(
        altitude_m=altitude_m,
        mach=mach,

        temperature_k=result.temperature_k,
        pressure_pa=result.pressure_pa,
        density_kg_m3=result.density_kg_m3,
        speed_of_sound_m_s=result.speed_of_sound_m_s,

        true_airspeed_m_s=result.true_airspeed_m_s,
        dynamic_pressure_pa=result.dynamic_pressure_pa,

        weight_n=result.weight_n,
        aspect_ratio=result.aspect_ratio,

        lift_coefficient=result.lift_coefficient,
        drag_coefficient=result.drag_coefficient,
        lift_n=result.lift_n,
        drag_n=result.drag_n,
        lift_to_drag_ratio=result.lift_to_drag_ratio,

        thrust_available_n=result.thrust_available_n,
        fuel_mass_flow_kg_s=result.fuel_mass_flow_kg_s,

        excess_thrust_n=result.excess_thrust_n,
        excess_power_w=result.excess_power_w,
        rate_of_climb_m_s=result.rate_of_climb_m_s,
    )


# ============================================================================
# Single-point analysis
# ============================================================================

def evaluate_point(
    aircraft: AeroSimAircraft,
    altitude_m: float,
    mach: float,
    throttle: float,
) -> PerformancePoint:
    """
    Evaluate one aircraft flight condition.

    All actual physics calculations happen inside C.
    """

    condition = AeroSimFlightCondition(
        altitude_m=altitude_m,
        mach=mach,
        throttle=throttle,
    )

    result = calculate_scenario(
        aircraft,
        condition,
    )

    return _convert_result(
        altitude_m,
        mach,
        result,
    )


# ============================================================================
# Velocity / Mach sweep
# ============================================================================

def sweep_mach(
    aircraft: AeroSimAircraft,
    altitude_m: float,
    mach_values: list[float],
    throttle: float,
) -> list[PerformancePoint]:
    """
    Evaluate an aircraft over a list of Mach numbers.

    Example use:

        Mach 0.2
        Mach 0.3
        Mach 0.4
        ...
        Mach 1.0

    C performs every individual physics calculation.
    """

    if not mach_values:
        raise ValueError(
            "mach_values must contain at least one value."
        )

    points: list[PerformancePoint] = []

    for mach in mach_values:
        point = evaluate_point(
            aircraft=aircraft,
            altitude_m=altitude_m,
            mach=mach,
            throttle=throttle,
        )

        points.append(point)

    return points


# ============================================================================
# Altitude sweep
# ============================================================================

def sweep_altitude(
    aircraft: AeroSimAircraft,
    altitudes_m: list[float],
    mach: float,
    throttle: float,
) -> list[PerformancePoint]:
    """
    Evaluate an aircraft over multiple altitudes.
    """

    if not altitudes_m:
        raise ValueError(
            "altitudes_m must contain at least one value."
        )

    points: list[PerformancePoint] = []

    for altitude_m in altitudes_m:
        point = evaluate_point(
            aircraft=aircraft,
            altitude_m=altitude_m,
            mach=mach,
            throttle=throttle,
        )

        points.append(point)

    return points


# ============================================================================
# 2D altitude-Mach analysis
# ============================================================================

def sweep_altitude_mach(
    aircraft: AeroSimAircraft,
    altitudes_m: list[float],
    mach_values: list[float],
    throttle: float,
) -> list[PerformancePoint]:
    """
    Evaluate every altitude/Mach combination.

    If there are:

        5 altitudes
        10 Mach values

    this produces:

        5 × 10 = 50 C-engine evaluations.
    """

    if not altitudes_m:
        raise ValueError(
            "altitudes_m must contain at least one value."
        )

    if not mach_values:
        raise ValueError(
            "mach_values must contain at least one value."
        )

    points: list[PerformancePoint] = []

    for altitude_m in altitudes_m:

        for mach in mach_values:

            points.append(
                evaluate_point(
                    aircraft=aircraft,
                    altitude_m=altitude_m,
                    mach=mach,
                    throttle=throttle,
                )
            )

    return points


# ============================================================================
# Finding maximum rate of climb
# ============================================================================

def best_rate_of_climb(
    points: list[PerformancePoint],
) -> PerformancePoint:
    """
    Find the evaluated condition with the highest rate of climb.

    This does NOT calculate rate of climb itself.
    C already calculated it.

    Python only searches the resulting dataset.
    """

    if not points:
        raise ValueError(
            "points must contain at least one result."
        )

    return max(
        points,
        key=lambda point: point.rate_of_climb_m_s,
    )


# ============================================================================
# Finding maximum L/D
# ============================================================================

def best_lift_to_drag_ratio(
    points: list[PerformancePoint],
) -> PerformancePoint:
    """
    Find the condition with maximum aerodynamic efficiency.
    """

    if not points:
        raise ValueError(
            "points must contain at least one result."
        )

    return max(
        points,
        key=lambda point: point.lift_to_drag_ratio,
    )


# ============================================================================
# Finding minimum drag
# ============================================================================

def minimum_drag(
    points: list[PerformancePoint],
) -> PerformancePoint:
    """
    Find the evaluated condition with minimum drag.
    """

    if not points:
        raise ValueError(
            "points must contain at least one result."
        )

    return min(
        points,
        key=lambda point: point.drag_n,
    )


# ============================================================================
# Finding maximum excess thrust
# ============================================================================

def maximum_excess_thrust(
    points: list[PerformancePoint],
) -> PerformancePoint:
    """
    Find the evaluated condition with maximum excess thrust.
    """

    if not points:
        raise ValueError(
            "points must contain at least one result."
        )

    return max(
        points,
        key=lambda point: point.excess_thrust_n,
    )