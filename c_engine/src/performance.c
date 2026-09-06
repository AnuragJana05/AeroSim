#include "performance.h"

#include "aerodynamics.h"
#include "propulsion.h"

#include <math.h>


/*
 * ============================================================================
 * AeroSim - Aircraft Performance Model
 * ============================================================================
 *
 * This module combines the independent AeroSim physics modules:
 *
 *     Atmosphere
 *          |
 *          v
 *     Aerodynamics <----+
 *          |            |
 *          v            |
 *        Drag           |
 *                       |
 *     Propulsion ------+
 *          |
 *          v
 *       Thrust
 *
 * and calculates aircraft-level performance.
 *
 *
 * V1 PERFORMANCE MODEL
 * --------------------
 *
 * Excess thrust:
 *
 *     T_excess = T - D
 *
 * Excess power:
 *
 *     P_excess = (T-D)V
 *
 * Rate of climb:
 *
 *     ROC = P_excess/W
 *
 * Stall speed:
 *
 *     Vstall = sqrt(2W/(rho*S*CLmax))
 *
 *
 * Maximum level-flight speed:
 *
 *     T_available(V) - D(V) = 0
 *
 * This is solved numerically rather than analytically.
 *
 *
 * Jet range:
 *
 *     R = V/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 * Jet endurance:
 *
 *     E = 1/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 *
 * IMPORTANT:
 *
 * Range and endurance here are analytical Breguet estimates.
 * They are NOT numerical mission simulations.
 *
 * A future mission simulator can instead integrate the changing
 * aircraft mass over time and update the atmosphere, aerodynamics,
 * engine thrust and TSFC continuously.
 * ============================================================================
 */


/* Standard gravitational acceleration [m/s^2]. */
static const double GRAVITY = 9.80665;


/*
 * Standard sea-level air density [kg/m^3].
 *
 * Used to convert the supplied local density into the density ratio
 * required by the generic propulsion model:
 *
 *     sigma = rho / rho_SL
 */
static const double SEA_LEVEL_DENSITY = 1.225;


/*
 * Numerical tolerance used when solving:
 *
 *     T - D = 0
 *
 * Units: N
 */
static const double THRUST_ROOT_TOLERANCE_N = 1.0;


/*
 * Maximum number of iterations allowed during bisection.
 *
 * This prevents an unexpected numerical condition from causing
 * an infinite loop.
 */
static const int MAX_BISECTION_ITERATIONS = 100;


/*
 * ============================================================================
 * Internal validation helpers
 * ============================================================================
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
 * ============================================================================
 * Internal maximum-speed helper
 * ============================================================================
 *
 * Calculates:
 *
 *     F(V) = T_available - D
 *
 * A positive result means the aircraft has more thrust available
 * than the aerodynamic drag at that velocity.
 *
 * A zero result represents steady level-flight equilibrium.
 */
static double level_flight_thrust_margin(
    double velocity_m_s,
    double mass_kg,
    double density_kg_m3,
    double speed_of_sound_m_s,
    double wing_area_m2,
    double zero_lift_drag_coefficient,
    double oswald_efficiency,
    double aspect_ratio,
    double maximum_static_thrust_n,
    double throttle)
{
    double dynamic_pressure_pa;
    double weight_n;
    double lift_coefficient;
    double induced_drag_factor;
    double drag_coefficient;
    double drag_n;
    double mach;
    double density_ratio;
    double thrust_n;

    if (!valid_positive_finite(velocity_m_s) ||
        !valid_positive_finite(mass_kg) ||
        !valid_positive_finite(density_kg_m3) ||
        !valid_positive_finite(speed_of_sound_m_s) ||
        !valid_positive_finite(wing_area_m2) ||
        !valid_non_negative_finite(zero_lift_drag_coefficient) ||
        !valid_positive_finite(oswald_efficiency) ||
        !valid_positive_finite(aspect_ratio) ||
        !valid_positive_finite(maximum_static_thrust_n) ||
        !valid_throttle(throttle)) {
        return NAN;
    }

    /*
     * Aerodynamic dynamic pressure:
     *
     *     q = 0.5*rho*V^2
     */
    dynamic_pressure_pa =
        aero_dynamic_pressure(
            density_kg_m3,
            velocity_m_s
        );

    /*
     * Aircraft weight:
     *
     *     W = m*g
     */
    weight_n =
        aero_weight_from_mass(mass_kg);

    if (!isfinite(dynamic_pressure_pa) ||
        !isfinite(weight_n) ||
        dynamic_pressure_pa <= 0.0) {
        return NAN;
    }

    /*
     * Steady level flight requires:
     *
     *     L = W
     *
     * Therefore:
     *
     *     CL = W/(q*S)
     */
    lift_coefficient =
        aero_lift_coefficient_level_flight(
            mass_kg,
            density_kg_m3,
            velocity_m_s,
            wing_area_m2
        );

    /*
     * Induced drag factor:
     *
     *     k = 1/(pi*e*AR)
     */
    induced_drag_factor =
        aero_induced_drag_factor(
            oswald_efficiency,
            aspect_ratio
        );

    /*
     * Parabolic drag polar:
     *
     *     CD = CD0 + k*CL^2
     */
    drag_coefficient =
        aero_drag_coefficient(
            zero_lift_drag_coefficient,
            induced_drag_factor,
            lift_coefficient
        );

    /*
     * Aerodynamic drag:
     *
     *     D = q*S*CD
     */
    drag_n =
        aero_drag(
            dynamic_pressure_pa,
            wing_area_m2,
            drag_coefficient
        );

    /*
     * Convert velocity to Mach:
     *
     *     M = V/a
     */
    mach =
        velocity_m_s / speed_of_sound_m_s;

    /*
     * The propulsion model expects density ratio:
     *
     *     sigma = rho/rho_SL
     */
    density_ratio =
        density_kg_m3 / SEA_LEVEL_DENSITY;

    /*
     * Calculate available engine thrust.
     */
    thrust_n =
        propulsion_thrust_available(
            maximum_static_thrust_n,
            throttle,
            density_ratio,
            mach
        );

    if (!isfinite(drag_n) ||
        !isfinite(thrust_n)) {
        return NAN;
    }

    /*
     * Positive:
     *
     *     T > D
     *
     * means the aircraft can sustain the specified level-flight
     * condition and has excess thrust.
     *
     * Negative:
     *
     *     T < D
     *
     * means the condition cannot be sustained.
     */
    return thrust_n - drag_n;
}


/*
 * ============================================================================
 * Public performance functions
 * ============================================================================
 */


/*
 * Excess thrust:
 *
 *     T_excess = T_available - D
 */
double performance_excess_thrust(
    double thrust_n,
    double drag_n)
{
    if (!valid_non_negative_finite(thrust_n) ||
        !valid_non_negative_finite(drag_n)) {
        return NAN;
    }

    return thrust_n - drag_n;
}


/*
 * Excess power:
 *
 *     P_excess = (T-D)V
 *
 * Units:
 *
 *     N * m/s = W
 */
double performance_excess_power(
    double thrust_n,
    double drag_n,
    double velocity_m_s)
{
    double excess_thrust_n;

    if (!valid_non_negative_finite(thrust_n) ||
        !valid_non_negative_finite(drag_n) ||
        !valid_positive_finite(velocity_m_s)) {
        return NAN;
    }

    excess_thrust_n =
        performance_excess_thrust(
            thrust_n,
            drag_n
        );

    if (!isfinite(excess_thrust_n)) {
        return NAN;
    }

    return excess_thrust_n * velocity_m_s;
}


/*
 * Rate of climb:
 *
 *     ROC = P_excess/W
 *
 * with:
 *
 *     P_excess = (T-D)V
 *
 *     W = m*g
 *
 * Therefore:
 *
 *     ROC = (T-D)V/(m*g)
 *
 * The result is m/s.
 */
double performance_rate_of_climb(
    double thrust_n,
    double drag_n,
    double velocity_m_s,
    double mass_kg)
{
    double excess_power_w;
    double weight_n;

    if (!valid_non_negative_finite(thrust_n) ||
        !valid_non_negative_finite(drag_n) ||
        !valid_positive_finite(velocity_m_s) ||
        !valid_positive_finite(mass_kg)) {
        return NAN;
    }

    excess_power_w =
        performance_excess_power(
            thrust_n,
            drag_n,
            velocity_m_s
        );

    weight_n =
        aero_weight_from_mass(mass_kg);

    if (!isfinite(excess_power_w) ||
        !isfinite(weight_n) ||
        weight_n <= 0.0) {
        return NAN;
    }

    return excess_power_w / weight_n;
}


/*
 * 1-g stall speed:
 *
 *     L = W
 *
 *     L = 0.5*rho*V^2*S*CLmax
 *
 * therefore:
 *
 *     Vstall = sqrt(2W/(rho*S*CLmax))
 *
 * This is a steady, level, 1-g approximation.
 */
double performance_stall_speed(
    double mass_kg,
    double density_kg_m3,
    double wing_area_m2,
    double maximum_lift_coefficient)
{
    double weight_n;
    double numerator;
    double denominator;

    if (!valid_positive_finite(mass_kg) ||
        !valid_positive_finite(density_kg_m3) ||
        !valid_positive_finite(wing_area_m2) ||
        !valid_positive_finite(maximum_lift_coefficient)) {
        return NAN;
    }

    weight_n =
        aero_weight_from_mass(mass_kg);

    if (!isfinite(weight_n)) {
        return NAN;
    }

    numerator =
        2.0 * weight_n;

    denominator =
        density_kg_m3
        * wing_area_m2
        * maximum_lift_coefficient;

    return sqrt(numerator / denominator);
}


/*
 * Numerical maximum level-flight speed solver.
 *
 * Strategy:
 *
 * 1. Divide the requested velocity interval into multiple sections.
 *
 * 2. Evaluate:
 *
 *        F(V) = T_available - D
 *
 *    at each point.
 *
 * 3. Detect every sign change.
 *
 * 4. Use bisection to refine each root.
 *
 * 5. Return the highest root.
 *
 * This is deliberately straightforward rather than optimized.
 *
 * Searching for every sign change is important because the aircraft
 * can potentially have two level-flight equilibrium speeds:
 *
 *     low-speed intersection  -> minimum sustained speed
 *     high-speed intersection -> maximum level speed
 */
double performance_max_level_speed(
    double mass_kg,
    double density_kg_m3,
    double speed_of_sound_m_s,
    double wing_area_m2,
    double zero_lift_drag_coefficient,
    double oswald_efficiency,
    double aspect_ratio,
    double maximum_static_thrust_n,
    double throttle,
    double velocity_min_m_s,
    double velocity_max_m_s,
    int search_intervals)
{
    double interval_width;
    double previous_velocity;
    double previous_margin;
    double maximum_root = NAN;

    int i;

    if (!valid_positive_finite(mass_kg) ||
        !valid_positive_finite(density_kg_m3) ||
        !valid_positive_finite(speed_of_sound_m_s) ||
        !valid_positive_finite(wing_area_m2) ||
        !valid_non_negative_finite(zero_lift_drag_coefficient) ||
        !valid_positive_finite(oswald_efficiency) ||
        !valid_positive_finite(aspect_ratio) ||
        !valid_positive_finite(maximum_static_thrust_n) ||
        !valid_throttle(throttle) ||
        !valid_positive_finite(velocity_min_m_s) ||
        !valid_positive_finite(velocity_max_m_s) ||
        velocity_max_m_s <= velocity_min_m_s ||
        search_intervals < 10) {
        return NAN;
    }

    interval_width =
        (velocity_max_m_s - velocity_min_m_s)
        / (double)search_intervals;

    previous_velocity =
        velocity_min_m_s;

    previous_margin =
        level_flight_thrust_margin(
            previous_velocity,
            mass_kg,
            density_kg_m3,
            speed_of_sound_m_s,
            wing_area_m2,
            zero_lift_drag_coefficient,
            oswald_efficiency,
            aspect_ratio,
            maximum_static_thrust_n,
            throttle
        );

    if (!isfinite(previous_margin)) {
        return NAN;
    }

    for (i = 1; i <= search_intervals; ++i) {
        double current_velocity;
        double current_margin;

        current_velocity =
            velocity_min_m_s
            + (double)i * interval_width;

        current_margin =
            level_flight_thrust_margin(
                current_velocity,
                mass_kg,
                density_kg_m3,
                speed_of_sound_m_s,
                wing_area_m2,
                zero_lift_drag_coefficient,
                oswald_efficiency,
                aspect_ratio,
                maximum_static_thrust_n,
                throttle
            );

        if (!isfinite(current_margin)) {
            return NAN;
        }

        /*
         * Exact/near-zero point.
         */
        if (fabs(previous_margin) <= THRUST_ROOT_TOLERANCE_N) {
            maximum_root = previous_velocity;
        }

        /*
         * A sign change means that F(V)=T-D crossed zero.
         */
        if ((previous_margin > 0.0 &&
             current_margin < 0.0) ||
            (previous_margin < 0.0 &&
             current_margin > 0.0)) {

            double lower_velocity;
            double upper_velocity;
            double lower_margin;
            double root_velocity;
            int iteration;

            lower_velocity = previous_velocity;
            upper_velocity = current_velocity;
            lower_margin = previous_margin;

            /*
             * Bisection is robust because the sign change guarantees
             * that a root exists inside this interval for a continuous
             * thrust/drag model.
             */
            for (iteration = 0;
                 iteration < MAX_BISECTION_ITERATIONS;
                 ++iteration) {

                double midpoint_velocity;
                double midpoint_margin;

                midpoint_velocity =
                    0.5
                    * (lower_velocity + upper_velocity);

                midpoint_margin =
                    level_flight_thrust_margin(
                        midpoint_velocity,
                        mass_kg,
                        density_kg_m3,
                        speed_of_sound_m_s,
                        wing_area_m2,
                        zero_lift_drag_coefficient,
                        oswald_efficiency,
                        aspect_ratio,
                        maximum_static_thrust_n,
                        throttle
                    );

                if (!isfinite(midpoint_margin)) {
                    return NAN;
                }

                root_velocity = midpoint_velocity;

                if (fabs(midpoint_margin)
                    <= THRUST_ROOT_TOLERANCE_N) {
                    break;
                }

                /*
                 * Keep the half-interval containing the sign change.
                 */
                if ((lower_margin > 0.0 &&
                     midpoint_margin > 0.0) ||
                    (lower_margin < 0.0 &&
                     midpoint_margin < 0.0)) {

                    lower_velocity = midpoint_velocity;
                    lower_margin = midpoint_margin;

                } else {
                    upper_velocity = midpoint_velocity;
                }
            }

            /*
             * We are looking for maximum level-flight speed,
             * therefore retain the highest root found.
             */
            if (!isfinite(maximum_root) ||
                root_velocity > maximum_root) {
                maximum_root = root_velocity;
            }
        }

        previous_velocity = current_velocity;
        previous_margin = current_margin;
    }

    /*
     * Check the final search point for a near-zero solution.
     */
    if (fabs(previous_margin) <= THRUST_ROOT_TOLERANCE_N) {
        maximum_root = previous_velocity;
    }

    return maximum_root;
}


/*
 * ============================================================================
 * Analytical Breguet jet range
 * ============================================================================
 *
 *     R = V/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 * Since:
 *
 *     W = m*g
 *
 * and the same g appears in both initial and final weight,
 *
 *     Wi/Wf = mi/mf
 *
 * However, we deliberately convert mass to weight explicitly so
 * that the physical distinction remains clear in the implementation.
 */
double performance_jet_range(
    double velocity_m_s,
    double lift_to_drag_ratio,
    double tsfc_kg_n_s,
    double initial_mass_kg,
    double final_mass_kg)
{
    double initial_weight_n;
    double final_weight_n;
    double weight_ratio;
    double logarithmic_term;

    if (!valid_positive_finite(velocity_m_s) ||
        !valid_positive_finite(lift_to_drag_ratio) ||
        !valid_positive_finite(tsfc_kg_n_s) ||
        !valid_positive_finite(initial_mass_kg) ||
        !valid_positive_finite(final_mass_kg) ||
        final_mass_kg >= initial_mass_kg) {
        return NAN;
    }

    /*
     * Convert mass to weight.
     *
     * mass:   kg
     * weight: N
     */
    initial_weight_n =
        aero_weight_from_mass(initial_mass_kg);

    final_weight_n =
        aero_weight_from_mass(final_mass_kg);

    if (!isfinite(initial_weight_n) ||
        !isfinite(final_weight_n) ||
        final_weight_n <= 0.0) {
        return NAN;
    }

    weight_ratio =
        initial_weight_n / final_weight_n;

    logarithmic_term =
        log(weight_ratio);

    if (!isfinite(logarithmic_term) ||
        logarithmic_term <= 0.0) {
        return NAN;
    }

    return
        velocity_m_s
        / (GRAVITY * tsfc_kg_n_s)
        * lift_to_drag_ratio
        * logarithmic_term;
}


/*
 * ============================================================================
 * Analytical Breguet jet endurance
 * ============================================================================
 *
 *     E = 1/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 * This is an idealized constant-condition result.
 */
double performance_jet_endurance(
    double lift_to_drag_ratio,
    double tsfc_kg_n_s,
    double initial_mass_kg,
    double final_mass_kg)
{
    double initial_weight_n;
    double final_weight_n;
    double weight_ratio;
    double logarithmic_term;

    if (!valid_positive_finite(lift_to_drag_ratio) ||
        !valid_positive_finite(tsfc_kg_n_s) ||
        !valid_positive_finite(initial_mass_kg) ||
        !valid_positive_finite(final_mass_kg) ||
        final_mass_kg >= initial_mass_kg) {
        return NAN;
    }

    initial_weight_n =
        aero_weight_from_mass(initial_mass_kg);

    final_weight_n =
        aero_weight_from_mass(final_mass_kg);

    if (!isfinite(initial_weight_n) ||
        !isfinite(final_weight_n) ||
        final_weight_n <= 0.0) {
        return NAN;
    }

    weight_ratio =
        initial_weight_n / final_weight_n;

    logarithmic_term =
        log(weight_ratio);

    if (!isfinite(logarithmic_term) ||
        logarithmic_term <= 0.0) {
        return NAN;
    }

    return
        logarithmic_term
        * lift_to_drag_ratio
        / (GRAVITY * tsfc_kg_n_s);
}