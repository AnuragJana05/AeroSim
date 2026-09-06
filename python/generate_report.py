"""
AeroSim - Performance Visualization Demo

Runs several parameter sweeps through the C engine
and generates an initial engineering visualization set.
"""

from pathlib import Path

from ctypes_bridge import AeroSimAircraft

from analysis import (
    sweep_mach,
    sweep_altitude,
)

from plotting import (
    plot_drag_vs_mach,
    plot_lift_to_drag_vs_mach,
    plot_rate_of_climb_vs_mach,
    plot_thrust_vs_mach,
    plot_rate_of_climb_vs_altitude,
    plot_thrust_vs_altitude,
    plot_excess_thrust_vs_mach,
)


OUTPUT_DIR = Path("reports")


def main() -> None:

    aircraft = AeroSimAircraft(
        mass_kg=10000.0,
        wing_area_m2=50.0,
        wingspan_m=20.0,
        zero_lift_drag_coefficient=0.02,
        oswald_efficiency=0.8,
        maximum_lift_coefficient=1.5,
        maximum_static_thrust_n=60000.0,
        tsfc_kg_n_s=0.00002,
    )


    print("Running Mach sweep...")

    mach_points = sweep_mach(
        aircraft=aircraft,
        altitude_m=0.0,
        mach_values=[
            0.2,
            0.3,
            0.4,
            0.5,
            0.6,
            0.7,
            0.8,
            0.9,
            1.0,
        ],
        throttle=1.0,
    )


    print("Running altitude sweep...")

    altitude_points = sweep_altitude(
        aircraft=aircraft,
        altitudes_m=[
            0.0,
            1000.0,
            2000.0,
            4000.0,
            6000.0,
            8000.0,
            10000.0,
            12000.0,
        ],
        mach=0.5,
        throttle=1.0,
    )


    print("Generating plots...")


    plot_drag_vs_mach(
        mach_points,
        OUTPUT_DIR / "drag_vs_mach.png",
    )

    plot_lift_to_drag_vs_mach(
        mach_points,
        OUTPUT_DIR / "ld_vs_mach.png",
    )

    plot_rate_of_climb_vs_mach(
        mach_points,
        OUTPUT_DIR / "roc_vs_mach.png",
    )

    plot_thrust_vs_mach(
        mach_points,
        OUTPUT_DIR / "thrust_vs_mach.png",
    )

    plot_excess_thrust_vs_mach(
        mach_points,
        OUTPUT_DIR / "excess_thrust_vs_mach.png",
    )

    plot_rate_of_climb_vs_altitude(
        altitude_points,
        OUTPUT_DIR / "roc_vs_altitude.png",
    )

    plot_thrust_vs_altitude(
        altitude_points,
        OUTPUT_DIR / "thrust_vs_altitude.png",
    )


    print()
    print("============================================")
    print(" AeroSim Visualization Report Generated")
    print("============================================")
    print(f"Output directory: {OUTPUT_DIR.resolve()}")
    print()


if __name__ == "__main__":
    main()