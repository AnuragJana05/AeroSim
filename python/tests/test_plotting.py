"""
AeroSim - Phase 9 Visualization Tests

These tests verify that the visualization layer can consume
data produced by the C engine.

The tests use Matplotlib's non-interactive Agg backend so they
can run without opening GUI windows.
"""

import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")


PYTHON_DIR = Path(__file__).resolve().parents[1]

if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))


from ctypes_bridge import AeroSimAircraft
from analysis import (
    sweep_mach,
    sweep_altitude,
)
from mission_analysis import (
    analyze_mission,
)
from plotting import (
    plot_drag_vs_mach,
    plot_lift_to_drag_vs_mach,
    plot_rate_of_climb_vs_mach,
    plot_thrust_vs_mach,
    plot_rate_of_climb_vs_altitude,
    plot_thrust_vs_altitude,
    plot_excess_thrust_vs_mach,
    plot_fuel_remaining,
    plot_mass_vs_time,
    plot_distance_vs_time,
    plot_thrust_drag_vs_time,
)


OUTPUT_DIR = (
    Path(__file__).resolve().parents[1]
    / "plots"
)


def make_aircraft() -> AeroSimAircraft:
    """
    Generic AeroSim test aircraft.
    """

    return AeroSimAircraft(
        mass_kg=10000.0,
        wing_area_m2=50.0,
        wingspan_m=20.0,
        zero_lift_drag_coefficient=0.02,
        oswald_efficiency=0.8,
        maximum_lift_coefficient=1.5,
        maximum_static_thrust_n=60000.0,
        tsfc_kg_n_s=0.00002,
    )


def test_performance_plots() -> None:

    aircraft = make_aircraft()

    points = sweep_mach(
        aircraft=aircraft,
        altitude_m=0.0,
        mach_values=[
            0.3,
            0.4,
            0.5,
            0.6,
            0.7,
        ],
        throttle=1.0,
    )


    plot_drag_vs_mach(
        points,
        OUTPUT_DIR / "drag_vs_mach.png",
    )

    plot_lift_to_drag_vs_mach(
        points,
        OUTPUT_DIR / "ld_vs_mach.png",
    )

    plot_rate_of_climb_vs_mach(
        points,
        OUTPUT_DIR / "roc_vs_mach.png",
    )

    plot_thrust_vs_mach(
        points,
        OUTPUT_DIR / "thrust_vs_mach.png",
    )

    plot_excess_thrust_vs_mach(
        points,
        OUTPUT_DIR / "excess_thrust_vs_mach.png",
    )


    print(
        "PASS: Performance plots generated"
    )


def test_altitude_plots() -> None:

    aircraft = make_aircraft()

    points = sweep_altitude(
        aircraft=aircraft,
        altitudes_m=[
            0.0,
            2000.0,
            4000.0,
            6000.0,
            8000.0,
            10000.0,
        ],
        mach=0.5,
        throttle=1.0,
    )


    plot_rate_of_climb_vs_altitude(
        points,
        OUTPUT_DIR / "roc_vs_altitude.png",
    )

    plot_thrust_vs_altitude(
        points,
        OUTPUT_DIR / "thrust_vs_altitude.png",
    )


    print(
        "PASS: Altitude plots generated"
    )


def test_mission_plots() -> None:

    aircraft = make_aircraft()


    # Initial and dry mass explicitly define fuel quantity.
    from ctypes_bridge import AeroSimMissionConfig


    config = AeroSimMissionConfig(
        aircraft=aircraft,

        initial_mass_kg=10000.0,
        dry_mass_kg=9000.0,

        altitude_m=0.0,
        mach=0.5,
        throttle=0.8,

        timestep_s=1.0,
        maximum_time_s=600.0,
    )


    history, summary = analyze_mission(
        config
    )


    assert len(history.time_s) > 1
    assert summary.final_mass_kg >= 9000.0
    assert summary.final_distance_m > 0.0


    plot_fuel_remaining(
        history,
        OUTPUT_DIR / "fuel_remaining.png",
    )

    plot_mass_vs_time(
        history,
        OUTPUT_DIR / "mass_vs_time.png",
    )

    plot_distance_vs_time(
        history,
        OUTPUT_DIR / "distance_vs_time.png",
    )

    plot_thrust_drag_vs_time(
        history,
        OUTPUT_DIR / "thrust_drag_vs_time.png",
    )


    print(
        "PASS: Mission plots generated"
    )


def test_output_files() -> None:

    expected_files = [
        "drag_vs_mach.png",
        "ld_vs_mach.png",
        "roc_vs_mach.png",
        "thrust_vs_mach.png",
        "excess_thrust_vs_mach.png",
        "roc_vs_altitude.png",
        "thrust_vs_altitude.png",
        "fuel_remaining.png",
        "mass_vs_time.png",
        "distance_vs_time.png",
        "thrust_drag_vs_time.png",
    ]


    for filename in expected_files:

        path = OUTPUT_DIR / filename

        assert path.exists(), (
            f"Expected plot was not created: {path}"
        )

        assert path.stat().st_size > 0, (
            f"Plot file is empty: {path}"
        )


    print(
        "PASS: Plot output files verified"
    )


def main() -> None:

    print("============================================")
    print(" AeroSim - Phase 9 Visualization Tests")
    print("============================================")


    test_performance_plots()
    test_altitude_plots()
    test_mission_plots()
    test_output_files()


    print("\n============================================")
    print(" ALL PHASE 9 TESTS PASSED")
    print("============================================")


if __name__ == "__main__":
    main()