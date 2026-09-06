#ifndef AEROSIM_AERODYNAMICS_H
#define AEROSIM_AERODYNAMICS_H

/*
 * AeroSim - Basic aerodynamic model
 *
 * All quantities use SI units:
 *
 *   mass       : kg
 *   weight     : N
 *   velocity   : m/s
 *   density    : kg/m^3
 *   area       : m^2
 *   span       : m
 *   pressure   : Pa
 *   lift/drag  : N
 *
 * Coefficients and ratios such as CL, CD, k, e and L/D
 * are dimensionless.
 *
 * This module models steady, level, 1-g flight using
 * a simplified parabolic drag polar.
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Convert aircraft mass into aircraft weight.
 *
 * Physics:
 *
 *     W = m * g
 *
 * mass is measured in kilograms.
 * weight is a force measured in newtons.
 */
double aero_weight_from_mass(double mass_kg);


/*
 * Calculate dynamic pressure.
 *
 * Physics:
 *
 *     q = 0.5 * rho * V^2
 *
 * Returns NAN for invalid density or velocity.
 */
double aero_dynamic_pressure(
    double density_kg_m3,
    double velocity_m_s
);


/*
 * Calculate lift coefficient for steady, level, 1-g flight.
 *
 * Physics:
 *
 *     L = W
 *
 * and
 *
 *     L = q * S * CL
 *
 * Therefore:
 *
 *     CL = W / (q * S)
 *
 * The function accepts aircraft mass rather than weight and
 * explicitly converts mass to weight using gravitational
 * acceleration.
 */
double aero_lift_coefficient_level_flight(
    double mass_kg,
    double density_kg_m3,
    double velocity_m_s,
    double wing_area_m2
);


/*
 * Calculate induced drag factor.
 *
 * Physics:
 *
 *     k = 1 / (pi * e * AR)
 *
 * where:
 *     e  = Oswald/span efficiency factor
 *     AR = wing aspect ratio
 */
double aero_induced_drag_factor(
    double oswald_efficiency,
    double aspect_ratio
);


/*
 * Calculate drag coefficient using the parabolic drag polar.
 *
 * Physics:
 *
 *     CD = CD0 + k * CL^2
 */
double aero_drag_coefficient(
    double zero_lift_drag_coefficient,
    double induced_drag_factor,
    double lift_coefficient
);


/*
 * Calculate aerodynamic lift.
 *
 * Physics:
 *
 *     L = q * S * CL
 */
double aero_lift(
    double dynamic_pressure_pa,
    double wing_area_m2,
    double lift_coefficient
);


/*
 * Calculate aerodynamic drag.
 *
 * Physics:
 *
 *     D = q * S * CD
 */
double aero_drag(
    double dynamic_pressure_pa,
    double wing_area_m2,
    double drag_coefficient
);


/*
 * Calculate lift-to-drag ratio.
 *
 * Physics:
 *
 *     L/D
 *
 * Returns NAN when drag is zero or invalid.
 */
double aero_lift_to_drag_ratio(
    double lift_n,
    double drag_n
);


#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_AERODYNAMICS_H */