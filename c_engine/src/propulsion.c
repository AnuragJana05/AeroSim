#include "propulsion.h"

#include <math.h>


/*
 * ============================================================================
 * AeroSim - Generic Jet Propulsion Model
 * ============================================================================
 *
 * Simplified generic jet engine model:
 *
 *     T = T_SL * delta * sigma^n * F_M
 *
 * where:
 *
 *     T_SL = maximum sea-level static thrust
 *     delta = throttle setting
 *     sigma = local density / sea-level density
 *     n = altitude-lapse exponent
 *
 * Mach correction:
 *
 *     F_M = max(0, 1 - k_M*M)
 *
 * Therefore:
 *
 *     T = T_SL * delta * sigma^n * max(0, 1-k_M*M)
 *
 *
 * V1 ASSUMPTIONS
 * --------------
 *
 * 1. Generic turbojet/turbofan-like engine.
 *
 * 2. Maximum static thrust is supplied by the user.
 *
 * 3. Thrust varies linearly with throttle.
 *
 * 4. Altitude effects are approximated using density ratio.
 *
 * 5. Mach effects are represented by a simple linear correction.
 *
 * 6. The model is intended primarily for subsonic flight.
 *
 * 7. TSFC is supplied externally and is initially treated as constant.
 *
 * 8. No engine spool dynamics are modeled.
 *
 * 9. No compressor/turbine thermodynamics are modeled.
 *
 * 10. No afterburner is modeled.
 *
 * 11. No inlet pressure recovery is modeled.
 *
 * 12. No ram drag is explicitly modeled.
 *
 * 13. No engine installation losses are modeled.
 *
 *
 * IMPORTANT:
 * This is NOT a real engine performance deck.
 *
 * Real engine thrust depends on many additional variables including:
 *
 *     - ambient pressure and temperature
 *     - Mach number
 *     - mass flow
 *     - compressor pressure ratio
 *     - turbine temperature
 *     - nozzle conditions
 *     - throttle setting
 *     - engine operating state
 *     - inlet recovery
 *     - bleed extraction
 *     - afterburner state
 *
 * The purpose of this model is to provide a transparent propulsion
 * approximation suitable for AeroSim V1.
 * ============================================================================
 */


/* --------------------------------------------------------------------------
 * Model constants
 * -------------------------------------------------------------------------- */


/*
 * Altitude/thrust-lapse exponent.
 *
 * The density ratio is raised to this exponent:
 *
 *     altitude_factor = sigma^n
 *
 * n = 0.7 is a deliberately generic engineering approximation.
 *
 * It is NOT a universal jet-engine constant.
 */
static const double ALTITUDE_LAPSE_EXPONENT = 0.7;


/*
 * Mach thrust-loss coefficient.
 *
 *     F_M = 1 - k_M*M
 *
 * A value of 0.30 gives:
 *
 *     M = 0.0 -> F_M = 1.00
 *     M = 0.5 -> F_M = 0.85
 *     M = 1.0 -> F_M = 0.70
 *
 * This is only a simplified approximation.
 */
static const double MACH_THRUST_LOSS_COEFFICIENT = 0.30;


/* --------------------------------------------------------------------------
 * Internal validation helpers
 * -------------------------------------------------------------------------- */


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


/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */


/*
 * Calculate the dimensionless thrust fraction.
 *
 * The altitude effect is represented using density ratio:
 *
 *     sigma = rho / rho_SL
 *
 * and:
 *
 *     altitude_factor = sigma^n
 *
 * The Mach correction is:
 *
 *     mach_factor = max(0, 1-k_M*M)
 *
 * Combining them:
 *
 *     thrust_fraction =
 *         sigma^n * mach_factor
 *
 * This quantity represents the fraction of maximum static thrust
 * available before throttle is applied.
 */
double propulsion_thrust_fraction(
    double density_ratio,
    double mach)
{
    double altitude_factor;
    double mach_factor;

    if (!valid_positive_finite(density_ratio) ||
        !valid_non_negative_finite(mach)) {
        return NAN;
    }

    /*
     * Density ratio should physically be no greater than approximately
     * one for positive altitude in the standard atmosphere.
     *
     * We allow values greater than one because they can represent
     * denser-than-standard conditions and make this module more
     * generally useful.
     */
    altitude_factor =
        pow(
            density_ratio,
            ALTITUDE_LAPSE_EXPONENT
        );

    /*
     * Simplified Mach correction.
     *
     * The max operation prevents the model from producing negative
     * thrust at high Mach numbers.
     */
    mach_factor =
        1.0
        - MACH_THRUST_LOSS_COEFFICIENT * mach;

    if (mach_factor < 0.0) {
        mach_factor = 0.0;
    }

    return altitude_factor * mach_factor;
}


/*
 * Calculate available thrust.
 *
 *     T = T_SL * throttle * thrust_fraction
 *
 * where:
 *
 *     thrust_fraction =
 *         sigma^n * max(0, 1-k_M*M)
 *
 * At sea level:
 *
 *     sigma = 1
 *
 * At zero Mach:
 *
 *     mach_factor = 1
 *
 * Therefore:
 *
 *     T = T_SL * throttle
 *
 * which gives the expected static-thrust behavior.
 */
double propulsion_thrust_available(
    double maximum_static_thrust_n,
    double throttle,
    double density_ratio,
    double mach)
{
    double thrust_fraction;

    if (!valid_positive_finite(maximum_static_thrust_n) ||
        !valid_throttle(throttle) ||
        !valid_positive_finite(density_ratio) ||
        !valid_non_negative_finite(mach)) {
        return NAN;
    }

    thrust_fraction =
        propulsion_thrust_fraction(
            density_ratio,
            mach
        );

    if (!isfinite(thrust_fraction)) {
        return NAN;
    }

    return maximum_static_thrust_n
         * throttle
         * thrust_fraction;
}


/*
 * Calculate fuel mass flow.
 *
 * The definition of thrust-specific fuel consumption is:
 *
 *     TSFC = mdot_f / T
 *
 * Therefore:
 *
 *     mdot_f = TSFC * T
 *
 * Units:
 *
 *     TSFC = kg/(N*s)
 *     T    = N
 *
 * resulting in:
 *
 *     mdot_f = kg/s
 */
double propulsion_fuel_mass_flow(
    double thrust_n,
    double tsfc_kg_n_s)
{
    if (!valid_non_negative_finite(thrust_n) ||
        !valid_positive_finite(tsfc_kg_n_s)) {
        return NAN;
    }

    return thrust_n * tsfc_kg_n_s;
}