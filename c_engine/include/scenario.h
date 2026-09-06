#ifndef AEROSIM_SCENARIO_H
#define AEROSIM_SCENARIO_H

/*
 * AeroSim - Aircraft Performance Scenario
 *
 * This module provides a high-level interface over the existing
 * atmosphere, aerodynamic and propulsion modules.
 *
 * The caller provides:
 *
 *   1. Aircraft parameters
 *   2. Flight condition
 *
 * AeroSim then calculates a complete performance snapshot.
 *
 * All quantities use SI units:
 *
 *   altitude       : m
 *   mass           : kg
 *   wing area      : m^2
 *   wingspan       : m
 *   velocity       : m/s
 *   temperature    : K
 *   pressure       : Pa
 *   density        : kg/m^3
 *   thrust         : N
 *   lift/drag      : N
 *   power          : W
 *   fuel flow      : kg/s
 *
 * Dimensionless:
 *
 *   Mach
 *   CL
 *   CD
 *   L/D
 *   throttle
 *   density ratio
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Aircraft configuration.
 *
 * Mass is deliberately stored separately from weight.
 * Weight is calculated as:
 *
 *     W = m*g
 */
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


/*
 * Flight condition.
 *
 * The user supplies altitude and Mach.
 *
 * True airspeed is then calculated from the local speed of sound:
 *
 *     V = M*a
 */
typedef struct
{
    double altitude_m;
    double mach;
    double throttle;

} AeroSimFlightCondition;


/*
 * Complete calculated performance state.
 *
 * This structure is designed to eventually be exposed to Python
 * through a shared library interface.
 */
typedef struct
{
    /* Atmospheric state */
    double temperature_k;
    double pressure_pa;
    double density_kg_m3;
    double speed_of_sound_m_s;

    /* Flight condition */
    double true_airspeed_m_s;
    double dynamic_pressure_pa;
    double density_ratio;

    /* Aircraft state */
    double weight_n;
    double aspect_ratio;

    /* Aerodynamics */
    double lift_coefficient;
    double induced_drag_factor;
    double drag_coefficient;
    double lift_n;
    double drag_n;
    double lift_to_drag_ratio;

    /* Propulsion */
    double thrust_available_n;
    double fuel_mass_flow_kg_s;

    /* Performance */
    double excess_thrust_n;
    double excess_power_w;
    double rate_of_climb_m_s;

} AeroSimPerformanceResult;


/*
 * Calculate a complete aircraft performance snapshot.
 *
 * Returns:
 *
 *     0  -> success
 *    -1  -> invalid aircraft or flight-condition input
 *    -2  -> internal calculation failure
 *
 * The result structure is only valid when the function returns 0.
 */
int aerosim_calculate_scenario(
    const AeroSimAircraft *aircraft,
    const AeroSimFlightCondition *condition,
    AeroSimPerformanceResult *result
);


#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_SCENARIO_H */