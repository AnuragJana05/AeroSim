"""
AeroSim - Phase 8 Analysis Tests
"""

import sys
from pathlib import Path

# Allow imports from python/
PYTHON_DIR = Path(__file__).resolve().parents[1]

if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))


from ctypes_bridge import AeroSimAircraft
from analysis import (
    evaluate_point,
    sweep_mach,
    sweep_altitude,
    sweep_altitude_mach,
    best_rate_of_climb,
    best_lift_to_drag_ratio,
    minimum_drag,
    maximum_excess_thrust,
)


def make_aircraft() -> AeroSimAircraft:
    """
    Generic aircraft used by the tests.
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


def test_single_point() -> None:

    aircraft = make_aircraft()

    point = evaluate_point(
        aircraft=aircraft,
        altitude_m=0.0,
        mach=0.5,
        throttle=1.0,
    )

    assert abs(point.temperature_k - 288.15) < 0.1
    assert point.density_kg_m3 > 0.0

    assert point.true_airspeed_m_s > 0.0
    assert point.dynamic_pressure_pa > 0.0

    assert point.lift_n > 0.0
    assert point.drag_n > 0.0
    assert point.thrust_available_n > 0.0

    print("PASS: Single-point analysis")


def test_mach_sweep() -> None:

    aircraft = make_aircraft()

    mach_values = [
        0.3,
        0.4,
        0.5,
        0.6,
        0.7,
    ]

    points = sweep_mach(
        aircraft=aircraft,
        altitude_m=0.0,
        mach_values=mach_values,
        throttle=1.0,
    )

    assert len(points) == len(mach_values)

    for point, expected_mach in zip(
        points,
        mach_values,
    ):
        assert abs(point.mach - expected_mach) < 1e-12

    # At constant altitude, higher Mach means higher TAS.
    for i in range(1, len(points)):
        assert (
            points[i].true_airspeed_m_s
            > points[i - 1].true_airspeed_m_s
        )

    print("PASS: Mach sweep")


def test_altitude_sweep() -> None:

    aircraft = make_aircraft()

    altitudes = [
        0.0,
        2000.0,
        5000.0,
        8000.0,
        10000.0,
    ]

    points = sweep_altitude(
        aircraft=aircraft,
        altitudes_m=altitudes,
        mach=0.5,
        throttle=1.0,
    )

    assert len(points) == len(altitudes)

    for point, expected_altitude in zip(
        points,
        altitudes,
    ):
        assert abs(
            point.altitude_m - expected_altitude
        ) < 1e-12

    # Atmospheric density should decrease with altitude.
    for i in range(1, len(points)):
        assert (
            points[i].density_kg_m3
            < points[i - 1].density_kg_m3
        )

    print("PASS: Altitude sweep")


def test_2d_sweep() -> None:

    aircraft = make_aircraft()

    altitudes = [
        0.0,
        5000.0,
        10000.0,
    ]

    mach_values = [
        0.4,
        0.6,
        0.8,
        1.0,
    ]

    points = sweep_altitude_mach(
        aircraft=aircraft,
        altitudes_m=altitudes,
        mach_values=mach_values,
        throttle=1.0,
    )

    expected_count = (
        len(altitudes)
        * len(mach_values)
    )

    assert len(points) == expected_count

    print("PASS: 2D altitude/Mach sweep")


def test_analysis_searches() -> None:

    aircraft = make_aircraft()

    mach_values = [
        0.3,
        0.4,
        0.5,
        0.6,
        0.7,
    ]

    points = sweep_mach(
        aircraft=aircraft,
        altitude_m=0.0,
        mach_values=mach_values,
        throttle=1.0,
    )

    best_roc = best_rate_of_climb(points)
    best_ld = best_lift_to_drag_ratio(points)
    min_drag = minimum_drag(points)
    max_excess = maximum_excess_thrust(points)

    assert best_roc.rate_of_climb_m_s == max(
        point.rate_of_climb_m_s
        for point in points
    )

    assert best_ld.lift_to_drag_ratio == max(
        point.lift_to_drag_ratio
        for point in points
    )

    assert min_drag.drag_n == min(
        point.drag_n
        for point in points
    )

    assert max_excess.excess_thrust_n == max(
        point.excess_thrust_n
        for point in points
    )

    print("PASS: Analysis search functions")


def main() -> None:

    print("============================================")
    print(" AeroSim - Phase 8 Analysis Tests")
    print("============================================")

    test_single_point()
    test_mach_sweep()
    test_altitude_sweep()
    test_2d_sweep()
    test_analysis_searches()

    print("\n============================================")
    print(" ALL PHASE 8 TESTS PASSED")
    print("============================================")


if __name__ == "__main__":
    main()