"""
AeroSim - Mission Analysis

Converts mission samples produced by the C engine into
Python-friendly analysis data.

No mission physics is calculated here.
"""

from __future__ import annotations

from dataclasses import dataclass

from ctypes_bridge import (
    AeroSimMissionConfig,
    AeroSimMissionSample,
    run_mission,
)


@dataclass(frozen=True)
class MissionHistory:
    """
    Time history returned from the C mission simulation.

    All values use SI units.
    """

    time_s: list[float]
    distance_m: list[float]

    mass_kg: list[float]
    fuel_remaining_kg: list[float]

    true_airspeed_m_s: list[float]
    lift_n: list[float]
    drag_n: list[float]
    thrust_n: list[float]

    fuel_mass_flow_kg_s: list[float]
    excess_thrust_n: list[float]
    excess_power_w: list[float]
    rate_of_climb_m_s: list[float]


@dataclass(frozen=True)
class MissionSummary:
    """
    High-level mission summary.
    """

    final_time_s: float
    final_distance_m: float

    initial_mass_kg: float
    final_mass_kg: float

    initial_fuel_kg: float
    fuel_burned_kg: float
    remaining_fuel_kg: float


def analyze_mission(
    config: AeroSimMissionConfig,
) -> tuple[MissionHistory, MissionSummary]:
    """
    Run the C mission simulator and convert its output
    into Python lists.

    Physics remains entirely inside C.
    """

    samples, result = run_mission(config)

    history = _build_history(samples)

    summary = MissionSummary(
        final_time_s=result.final_time_s,
        final_distance_m=result.final_distance_m,

        initial_mass_kg=result.initial_mass_kg,
        final_mass_kg=result.final_mass_kg,

        initial_fuel_kg=result.initial_fuel_kg,
        fuel_burned_kg=result.fuel_burned_kg,
        remaining_fuel_kg=result.remaining_fuel_kg,
    )

    return history, summary


def _build_history(
    samples: list[AeroSimMissionSample],
) -> MissionHistory:
    """
    Convert C mission samples into separate Python series.
    """

    return MissionHistory(
        time_s=[
            sample.time_s
            for sample in samples
        ],

        distance_m=[
            sample.distance_m
            for sample in samples
        ],

        mass_kg=[
            sample.aircraft_mass_kg
            for sample in samples
        ],

        fuel_remaining_kg=[
            sample.fuel_remaining_kg
            for sample in samples
        ],

        true_airspeed_m_s=[
            sample.performance.true_airspeed_m_s
            for sample in samples
        ],

        lift_n=[
            sample.performance.lift_n
            for sample in samples
        ],

        drag_n=[
            sample.performance.drag_n
            for sample in samples
        ],

        thrust_n=[
            sample.performance.thrust_available_n
            for sample in samples
        ],

        fuel_mass_flow_kg_s=[
            sample.performance.fuel_mass_flow_kg_s
            for sample in samples
        ],

        excess_thrust_n=[
            sample.performance.excess_thrust_n
            for sample in samples
        ],

        excess_power_w=[
            sample.performance.excess_power_w
            for sample in samples
        ],

        rate_of_climb_m_s=[
            sample.performance.rate_of_climb_m_s
            for sample in samples
        ],
    )