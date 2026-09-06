#include "scenario.h"

#include "atmosphere.h"
#include "aerodynamics.h"
#include "propulsion.h"
#include "performance.h"

#include <math.h>
#include <stddef.h>

/*
 * ============================================================================
 * AeroSim - Complete Performance Scenario
 * ============================================================================
 *
 * This module connects the independent physics modules.
 *
 *
 *                  Aircraft
 *                     +
 *                Flight Condition
 *                     |
 *                     v
 *                ┌───────────┐
 *                │ Atmosphere│
 *                └─────┬─────┘
 *                      |
 *                T, P, rho, a
 *                      |
 *          ┌───────────┴───────────┐
 *          v                       v
 *     Aerodynamics             Propulsion
 *          |                       |
 *       L, D, CL, CD             T, fuel
 *          |                       |
 *          └───────────┬───────────┘
 *                      v
 *                 Performance
 *                      |
 *                 ROC, excess
 *
 *
 * This is a single steady-flight snapshot.
 *
 * It does NOT simulate time evolution, fuel burn, acceleration,
 * climb trajectory or mission waypoints.
 * ============================================================================
 */


/*
 * Standard sea-level density [kg/m^3].
 *
 * Required to convert local density into the density ratio used
 * by the generic propulsion model.
 */
static const double SEA_LEVEL_DENSITY = 1.225;


/*
 * Internal validation.
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
 * Calculate the complete scenario.
 */
int aerosim_calculate_scenario(
    const AeroSimAircraft *aircraft,
    const AeroSimFlightCondition *condition,
    AeroSimPerformanceResult *result)
{
    double aspect_ratio;
    double temperature_k;
    double pressure_pa;
    double density_kg_m3;
    double speed_of_sound_m_s;
    double true_airspeed_m_s;
    double dynamic_pressure_pa;
    double density_ratio;

    double weight_n;
    double lift_coefficient;
    double induced_drag_factor;
    double drag_coefficient;
    double lift_n;
    double drag_n;
    double lift_to_drag_ratio;

    double thrust_n;
    double fuel_mass_flow_kg_s;

    double excess_thrust_n;
    double excess_power_w;
    double rate_of_climb_m_s;


    /*
     * ------------------------------------------------------------------------
     * Validate pointers
     * ------------------------------------------------------------------------
     */
    if (aircraft == NULL ||
        condition == NULL ||
        result == NULL) {
        return -1;
    }


    /*
     * ------------------------------------------------------------------------
     * Validate aircraft parameters
     * ------------------------------------------------------------------------
     */

    if (!valid_positive_finite(aircraft->mass_kg) ||
        !valid_positive_finite(aircraft->wing_area_m2) ||
        !valid_positive_finite(aircraft->wingspan_m) ||
        !valid_non_negative_finite(
            aircraft->zero_lift_drag_coefficient) ||
        !valid_positive_finite(
            aircraft->oswald_efficiency) ||
        !valid_positive_finite(
            aircraft->maximum_lift_coefficient) ||
        !valid_positive_finite(
            aircraft->maximum_static_thrust_n) ||
        !valid_positive_finite(
            aircraft->tsfc_kg_n_s)) {
        return -1;
    }


    /*
     * ------------------------------------------------------------------------
     * Validate flight condition
     * ------------------------------------------------------------------------
     */

    if (!valid_non_negative_finite(condition->altitude_m) ||
        !valid_non_negative_finite(condition->mach) ||
        !valid_throttle(condition->throttle)) {
        return -1;
    }


    /*
     * ------------------------------------------------------------------------
     * Aircraft geometry
     * ------------------------------------------------------------------------
     *
     * Aspect ratio:
     *
     *     AR = b^2/S
     *
     * b -> wingspan [m]
     * S -> wing area [m^2]
     */
    aspect_ratio =
        (aircraft->wingspan_m * aircraft->wingspan_m)
        / aircraft->wing_area_m2;

    if (!valid_positive_finite(aspect_ratio)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Atmosphere
     * ------------------------------------------------------------------------
     */

    temperature_k =
        atmosphere_temperature(condition->altitude_m);

    pressure_pa =
        atmosphere_pressure(condition->altitude_m);

    density_kg_m3 =
        atmosphere_density(condition->altitude_m);

    if (!isfinite(temperature_k) ||
        !isfinite(pressure_pa) ||
        !isfinite(density_kg_m3)) {
        return -2;
    }


    /*
     * Speed of sound:
     *
     *     a = sqrt(gamma*R*T)
     */
    speed_of_sound_m_s =
        atmosphere_speed_of_sound(temperature_k);

    if (!isfinite(speed_of_sound_m_s)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Flight condition
     * ------------------------------------------------------------------------
     *
     * True airspeed:
     *
     *     V = M*a
     */
    true_airspeed_m_s =
        condition->mach * speed_of_sound_m_s;

    /*
     * Our aerodynamic model does not accept zero velocity because
     * level-flight CL would require division by zero.
     */
    if (!valid_positive_finite(true_airspeed_m_s)) {
        return -1;
    }


    /*
     * Dynamic pressure:
     *
     *     q = 0.5*rho*V^2
     */
    dynamic_pressure_pa =
        aero_dynamic_pressure(
            density_kg_m3,
            true_airspeed_m_s
        );

    if (!isfinite(dynamic_pressure_pa)) {
        return -2;
    }


    /*
     * Density ratio:
     *
     *     sigma = rho / rho_SL
     */
    density_ratio =
        density_kg_m3 / SEA_LEVEL_DENSITY;

    if (!valid_positive_finite(density_ratio)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Aircraft weight
     * ------------------------------------------------------------------------
     *
     *     W = m*g
     *
     * Mass remains in kg.
     * Weight is a force in N.
     */
    weight_n =
        aero_weight_from_mass(
            aircraft->mass_kg
        );

    if (!isfinite(weight_n)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Aerodynamics
     * ------------------------------------------------------------------------
     *
     * Steady level flight:
     *
     *     L = W
     *
     * Therefore:
     *
     *     CL = W/(q*S)
     */
    lift_coefficient =
        aero_lift_coefficient_level_flight(
            aircraft->mass_kg,
            density_kg_m3,
            true_airspeed_m_s,
            aircraft->wing_area_m2
        );

    if (!isfinite(lift_coefficient)) {
        return -2;
    }


    /*
     * Induced drag factor:
     *
     *     k = 1/(pi*e*AR)
     */
    induced_drag_factor =
        aero_induced_drag_factor(
            aircraft->oswald_efficiency,
            aspect_ratio
        );

    if (!isfinite(induced_drag_factor)) {
        return -2;
    }


    /*
     * Parabolic drag polar:
     *
     *     CD = CD0 + k*CL^2
     */
    drag_coefficient =
        aero_drag_coefficient(
            aircraft->zero_lift_drag_coefficient,
            induced_drag_factor,
            lift_coefficient
        );

    if (!isfinite(drag_coefficient)) {
        return -2;
    }


    /*
     * Lift:
     *
     *     L = q*S*CL
     */
    lift_n =
        aero_lift(
            dynamic_pressure_pa,
            aircraft->wing_area_m2,
            lift_coefficient
        );

    /*
     * Drag:
     *
     *     D = q*S*CD
     */
    drag_n =
        aero_drag(
            dynamic_pressure_pa,
            aircraft->wing_area_m2,
            drag_coefficient
        );

    if (!isfinite(lift_n) ||
        !isfinite(drag_n)) {
        return -2;
    }


    /*
     * Aerodynamic efficiency:
     *
     *     L/D
     */
    lift_to_drag_ratio =
        aero_lift_to_drag_ratio(
            lift_n,
            drag_n
        );

    if (!isfinite(lift_to_drag_ratio)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Propulsion
     * ------------------------------------------------------------------------
     *
     *     T = T_SL * throttle * sigma^n * Mach_factor
     */
    thrust_n =
        propulsion_thrust_available(
            aircraft->maximum_static_thrust_n,
            condition->throttle,
            density_ratio,
            condition->mach
        );

    if (!isfinite(thrust_n)) {
        return -2;
    }


    /*
     * Fuel mass flow:
     *
     *     mdot_f = TSFC*T
     */
    fuel_mass_flow_kg_s =
        propulsion_fuel_mass_flow(
            thrust_n,
            aircraft->tsfc_kg_n_s
        );

    if (!isfinite(fuel_mass_flow_kg_s)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Performance
     * ------------------------------------------------------------------------
     */

    /*
     * Excess thrust:
     *
     *     T_excess = T-D
     */
    excess_thrust_n =
        performance_excess_thrust(
            thrust_n,
            drag_n
        );


    /*
     * Excess power:
     *
     *     P_excess = (T-D)V
     */
    excess_power_w =
        performance_excess_power(
            thrust_n,
            drag_n,
            true_airspeed_m_s
        );


    /*
     * Rate of climb:
     *
     *     ROC = P_excess/W
     */
    rate_of_climb_m_s =
        performance_rate_of_climb(
            thrust_n,
            drag_n,
            true_airspeed_m_s,
            aircraft->mass_kg
        );

    if (!isfinite(excess_thrust_n) ||
        !isfinite(excess_power_w) ||
        !isfinite(rate_of_climb_m_s)) {
        return -2;
    }


    /*
     * ------------------------------------------------------------------------
     * Populate output structure
     * ------------------------------------------------------------------------
     */

    result->temperature_k = temperature_k;
    result->pressure_pa = pressure_pa;
    result->density_kg_m3 = density_kg_m3;
    result->speed_of_sound_m_s = speed_of_sound_m_s;

    result->true_airspeed_m_s = true_airspeed_m_s;
    result->dynamic_pressure_pa = dynamic_pressure_pa;
    result->density_ratio = density_ratio;

    result->weight_n = weight_n;
    result->aspect_ratio = aspect_ratio;

    result->lift_coefficient = lift_coefficient;
    result->induced_drag_factor = induced_drag_factor;
    result->drag_coefficient = drag_coefficient;
    result->lift_n = lift_n;
    result->drag_n = drag_n;
    result->lift_to_drag_ratio = lift_to_drag_ratio;

    result->thrust_available_n = thrust_n;
    result->fuel_mass_flow_kg_s = fuel_mass_flow_kg_s;

    result->excess_thrust_n = excess_thrust_n;
    result->excess_power_w = excess_power_w;
    result->rate_of_climb_m_s = rate_of_climb_m_s;

    return 0;
}