#ifndef AEROSIM_PERFORMANCE_H
#define AEROSIM_PERFORMANCE_H

/*
 * AeroSim - Aircraft Performance Model
 *
 * All quantities use SI units:
 *
 *   mass              : kg
 *   weight            : N
 *   velocity          : m/s
 *   density           : kg/m^3
 *   wing area         : m^2
 *   lift/drag         : N
 *   power             : W
 *   rate of climb     : m/s
 *   range             : m
 *   endurance         : s
 *   TSFC              : kg/(N*s)
 *
 * Coefficients and ratios are dimensionless.
 *
 * This module combines the previously implemented:
 *
 *   - atmosphere
 *   - aerodynamics
 *   - propulsion
 *
 * modules to calculate aircraft-level performance.
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Calculate excess thrust.
 *
 *     T_excess = T_available - D
 *
 * Positive excess thrust indicates that thrust remains after
 * overcoming aerodynamic drag.
 */
double performance_excess_thrust(
    double thrust_n,
    double drag_n
);


/*
 * Calculate excess power.
 *
 *     P_excess = (T - D) * V
 *
 * Positive excess power means the aircraft has energy available
 * for acceleration and/or climbing.
 */
double performance_excess_power(
    double thrust_n,
    double drag_n,
    double velocity_m_s
);


/*
 * Calculate rate of climb from excess power.
 *
 *     ROC = P_excess / W
 *
 * where:
 *
 *     W = m*g
 */
double performance_rate_of_climb(
    double thrust_n,
    double drag_n,
    double velocity_m_s,
    double mass_kg
);


/*
 * Calculate 1-g stall speed.
 *
 *     V_stall = sqrt(2W / (rho*S*CLmax))
 *
 * where:
 *
 *     W = m*g
 */
double performance_stall_speed(
    double mass_kg,
    double density_kg_m3,
    double wing_area_m2,
    double maximum_lift_coefficient
);


/*
 * Calculate maximum steady level-flight speed using a numerical search.
 *
 * The function searches a specified velocity interval for roots of:
 *
 *     T_available(V) - D(V) = 0
 *
 * Multiple roots are possible because the drag curve can have a
 * low-speed and high-speed intersection with the thrust curve.
 *
 * The highest valid root is returned.
 *
 * Returns NAN if no level-flight equilibrium is found.
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
    int search_intervals
);


/*
 * Calculate idealized analytical jet range using the Breguet equation.
 *
 *     R = V/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 * This assumes constant:
 *
 *     - speed
 *     - TSFC
 *     - L/D
 *     - altitude
 *     - flight condition
 *
 * Initial and final values are aircraft MASS values in kg.
 * They are explicitly converted to weights internally.
 */
double performance_jet_range(
    double velocity_m_s,
    double lift_to_drag_ratio,
    double tsfc_kg_n_s,
    double initial_mass_kg,
    double final_mass_kg
);


/*
 * Calculate idealized analytical jet endurance using the Breguet
 * endurance equation.
 *
 *     E = 1/(g*TSFC) * (L/D) * ln(Wi/Wf)
 *
 * Initial and final aircraft masses are supplied in kg.
 */
double performance_jet_endurance(
    double lift_to_drag_ratio,
    double tsfc_kg_n_s,
    double initial_mass_kg,
    double final_mass_kg
);


#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_PERFORMANCE_H */