#ifndef AEROSIM_MISSION_H
#define AEROSIM_MISSION_H

/*
 * AeroSim - Numerical Mission / Fuel-Burn Simulation
 *
 * Phase 6 V1 models a steady cruise mission:
 *
 *     - constant altitude
 *     - constant Mach
 *     - constant throttle
 *     - changing aircraft mass due to fuel burn
 *
 * The aircraft is recalculated at every timestep.
 *
 * All quantities use SI units:
 *
 *     time       : s
 *     distance   : m
 *     mass       : kg
 *     altitude   : m
 *     velocity   : m/s
 *     fuel flow  : kg/s
 *
 * Dimensionless:
 *
 *     Mach
 *     throttle
 */

#include <stddef.h>

#include "scenario.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Mission configuration.
 *
 * The aircraft's mass field is ignored for the initial condition.
 * initial_mass_kg and dry_mass_kg explicitly define the fuel state.
 */
typedef struct
{
    AeroSimAircraft aircraft;

    double initial_mass_kg;
    double dry_mass_kg;

    double altitude_m;
    double mach;
    double throttle;

    double timestep_s;
    double maximum_time_s;

} AeroSimMissionConfig;


/*
 * One recorded simulation sample.
 *
 * This structure is intentionally composed of the scenario result
 * plus mission-specific state.
 */
typedef struct
{
    double time_s;
    double distance_m;

    double aircraft_mass_kg;
    double fuel_remaining_kg;

    AeroSimPerformanceResult performance;

} AeroSimMissionSample;


/*
 * Overall mission result.
 */
typedef struct
{
    double final_time_s;
    double final_distance_m;

    double initial_mass_kg;
    double final_mass_kg;

    double initial_fuel_kg;
    double fuel_burned_kg;
    double remaining_fuel_kg;

    size_t sample_count;

} AeroSimMissionResult;


/*
 * Run a numerical fuel-burn mission.
 *
 * The caller supplies an output buffer containing sample_capacity
 * entries.
 *
 * Return codes:
 *
 *      0  success
 *     -1  invalid input
 *     -2  internal calculation failure
 *     -3  output buffer too small
 *
 * The simulation stops when:
 *
 *     aircraft mass <= dry mass
 *
 * or:
 *
 *     simulation time >= maximum_time_s
 *
 * The caller owns the output buffer.
 */
int aerosim_run_mission(
    const AeroSimMissionConfig *config,
    AeroSimMissionSample *samples,
    size_t sample_capacity,
    AeroSimMissionResult *result
);


#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_MISSION_H */