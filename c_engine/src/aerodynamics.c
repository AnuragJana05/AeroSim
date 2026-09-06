#include "aerodynamics.h"

#include <math.h>


/*
 * ============================================================================
 * AeroSim - Basic Aerodynamic Model
 * ============================================================================
 *
 * Physical model:
 *
 *     q  = 0.5 * rho * V^2
 *
 *     CL = W / (q * S)
 *
 *     CD = CD0 + k * CL^2
 *
 *     k  = 1 / (pi * e * AR)
 *
 *     L  = q * S * CL
 *
 *     D  = q * S * CD
 *
 *     L/D = lift / drag
 *
 *
 * Assumptions:
 *
 *   - Steady flight
 *   - Level flight where CL is calculated
 *   - 1-g loading
 *   - Rigid aircraft
 *   - Fixed reference wing area
 *   - Subsonic aerodynamic regime
 *   - Parabolic drag polar
 *   - Constant CD0
 *   - Constant Oswald efficiency factor
 *
 * This is an engineering approximation, not a high-fidelity
 * aerodynamic model.
 * ============================================================================
 */


/* Standard gravitational acceleration [m/s^2]. */
static const double GRAVITY = 9.80665;


/*
 * Mathematical constant pi.
 *
 * This is defined locally rather than relying on a non-standard
 * M_PI macro, which improves portability between compilers.
 */
static const double PI = 3.14159265358979323846;


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


/*
 * ============================================================================
 * Public API
 * ============================================================================
 */


/*
 * Weight is a force caused by gravity:
 *
 *     W = m * g
 *
 * Mass and weight must not be confused:
 *
 *     mass   -> kg
 *     weight -> N
 *
 * AeroSim deliberately performs this conversion explicitly here.
 */
double aero_weight_from_mass(double mass_kg)
{
    if (!valid_positive_finite(mass_kg)) {
        return NAN;
    }

    return mass_kg * GRAVITY;
}


/*
 * Dynamic pressure:
 *
 *     q = 1/2 * rho * V^2
 *
 * Dynamic pressure represents the pressure scale associated
 * with the aircraft moving through the surrounding air.
 *
 * rho -> kg/m^3
 * V   -> m/s
 * q   -> Pa
 */
double aero_dynamic_pressure(
    double density_kg_m3,
    double velocity_m_s)
{
    if (!valid_positive_finite(density_kg_m3) ||
        !valid_positive_finite(velocity_m_s)) {
        return NAN;
    }

    return 0.5
         * density_kg_m3
         * velocity_m_s
         * velocity_m_s;
}


/*
 * For steady, level, 1-g flight:
 *
 *     L = W
 *
 * Aerodynamic lift is:
 *
 *     L = q * S * CL
 *
 * Therefore:
 *
 *     CL = W / (q * S)
 *
 * The aircraft input is mass rather than weight.
 * We explicitly calculate:
 *
 *     W = m * g
 *
 * before calculating CL.
 */
double aero_lift_coefficient_level_flight(
    double mass_kg,
    double density_kg_m3,
    double velocity_m_s,
    double wing_area_m2)
{
    double weight_n;
    double dynamic_pressure_pa;

    if (!valid_positive_finite(mass_kg) ||
        !valid_positive_finite(density_kg_m3) ||
        !valid_positive_finite(velocity_m_s) ||
        !valid_positive_finite(wing_area_m2)) {
        return NAN;
    }

    weight_n = aero_weight_from_mass(mass_kg);

    dynamic_pressure_pa =
        aero_dynamic_pressure(
            density_kg_m3,
            velocity_m_s
        );

    if (!isfinite(weight_n) ||
        !isfinite(dynamic_pressure_pa) ||
        dynamic_pressure_pa <= 0.0) {
        return NAN;
    }

    return weight_n
         / (dynamic_pressure_pa * wing_area_m2);
}


/*
 * The induced drag factor is:
 *
 *     k = 1 / (pi * e * AR)
 *
 * Induced drag increases with CL^2:
 *
 *     CDi = k * CL^2
 *
 * A larger aspect ratio generally reduces induced drag.
 */
double aero_induced_drag_factor(
    double oswald_efficiency,
    double aspect_ratio)
{
    if (!valid_positive_finite(oswald_efficiency) ||
        !valid_positive_finite(aspect_ratio)) {
        return NAN;
    }

    return 1.0
         / (PI * oswald_efficiency * aspect_ratio);
}


/*
 * Parabolic drag polar:
 *
 *     CD = CD0 + k * CL^2
 *
 * CD0 represents the approximately zero-lift/parasitic component.
 *
 * k*CL^2 represents induced drag associated with producing lift.
 *
 * This relationship is intentionally simplified and should not
 * be considered valid through stall, strong flow separation,
 * or transonic/supersonic drag-rise regions.
 */
double aero_drag_coefficient(
    double zero_lift_drag_coefficient,
    double induced_drag_factor,
    double lift_coefficient)
{
    if (!valid_non_negative_finite(zero_lift_drag_coefficient) ||
        !valid_non_negative_finite(induced_drag_factor) ||
        !isfinite(lift_coefficient)) {
        return NAN;
    }

    return zero_lift_drag_coefficient
         + induced_drag_factor
         * lift_coefficient
         * lift_coefficient;
}


/*
 * Lift:
 *
 *     L = q * S * CL
 *
 * q  -> Pa = N/m^2
 * S  -> m^2
 * CL -> dimensionless
 *
 * Therefore:
 *
 *     L -> N
 */
double aero_lift(
    double dynamic_pressure_pa,
    double wing_area_m2,
    double lift_coefficient)
{
    if (!valid_non_negative_finite(dynamic_pressure_pa) ||
        !valid_positive_finite(wing_area_m2) ||
        !isfinite(lift_coefficient)) {
        return NAN;
    }

    return dynamic_pressure_pa
         * wing_area_m2
         * lift_coefficient;
}


/*
 * Drag:
 *
 *     D = q * S * CD
 *
 * q  -> Pa
 * S  -> m^2
 * CD -> dimensionless
 *
 * Therefore:
 *
 *     D -> N
 */
double aero_drag(
    double dynamic_pressure_pa,
    double wing_area_m2,
    double drag_coefficient)
{
    if (!valid_non_negative_finite(dynamic_pressure_pa) ||
        !valid_positive_finite(wing_area_m2) ||
        !valid_non_negative_finite(drag_coefficient)) {
        return NAN;
    }

    return dynamic_pressure_pa
         * wing_area_m2
         * drag_coefficient;
}


/*
 * Aerodynamic efficiency is represented by:
 *
 *     L/D
 *
 * A larger value means that more lift is generated for a given
 * amount of drag.
 */
double aero_lift_to_drag_ratio(
    double lift_n,
    double drag_n)
{
    if (!valid_non_negative_finite(lift_n) ||
        !valid_positive_finite(drag_n)) {
        return NAN;
    }

    return lift_n / drag_n;
}