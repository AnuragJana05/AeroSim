#ifndef AEROSIM_PROPULSION_H
#define AEROSIM_PROPULSION_H

/*
 * AeroSim - Generic Jet Propulsion Model
 *
 * All quantities use SI units:
 *
 *   thrust          : N
 *   fuel mass flow  : kg/s
 *   TSFC            : kg/(N*s)
 *   Mach number     : dimensionless
 *   throttle        : dimensionless, 0 to 1
 *   density ratio   : dimensionless
 *
 * This module intentionally contains NO aerodynamic calculations.
 *
 * The engine model is a simplified engineering approximation:
 *
 *     T = T_SL * throttle * altitude_factor * mach_factor
 *
 * where:
 *
 *     altitude_factor = sigma^n
 *
 *     mach_factor = max(0, 1 - k_M * M)
 *
 * and:
 *
 *     sigma = rho / rho_SL
 *
 * The atmospheric density ratio is supplied by the caller.
 * This allows the propulsion model to remain independent of
 * the atmosphere implementation.
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Calculate available thrust.
 *
 * Parameters:
 *
 *   maximum_static_thrust_n : maximum engine thrust at sea level,
 *                             zero Mach and full throttle [N]
 *
 *   throttle                : throttle command [0, 1]
 *
 *   density_ratio           : local air density / sea-level density [-]
 *
 *   mach                    : flight Mach number [-]
 *
 * Returns:
 *
 *   Available thrust [N]
 *
 * Returns NAN for invalid inputs.
 */
double propulsion_thrust_available(
    double maximum_static_thrust_n,
    double throttle,
    double density_ratio,
    double mach
);


/*
 * Calculate fuel mass flow from thrust and TSFC.
 *
 * Physics:
 *
 *     mdot_f = TSFC * T
 *
 * Parameters:
 *
 *   thrust_n       : engine thrust [N]
 *   tsfc_kg_n_s    : thrust-specific fuel consumption [kg/(N*s)]
 *
 * Returns:
 *
 *   Fuel mass flow [kg/s]
 *
 * Returns NAN for invalid inputs.
 */
double propulsion_fuel_mass_flow(
    double thrust_n,
    double tsfc_kg_n_s
);


/*
 * Calculate the thrust fraction produced by the simplified
 * altitude/Mach model.
 *
 * This is useful for analysis and testing.
 *
 *     thrust_fraction =
 *         sigma^n * max(0, 1 - k_M*M)
 *
 * Returns a dimensionless value between 0 and 1 for the
 * intended subsonic operating range.
 */
double propulsion_thrust_fraction(
    double density_ratio,
    double mach
);


#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_PROPULSION_H */