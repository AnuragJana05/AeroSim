"""
AeroSim - Python ctypes bridge

This module provides a safe Python interface to the AeroSim C physics engine.

IMPORTANT
---------
All physics calculations remain inside C.

Python is responsible only for:

    - loading the shared library
    - converting Python values into ctypes values
    - calling C functions
    - checking C return codes
    - exposing results to higher-level Python code

The C engine remains the source of truth for the physics.
"""

from __future__ import annotations

import ctypes
import platform
from pathlib import Path


# ============================================================================
# Shared-library loading
# ============================================================================

PROJECT_ROOT = Path(__file__).resolve().parent.parent


def _library_filename() -> str:
    """
    Select the appropriate shared-library filename for the host OS.
    """

    system = platform.system()

    if system == "Windows":
        return "aerosim.dll"

    if system == "Linux":
        return "libaerosim.so"

    if system == "Darwin":
        return "libaerosim.dylib"

    raise RuntimeError(
        f"Unsupported operating system: {system}"
    )


LIBRARY_PATH = PROJECT_ROOT / _library_filename()


def _load_library() -> ctypes.CDLL:
    """
    Load the compiled AeroSim shared library.
    """

    if not LIBRARY_PATH.exists():
        raise FileNotFoundError(
            f"AeroSim shared library not found:\n"
            f"{LIBRARY_PATH}\n\n"
            f"Compile the C engine before using the Python bridge."
        )

    try:
        return ctypes.CDLL(str(LIBRARY_PATH))

    except OSError as exc:
        raise RuntimeError(
            f"Failed to load AeroSim shared library:\n"
            f"{LIBRARY_PATH}\n\n"
            f"Original error: {exc}"
        ) from exc


_lib = _load_library()


# ============================================================================
# C structure definitions
# ============================================================================

class AeroSimAircraft(ctypes.Structure):
    """
    Corresponds exactly to:

        typedef struct
        {
            double mass_kg;
            double wing_area_m2;
            double wingspan_m;
            double zero_lift_drag_coefficient;
            double oswald_efficiency;
            double maximum_lift_coefficient;
            double maximum_static_thrust_n;
            double tsfc_kg_n_s;
        } AeroSimAircraft;
    """

    _fields_ = [
        ("mass_kg", ctypes.c_double),
        ("wing_area_m2", ctypes.c_double),
        ("wingspan_m", ctypes.c_double),
        ("zero_lift_drag_coefficient", ctypes.c_double),
        ("oswald_efficiency", ctypes.c_double),
        ("maximum_lift_coefficient", ctypes.c_double),
        ("maximum_static_thrust_n", ctypes.c_double),
        ("tsfc_kg_n_s", ctypes.c_double),
    ]


class AeroSimFlightCondition(ctypes.Structure):
    """
    Corresponds exactly to the C AeroSimFlightCondition structure.
    """

    _fields_ = [
        ("altitude_m", ctypes.c_double),
        ("mach", ctypes.c_double),
        ("throttle", ctypes.c_double),
    ]


class AeroSimPerformanceResult(ctypes.Structure):
    """
    Corresponds exactly to the C AeroSimPerformanceResult structure.
    """

    _fields_ = [
        # Atmosphere
        ("temperature_k", ctypes.c_double),
        ("pressure_pa", ctypes.c_double),
        ("density_kg_m3", ctypes.c_double),
        ("speed_of_sound_m_s", ctypes.c_double),

        # Flight condition
        ("true_airspeed_m_s", ctypes.c_double),
        ("dynamic_pressure_pa", ctypes.c_double),
        ("density_ratio", ctypes.c_double),

        # Aircraft
        ("weight_n", ctypes.c_double),
        ("aspect_ratio", ctypes.c_double),

        # Aerodynamics
        ("lift_coefficient", ctypes.c_double),
        ("induced_drag_factor", ctypes.c_double),
        ("drag_coefficient", ctypes.c_double),
        ("lift_n", ctypes.c_double),
        ("drag_n", ctypes.c_double),
        ("lift_to_drag_ratio", ctypes.c_double),

        # Propulsion
        ("thrust_available_n", ctypes.c_double),
        ("fuel_mass_flow_kg_s", ctypes.c_double),

        # Performance
        ("excess_thrust_n", ctypes.c_double),
        ("excess_power_w", ctypes.c_double),
        ("rate_of_climb_m_s", ctypes.c_double),
    ]


class AeroSimMissionConfig(ctypes.Structure):
    """
    Corresponds to the C AeroSimMissionConfig structure.
    """

    _fields_ = [
        ("aircraft", AeroSimAircraft),
        ("initial_mass_kg", ctypes.c_double),
        ("dry_mass_kg", ctypes.c_double),
        ("altitude_m", ctypes.c_double),
        ("mach", ctypes.c_double),
        ("throttle", ctypes.c_double),
        ("timestep_s", ctypes.c_double),
        ("maximum_time_s", ctypes.c_double),
    ]


class AeroSimMissionSample(ctypes.Structure):
    """
    Corresponds to the C AeroSimMissionSample structure.
    """

    _fields_ = [
        ("time_s", ctypes.c_double),
        ("distance_m", ctypes.c_double),
        ("aircraft_mass_kg", ctypes.c_double),
        ("fuel_remaining_kg", ctypes.c_double),
        ("performance", AeroSimPerformanceResult),
    ]


class AeroSimMissionResult(ctypes.Structure):
    """
    Corresponds to the C AeroSimMissionResult structure.
    """

    _fields_ = [
        ("final_time_s", ctypes.c_double),
        ("final_distance_m", ctypes.c_double),
        ("initial_mass_kg", ctypes.c_double),
        ("final_mass_kg", ctypes.c_double),
        ("initial_fuel_kg", ctypes.c_double),
        ("fuel_burned_kg", ctypes.c_double),
        ("remaining_fuel_kg", ctypes.c_double),
        ("sample_count", ctypes.c_size_t),
    ]


# ============================================================================
# C function declarations
# ============================================================================

# ----------------------------------------------------------------------------
# Atmosphere
# ----------------------------------------------------------------------------

_lib.atmosphere_temperature.argtypes = [
    ctypes.c_double
]

_lib.atmosphere_temperature.restype = ctypes.c_double


_lib.atmosphere_pressure.argtypes = [
    ctypes.c_double
]

_lib.atmosphere_pressure.restype = ctypes.c_double


_lib.atmosphere_density.argtypes = [
    ctypes.c_double
]

_lib.atmosphere_density.restype = ctypes.c_double


_lib.atmosphere_speed_of_sound.argtypes = [
    ctypes.c_double
]

_lib.atmosphere_speed_of_sound.restype = ctypes.c_double


# ----------------------------------------------------------------------------
# Aerodynamics
# ----------------------------------------------------------------------------

_lib.aero_weight_from_mass.argtypes = [
    ctypes.c_double
]

_lib.aero_weight_from_mass.restype = ctypes.c_double


_lib.aero_dynamic_pressure.argtypes = [
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_dynamic_pressure.restype = ctypes.c_double


_lib.aero_lift_coefficient_level_flight.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_lift_coefficient_level_flight.restype = ctypes.c_double


_lib.aero_induced_drag_factor.argtypes = [
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_induced_drag_factor.restype = ctypes.c_double


_lib.aero_drag_coefficient.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_drag_coefficient.restype = ctypes.c_double


_lib.aero_lift.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_lift.restype = ctypes.c_double


_lib.aero_drag.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_drag.restype = ctypes.c_double


_lib.aero_lift_to_drag_ratio.argtypes = [
    ctypes.c_double,
    ctypes.c_double
]

_lib.aero_lift_to_drag_ratio.restype = ctypes.c_double


# ----------------------------------------------------------------------------
# Propulsion
# ----------------------------------------------------------------------------

_lib.propulsion_thrust_available.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.propulsion_thrust_available.restype = ctypes.c_double


_lib.propulsion_fuel_mass_flow.argtypes = [
    ctypes.c_double,
    ctypes.c_double
]

_lib.propulsion_fuel_mass_flow.restype = ctypes.c_double


# ----------------------------------------------------------------------------
# Performance
# ----------------------------------------------------------------------------

_lib.performance_excess_thrust.argtypes = [
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_excess_thrust.restype = ctypes.c_double


_lib.performance_excess_power.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_excess_power.restype = ctypes.c_double


_lib.performance_rate_of_climb.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_rate_of_climb.restype = ctypes.c_double


_lib.performance_stall_speed.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_stall_speed.restype = ctypes.c_double


_lib.performance_jet_range.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_jet_range.restype = ctypes.c_double


_lib.performance_jet_endurance.argtypes = [
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double
]

_lib.performance_jet_endurance.restype = ctypes.c_double


# ----------------------------------------------------------------------------
# Scenario
# ----------------------------------------------------------------------------

_lib.aerosim_calculate_scenario.argtypes = [
    ctypes.POINTER(AeroSimAircraft),
    ctypes.POINTER(AeroSimFlightCondition),
    ctypes.POINTER(AeroSimPerformanceResult)
]

_lib.aerosim_calculate_scenario.restype = ctypes.c_int


# ----------------------------------------------------------------------------
# Mission
# ----------------------------------------------------------------------------

_lib.aerosim_run_mission.argtypes = [
    ctypes.POINTER(AeroSimMissionConfig),
    ctypes.POINTER(AeroSimMissionSample),
    ctypes.c_size_t,
    ctypes.POINTER(AeroSimMissionResult)
]

_lib.aerosim_run_mission.restype = ctypes.c_int


# ============================================================================
# Error handling
# ============================================================================

def _check_finite(value: float, name: str) -> float:
    """
    Reject NaN or infinite values returned by C.
    """

    if not isinstance(value, (int, float)):
        raise TypeError(
            f"{name} must be numeric."
        )

    value = float(value)

    if not ctypes.c_double(value).value == value:
        raise ValueError(
            f"{name} could not be represented as a C double."
        )

    return value


def _check_c_double_result(
    value: float,
    name: str
) -> float:
    """
    Convert and validate a scalar returned by C.
    """

    value = float(value)

    if value != value:
        raise RuntimeError(
            f"AeroSim C engine returned NaN for {name}."
        )

    if value in (float("inf"), float("-inf")):
        raise RuntimeError(
            f"AeroSim C engine returned infinity for {name}."
        )

    return value


def _check_status(status: int) -> None:
    """
    Convert C API error codes into Python exceptions.
    """

    if status == 0:
        return

    if status == -1:
        raise ValueError(
            "AeroSim C engine rejected the input."
        )

    if status == -2:
        raise RuntimeError(
            "AeroSim C engine reported an internal calculation failure."
        )

    if status == -3:
        raise RuntimeError(
            "AeroSim output buffer is too small."
        )

    raise RuntimeError(
        f"AeroSim C engine returned unknown error code {status}."
    )


# ============================================================================
# Safe atmosphere wrappers
# ============================================================================

def temperature_from_altitude(
    altitude_m: float
) -> float:
    """
    Calculate ISA temperature from altitude.

    Physics remains entirely in C.
    """

    altitude_m = _check_finite(
        altitude_m,
        "altitude_m"
    )

    return _check_c_double_result(
        _lib.atmosphere_temperature(altitude_m),
        "temperature"
    )


def pressure_from_altitude(
    altitude_m: float
) -> float:
    """
    Calculate ISA pressure from altitude.
    """

    altitude_m = _check_finite(
        altitude_m,
        "altitude_m"
    )

    return _check_c_double_result(
        _lib.atmosphere_pressure(altitude_m),
        "pressure"
    )


def density_from_altitude(
    altitude_m: float
) -> float:
    """
    Calculate ISA density from altitude.
    """

    altitude_m = _check_finite(
        altitude_m,
        "altitude_m"
    )

    return _check_c_double_result(
        _lib.atmosphere_density(altitude_m),
        "density"
    )


def speed_of_sound_from_temperature(
    temperature_k: float
) -> float:
    """
    Calculate speed of sound from temperature.
    """

    temperature_k = _check_finite(
        temperature_k,
        "temperature_k"
    )

    return _check_c_double_result(
        _lib.atmosphere_speed_of_sound(temperature_k),
        "speed_of_sound"
    )


# ============================================================================
# Safe aerodynamic wrappers
# ============================================================================

def weight_from_mass(
    mass_kg: float
) -> float:
    """
    Convert aircraft mass [kg] to weight [N].
    """

    mass_kg = _check_finite(mass_kg, "mass_kg")

    return _check_c_double_result(
        _lib.aero_weight_from_mass(mass_kg),
        "weight"
    )


def dynamic_pressure(
    density_kg_m3: float,
    velocity_m_s: float
) -> float:
    """
    Calculate dynamic pressure [Pa].
    """

    density_kg_m3 = _check_finite(
        density_kg_m3,
        "density_kg_m3"
    )

    velocity_m_s = _check_finite(
        velocity_m_s,
        "velocity_m_s"
    )

    return _check_c_double_result(
        _lib.aero_dynamic_pressure(
            density_kg_m3,
            velocity_m_s
        ),
        "dynamic_pressure"
    )


def lift_coefficient_level_flight(
    mass_kg: float,
    density_kg_m3: float,
    velocity_m_s: float,
    wing_area_m2: float
) -> float:
    """
    Calculate CL for steady level flight.
    """

    return _check_c_double_result(
        _lib.aero_lift_coefficient_level_flight(
            _check_finite(mass_kg, "mass_kg"),
            _check_finite(density_kg_m3, "density_kg_m3"),
            _check_finite(velocity_m_s, "velocity_m_s"),
            _check_finite(wing_area_m2, "wing_area_m2"),
        ),
        "lift_coefficient"
    )


def induced_drag_factor(
    oswald_efficiency: float,
    aspect_ratio: float
) -> float:
    """
    Calculate induced drag factor k.
    """

    return _check_c_double_result(
        _lib.aero_induced_drag_factor(
            _check_finite(
                oswald_efficiency,
                "oswald_efficiency"
            ),
            _check_finite(
                aspect_ratio,
                "aspect_ratio"
            ),
        ),
        "induced_drag_factor"
    )


def drag_coefficient(
    cd0: float,
    induced_factor: float,
    lift_coefficient_value: float
) -> float:
    """
    Calculate the parabolic drag coefficient.
    """

    return _check_c_double_result(
        _lib.aero_drag_coefficient(
            _check_finite(cd0, "cd0"),
            _check_finite(induced_factor, "induced_factor"),
            _check_finite(
                lift_coefficient_value,
                "lift_coefficient"
            ),
        ),
        "drag_coefficient"
    )


def lift(
    dynamic_pressure_pa: float,
    wing_area_m2: float,
    lift_coefficient_value: float
) -> float:
    """
    Calculate aerodynamic lift [N].
    """

    return _check_c_double_result(
        _lib.aero_lift(
            _check_finite(
                dynamic_pressure_pa,
                "dynamic_pressure_pa"
            ),
            _check_finite(
                wing_area_m2,
                "wing_area_m2"
            ),
            _check_finite(
                lift_coefficient_value,
                "lift_coefficient"
            ),
        ),
        "lift"
    )


def drag(
    dynamic_pressure_pa: float,
    wing_area_m2: float,
    drag_coefficient_value: float
) -> float:
    """
    Calculate aerodynamic drag [N].
    """

    return _check_c_double_result(
        _lib.aero_drag(
            _check_finite(
                dynamic_pressure_pa,
                "dynamic_pressure_pa"
            ),
            _check_finite(
                wing_area_m2,
                "wing_area_m2"
            ),
            _check_finite(
                drag_coefficient_value,
                "drag_coefficient"
            ),
        ),
        "drag"
    )


def lift_to_drag_ratio(
    lift_n: float,
    drag_n: float
) -> float:
    """
    Calculate aerodynamic efficiency L/D.
    """

    return _check_c_double_result(
        _lib.aero_lift_to_drag_ratio(
            _check_finite(lift_n, "lift_n"),
            _check_finite(drag_n, "drag_n"),
        ),
        "lift_to_drag_ratio"
    )


# ============================================================================
# Scenario wrapper
# ============================================================================

def calculate_scenario(
    aircraft: AeroSimAircraft,
    condition: AeroSimFlightCondition
) -> AeroSimPerformanceResult:
    """
    Calculate a complete AeroSim performance snapshot.

    All actual physics calculations happen inside C.
    """

    result = AeroSimPerformanceResult()

    status = _lib.aerosim_calculate_scenario(
        ctypes.byref(aircraft),
        ctypes.byref(condition),
        ctypes.byref(result)
    )

    _check_status(status)

    return result


# ============================================================================
# Mission wrapper
# ============================================================================

def run_mission(
    config: AeroSimMissionConfig
) -> tuple[list[AeroSimMissionSample], AeroSimMissionResult]:
    """
    Run the numerical mission simulation.

    The Python layer only allocates the output buffer and calls C.
    Fuel burn, aerodynamics, propulsion and integration remain in C.
    """

    if config.timestep_s <= 0.0:
        raise ValueError(
            "Mission timestep must be positive."
        )

    if config.maximum_time_s <= 0.0:
        raise ValueError(
            "Maximum mission time must be positive."
        )

    # +1 guarantees enough room for t=0 and the final sample.
    sample_capacity = (
        int(config.maximum_time_s / config.timestep_s) + 2
    )

    samples = (
        AeroSimMissionSample * sample_capacity
    )()

    result = AeroSimMissionResult()

    status = _lib.aerosim_run_mission(
        ctypes.byref(config),
        samples,
        sample_capacity,
        ctypes.byref(result)
    )

    _check_status(status)

    python_samples = [
        samples[index]
        for index in range(result.sample_count)
    ]

    return python_samples, result