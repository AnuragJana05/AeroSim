"""
AeroSim - ctypes bridge test

This test proves that Python can successfully communicate
with the C physics engine.

No physics equations are implemented here.
"""

from ctypes_bridge import (
    temperature_from_altitude,
    pressure_from_altitude,
    density_from_altitude,
    speed_of_sound_from_temperature,
    dynamic_pressure,
    lift_coefficient_level_flight,
    induced_drag_factor,
    drag_coefficient,
    lift,
    drag,
    lift_to_drag_ratio,
)


def main() -> None:

    print("============================================")
    print(" AeroSim - Python ctypes Bridge Test")
    print("============================================")

    # ------------------------------------------------------------
    # Atmosphere
    # ------------------------------------------------------------

    altitude_m = 0.0

    temperature = temperature_from_altitude(
        altitude_m
    )

    pressure = pressure_from_altitude(
        altitude_m
    )

    density = density_from_altitude(
        altitude_m
    )

    speed_of_sound = speed_of_sound_from_temperature(
        temperature
    )

    print("\nAtmosphere:")

    print(
        f"  Temperature      : {temperature:.4f} K"
    )

    print(
        f"  Pressure         : {pressure:.4f} Pa"
    )

    print(
        f"  Density          : {density:.6f} kg/m^3"
    )

    print(
        f"  Speed of sound   : {speed_of_sound:.4f} m/s"
    )

    # ------------------------------------------------------------
    # Basic aerodynamic calculation
    # ------------------------------------------------------------

    mass_kg = 10000.0
    velocity_m_s = 170.0
    wing_area_m2 = 50.0

    q = dynamic_pressure(
        density,
        velocity_m_s
    )

    cl = lift_coefficient_level_flight(
        mass_kg,
        density,
        velocity_m_s,
        wing_area_m2
    )

    k = induced_drag_factor(
        0.8,
        8.0
    )

    cd = drag_coefficient(
        0.02,
        k,
        cl
    )

    lift_n = lift(
        q,
        wing_area_m2,
        cl
    )

    drag_n = drag(
        q,
        wing_area_m2,
        cd
    )

    ld = lift_to_drag_ratio(
        lift_n,
        drag_n
    )

    print("\nAerodynamics:")

    print(
        f"  Dynamic pressure : {q:.4f} Pa"
    )

    print(
        f"  CL               : {cl:.6f}"
    )

    print(
        f"  Induced factor   : {k:.6f}"
    )

    print(
        f"  CD               : {cd:.6f}"
    )

    print(
        f"  Lift             : {lift_n:.4f} N"
    )

    print(
        f"  Drag             : {drag_n:.4f} N"
    )

    print(
        f"  L/D              : {ld:.6f}"
    )

    # ------------------------------------------------------------
    # Basic sanity checks
    # ------------------------------------------------------------

    assert abs(temperature - 288.15) < 0.01
    assert abs(pressure - 101325.0) < 1.0
    assert abs(density - 1.225) < 0.01

    assert q > 0.0
    assert cl > 0.0
    assert k > 0.0
    assert cd > 0.0
    assert lift_n > 0.0
    assert drag_n > 0.0
    assert ld > 0.0

    print("\n============================================")
    print(" PYTHON → C BRIDGE TEST PASSED")
    print("============================================")


if __name__ == "__main__":
    main()