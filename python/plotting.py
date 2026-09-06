"""
AeroSim - Visualization Layer

Phase 9

This module visualizes results produced by the C physics engine.

IMPORTANT
---------
No aircraft physics calculations are performed here.

Python is responsible for:

    - selecting data to plot
    - creating figures
    - labeling axes
    - displaying/saving plots

The C engine remains responsible for all physics.
"""

from __future__ import annotations

from pathlib import Path
from typing import Sequence

import matplotlib.pyplot as plt

from analysis import PerformancePoint
from mission_analysis import MissionHistory


# ============================================================================
# Utility
# ============================================================================

def _validate_points(
    points: Sequence[PerformancePoint],
) -> None:
    """Ensure a performance dataset is available."""

    if not points:
        raise ValueError(
            "At least one performance point is required."
        )


def _validate_mission(
    history: MissionHistory,
) -> None:
    """Ensure a mission history contains data."""

    if not history.time_s:
        raise ValueError(
            "Mission history contains no samples."
        )


def _save_or_show(
    figure: plt.Figure,
    save_path: str | Path | None,
) -> None:
    """
    Either save the figure or display it.

    Closing the figure after saving prevents a large parameter sweep
    from leaving many open figures in memory.
    """

    if save_path is not None:

        path = Path(save_path)

        path.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        figure.savefig(
            path,
            dpi=150,
            bbox_inches="tight",
        )

        plt.close(figure)

    else:
        plt.show()


# ============================================================================
# Performance plots
# ============================================================================

def plot_drag_vs_mach(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot aerodynamic drag against Mach number.
    """

    _validate_points(points)

    mach = [
        point.mach
        for point in points
    ]

    drag = [
        point.drag_n
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        mach,
        drag,
        marker="o",
    )

    axis.set_title(
        "Drag vs Mach"
    )

    axis.set_xlabel(
        "Mach"
    )

    axis.set_ylabel(
        "Drag [N]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_lift_to_drag_vs_mach(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot aerodynamic efficiency L/D against Mach number.
    """

    _validate_points(points)

    mach = [
        point.mach
        for point in points
    ]

    lift_to_drag = [
        point.lift_to_drag_ratio
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        mach,
        lift_to_drag,
        marker="o",
    )

    axis.set_title(
        "Lift-to-Drag Ratio vs Mach"
    )

    axis.set_xlabel(
        "Mach"
    )

    axis.set_ylabel(
        "L/D [-]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_rate_of_climb_vs_mach(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot rate of climb against Mach number.
    """

    _validate_points(points)

    mach = [
        point.mach
        for point in points
    ]

    rate_of_climb = [
        point.rate_of_climb_m_s
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        mach,
        rate_of_climb,
        marker="o",
    )

    axis.set_title(
        "Rate of Climb vs Mach"
    )

    axis.set_xlabel(
        "Mach"
    )

    axis.set_ylabel(
        "Rate of Climb [m/s]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_thrust_vs_mach(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot available thrust against Mach number.
    """

    _validate_points(points)

    mach = [
        point.mach
        for point in points
    ]

    thrust = [
        point.thrust_available_n
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        mach,
        thrust,
        marker="o",
    )

    axis.set_title(
        "Available Thrust vs Mach"
    )

    axis.set_xlabel(
        "Mach"
    )

    axis.set_ylabel(
        "Thrust [N]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


# ============================================================================
# Altitude plots
# ============================================================================

def plot_rate_of_climb_vs_altitude(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot rate of climb against altitude.
    """

    _validate_points(points)

    altitude_km = [
        point.altitude_m / 1000.0
        for point in points
    ]

    rate_of_climb = [
        point.rate_of_climb_m_s
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        altitude_km,
        rate_of_climb,
        marker="o",
    )

    axis.set_title(
        "Rate of Climb vs Altitude"
    )

    axis.set_xlabel(
        "Altitude [km]"
    )

    axis.set_ylabel(
        "Rate of Climb [m/s]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_thrust_vs_altitude(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot available thrust against altitude.
    """

    _validate_points(points)

    altitude_km = [
        point.altitude_m / 1000.0
        for point in points
    ]

    thrust = [
        point.thrust_available_n
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        altitude_km,
        thrust,
        marker="o",
    )

    axis.set_title(
        "Available Thrust vs Altitude"
    )

    axis.set_xlabel(
        "Altitude [km]"
    )

    axis.set_ylabel(
        "Thrust [N]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


# ============================================================================
# Mission plots
# ============================================================================

def plot_fuel_remaining(
    history: MissionHistory,
    save_path: str | Path | None = None,
) -> None:
    """
    Plot remaining fuel against mission time.
    """

    _validate_mission(history)

    time_minutes = [
        time / 60.0
        for time in history.time_s
    ]

    figure, axis = plt.subplots()

    axis.plot(
        time_minutes,
        history.fuel_remaining_kg,
    )

    axis.set_title(
        "Fuel Remaining vs Time"
    )

    axis.set_xlabel(
        "Time [min]"
    )

    axis.set_ylabel(
        "Fuel Remaining [kg]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_mass_vs_time(
    history: MissionHistory,
    save_path: str | Path | None = None,
) -> None:
    """
    Plot aircraft mass against mission time.
    """

    _validate_mission(history)

    time_minutes = [
        time / 60.0
        for time in history.time_s
    ]

    figure, axis = plt.subplots()

    axis.plot(
        time_minutes,
        history.mass_kg,
    )

    axis.set_title(
        "Aircraft Mass vs Time"
    )

    axis.set_xlabel(
        "Time [min]"
    )

    axis.set_ylabel(
        "Aircraft Mass [kg]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_distance_vs_time(
    history: MissionHistory,
    save_path: str | Path | None = None,
) -> None:
    """
    Plot accumulated distance against mission time.
    """

    _validate_mission(history)

    time_minutes = [
        time / 60.0
        for time in history.time_s
    ]

    distance_km = [
        distance / 1000.0
        for distance in history.distance_m
    ]

    figure, axis = plt.subplots()

    axis.plot(
        time_minutes,
        distance_km,
    )

    axis.set_title(
        "Distance vs Time"
    )

    axis.set_xlabel(
        "Time [min]"
    )

    axis.set_ylabel(
        "Distance [km]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


def plot_thrust_drag_vs_time(
    history: MissionHistory,
    save_path: str | Path | None = None,
) -> None:
    """
    Compare available thrust and aerodynamic drag throughout
    the mission.
    """

    _validate_mission(history)

    time_minutes = [
        time / 60.0
        for time in history.time_s
    ]

    figure, axis = plt.subplots()

    axis.plot(
        time_minutes,
        history.thrust_n,
        label="Thrust",
    )

    axis.plot(
        time_minutes,
        history.drag_n,
        label="Drag",
    )

    axis.set_title(
        "Thrust and Drag vs Time"
    )

    axis.set_xlabel(
        "Time [min]"
    )

    axis.set_ylabel(
        "Force [N]"
    )

    axis.legend()

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )


# ============================================================================
# Performance envelope
# ============================================================================

def plot_excess_thrust_vs_mach(
    points: Sequence[PerformancePoint],
    save_path: str | Path | None = None,
) -> None:
    """
    Plot excess thrust against Mach number.

    Positive excess thrust indicates available thrust exceeds
    aerodynamic drag for the evaluated condition.
    """

    _validate_points(points)

    mach = [
        point.mach
        for point in points
    ]

    excess_thrust = [
        point.excess_thrust_n
        for point in points
    ]

    figure, axis = plt.subplots()

    axis.plot(
        mach,
        excess_thrust,
        marker="o",
    )

    axis.axhline(
        0.0,
        linewidth=1.0,
    )

    axis.set_title(
        "Excess Thrust vs Mach"
    )

    axis.set_xlabel(
        "Mach"
    )

    axis.set_ylabel(
        "Excess Thrust [N]"
    )

    axis.grid(True)

    _save_or_show(
        figure,
        save_path,
    )