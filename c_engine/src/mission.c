#include "mission.h"

#include "scenario.h"

#include <math.h>
#include <stddef.h>


/*
 * ============================================================================
 * AeroSim - Phase 6 Numerical Mission Simulation
 * ============================================================================
 *
 * V1 mission model:
 *
 *     altitude = constant
 *     Mach     = constant
 *     throttle = constant
 *
 * The aircraft mass changes because fuel is consumed:
 *
 *     dm/dt = -mdot_f
 *
 * Using an explicit Euler timestep:
 *
 *     m_new = m_old - mdot_f * dt
 *
 * Distance is integrated using:
 *
 *     dR/dt = V
 *
 * giving:
 *
 *     R_new = R_old + V*dt
 *
 *
 * At every timestep the complete aircraft scenario is recalculated.
 *
 * Therefore changing mass automatically changes:
 *
 *     W
 *     CL
 *     CD
 *     Lift
 *     Drag
 *     Excess thrust
 *     Excess power
 *     Rate of climb
 *     Fuel flow
 *
 *
 * ASSUMPTIONS
 * -----------
 *
 * - Steady level flight.
 * - Constant altitude.
 * - Constant Mach number.
 * - Constant throttle.
 * - Constant TSFC from the propulsion model.
 * - No acceleration.
 * - No climb or descent.
 * - No reserve fuel policy.
 * - No wind.
 * - No gravitational trajectory effects.
 * - Explicit Euler integration.
 *
 *
 * This is a numerical fuel-burn simulation, not a full 6-DOF
 * aircraft simulation.
 * ============================================================================
 */


/*
 * Internal validation helpers.
 */
static int valid_positive_finite(double value)
{
    return isfinite(value) && value > 0.0;
}


static int valid_non_negative_finite(double value)
{
    return isfinite(value) && value >= 0.0;
}


static int valid_throttle(double throttle)
{
    return isfinite(throttle) &&
           throttle >= 0.0 &&
           throttle <= 1.0;
}


/*
 * Validate the complete mission configuration.
 */
static int valid_mission_config(
    const AeroSimMissionConfig *config)
{
    if (config == NULL) {
        return 0;
    }

    /*
     * Aircraft parameters.
     */
    if (!valid_positive_finite(config->aircraft.wing_area_m2) ||
        !valid_positive_finite(config->aircraft.wingspan_m) ||
        !valid_non_negative_finite(
            config->aircraft.zero_lift_drag_coefficient) ||
        !valid_positive_finite(
            config->aircraft.oswald_efficiency) ||
        !valid_positive_finite(
            config->aircraft.maximum_lift_coefficient) ||
        !valid_positive_finite(
            config->aircraft.maximum_static_thrust_n) ||
        !valid_positive_finite(
            config->aircraft.tsfc_kg_n_s)) {
        return 0;
    }

    /*
     * Explicit fuel-state definition.
     */
    if (!valid_positive_finite(config->initial_mass_kg) ||
        !valid_positive_finite(config->dry_mass_kg) ||
        config->dry_mass_kg >= config->initial_mass_kg) {
        return 0;
    }

    /*
     * Flight condition.
     */
    if (!valid_non_negative_finite(config->altitude_m) ||
        !valid_positive_finite(config->mach) ||
        !valid_throttle(config->throttle)) {
        return 0;
    }

    /*
     * Numerical integration settings.
     */
    if (!valid_positive_finite(config->timestep_s) ||
        !valid_positive_finite(config->maximum_time_s)) {
        return 0;
    }

    return 1;
}


/*
 * Store one mission sample.
 */
static void store_sample(
    AeroSimMissionSample *sample,
    double time_s,
    double distance_m,
    double aircraft_mass_kg,
    double dry_mass_kg,
    const AeroSimPerformanceResult *performance)
{
    sample->time_s = time_s;
    sample->distance_m = distance_m;

    sample->aircraft_mass_kg = aircraft_mass_kg;

    /*
     * Fuel remaining:
     *
     *     mfuel = mtotal - mdry
     */
    sample->fuel_remaining_kg =
        aircraft_mass_kg - dry_mass_kg;

    sample->performance = *performance;
}


/*
 * ============================================================================
 * Public mission simulation
 * ============================================================================
 */
int aerosim_run_mission(
    const AeroSimMissionConfig *config,
    AeroSimMissionSample *samples,
    size_t sample_capacity,
    AeroSimMissionResult *result)
{
    AeroSimAircraft aircraft;
    AeroSimFlightCondition condition;

    double current_time_s;
    double current_distance_m;
    double current_mass_kg;

    double initial_fuel_kg;

    size_t sample_count;


    /*
     * Validate pointers.
     */
    if (config == NULL ||
        samples == NULL ||
        result == NULL) {
        return -1;
    }


    /*
     * Validate mission configuration.
     */
    if (!valid_mission_config(config)) {
        return -1;
    }


    /*
     * At least one sample is required for the initial state.
     */
    if (sample_capacity == 0U) {
        return -3;
    }


    /*
     * The mission starts with:
     *
     *     m = initial_mass
     *
     * Fuel mass is:
     *
     *     mfuel = initial_mass - dry_mass
     */
    current_mass_kg =
        config->initial_mass_kg;

    initial_fuel_kg =
        config->initial_mass_kg
        - config->dry_mass_kg;


    current_time_s = 0.0;
    current_distance_m = 0.0;
    sample_count = 0U;


    /*
     * Aircraft definition.
     *
     * The mass in this structure is overwritten at every timestep.
     */
    aircraft = config->aircraft;


    /*
     * Fixed flight condition for Phase 6 V1.
     */
    condition.altitude_m =
        config->altitude_m;

    condition.mach =
        config->mach;

    condition.throttle =
        config->throttle;


    /*
     * ------------------------------------------------------------------------
     * Main numerical integration loop
     * ------------------------------------------------------------------------
     */
    while (current_time_s <= config->maximum_time_s) {

        AeroSimPerformanceResult performance;
        int scenario_status;


        /*
         * Make the current mass the aircraft mass used by the
         * performance engine.
         */
        aircraft.mass_kg =
            current_mass_kg;


        /*
         * Calculate the complete aircraft state at this timestep.
         */
        scenario_status =
            aerosim_calculate_scenario(
                &aircraft,
                &condition,
                &performance
            );


        if (scenario_status != 0) {
            return -2;
        }


        /*
         * Ensure there is room for the current sample.
         */
        if (sample_count >= sample_capacity) {
            return -3;
        }


        /*
         * Store the current state BEFORE fuel burn.
         */
        store_sample(
            &samples[sample_count],
            current_time_s,
            current_distance_m,
            current_mass_kg,
            config->dry_mass_kg,
            &performance
        );


        ++sample_count;


        /*
         * Stop if the maximum mission time has been reached.
         */
        if (current_time_s >= config->maximum_time_s) {
            break;
        }


        /*
         * Fuel consumption:
         *
         *     dm/dt = -mdot_f
         *
         * Euler integration:
         *
         *     m_new = m_old - mdot_f*dt
         */
        {
            double fuel_burn_kg;

            fuel_burn_kg =
                performance.fuel_mass_flow_kg_s
                * config->timestep_s;


            /*
             * Prevent the timestep from burning through the dry mass.
             *
             * This is important because the final timestep may contain
             * less fuel than mdot*dt.
             */
            if (fuel_burn_kg
                >= current_mass_kg - config->dry_mass_kg) {

                /*
                 * The remaining fuel is consumed.
                 */
                double remaining_fuel_kg =
                    current_mass_kg
                    - config->dry_mass_kg;

                /*
                 * Integrate only the time required to consume the
                 * remaining fuel.
                 */
                if (performance.fuel_mass_flow_kg_s > 0.0) {

                    double final_dt_s =
                        remaining_fuel_kg
                        / performance.fuel_mass_flow_kg_s;

                    if (final_dt_s > 0.0) {
                        current_distance_m +=
                            performance.true_airspeed_m_s
                            * final_dt_s;

                        current_time_s +=
                            final_dt_s;
                    }
                }

                current_mass_kg =
                    config->dry_mass_kg;

                /*
                 * The next loop iteration records the dry-aircraft
                 * state before terminating.
                 */
                break;
            }


            /*
             * Normal Euler mass update.
             */
            current_mass_kg -= fuel_burn_kg;


            /*
             * Distance integration:
             *
             *     dR = V*dt
             */
            current_distance_m +=
                performance.true_airspeed_m_s
                * config->timestep_s;


            /*
             * Time integration.
             */
            current_time_s +=
                config->timestep_s;
        }


        /*
         * Floating-point protection.
         *
         * The physical constraint is:
         *
         *     mass >= dry_mass
         */
        if (current_mass_kg <
            config->dry_mass_kg) {

            current_mass_kg =
                config->dry_mass_kg;
        }


        /*
        * Stop the simulation when the aircraft reaches dry mass.
        *
        * The current sample already represents the state before this
        * timestep's fuel burn. If the remaining fuel was consumed during
        * this timestep, the dry-mass state was handled explicitly above.
        *
        * There is therefore no reason to continue integrating once
        * the aircraft has no usable fuel remaining.
        */
        if (current_mass_kg <=
            config->dry_mass_kg) {

            current_mass_kg =
                config->dry_mass_kg;

            break;
        }
    }


    /*
     * ------------------------------------------------------------------------
     * Final result
     * ------------------------------------------------------------------------
     */
    result->final_time_s =
        current_time_s;

    result->final_distance_m =
        current_distance_m;

    result->initial_mass_kg =
        config->initial_mass_kg;

    result->final_mass_kg =
        current_mass_kg;

    result->initial_fuel_kg =
        initial_fuel_kg;

    result->fuel_burned_kg =
        config->initial_mass_kg
        - current_mass_kg;

    result->remaining_fuel_kg =
        current_mass_kg
        - config->dry_mass_kg;

    result->sample_count =
        sample_count;


    return 0;
}