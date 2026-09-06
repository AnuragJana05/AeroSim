#include "atmosphere.h"

#include <math.h>

/*
 * ============================================================================
 * AeroSim - Simplified International Standard Atmosphere
 * ============================================================================
 *
 * Model:
 *   - Troposphere:       0 km to 11 km
 *   - Lower stratosphere: 11 km to 20 km
 *
 * Assumptions:
 *   - Hydrostatic equilibrium
 *   - Ideal gas
 *   - Dry air
 *   - Constant gravitational acceleration
 *   - Standard ISA temperature profile
 *
 * All calculations use SI units and double precision.
 * ============================================================================
 */


/* --------------------------------------------------------------------------
 * Physical constants
 * -------------------------------------------------------------------------- */

/* Standard gravitational acceleration [m/s^2] */
static const double GRAVITY = 9.80665;

/* Specific gas constant for dry air [J/(kg*K)] */
static const double GAS_CONSTANT_AIR = 287.05287;

/* Ratio of specific heats for air [-] */
static const double GAMMA_AIR = 1.4;


/* --------------------------------------------------------------------------
 * ISA sea-level reference conditions
 * -------------------------------------------------------------------------- */

/* Sea-level standard temperature [K] */
static const double ISA_SEA_LEVEL_TEMPERATURE = 288.15;

/* Sea-level standard pressure [Pa] */
static const double ISA_SEA_LEVEL_PRESSURE = 101325.0;


/* --------------------------------------------------------------------------
 * ISA layer boundaries
 * -------------------------------------------------------------------------- */

/* Maximum altitude supported by this simplified model [m] */
static const double ISA_MAX_ALTITUDE = 20000.0;

/* Boundary between troposphere and lower stratosphere [m] */
static const double TROPOPAUSE_ALTITUDE = 11000.0;


/* --------------------------------------------------------------------------
 * ISA temperature lapse rates
 * -------------------------------------------------------------------------- */

/*
 * Tropospheric lapse rate.
 *
 * Temperature decreases by 6.5 K for every kilometre of altitude.
 *
 * Units: K/m
 */
static const double TROPOSPHERE_LAPSE_RATE = 0.0065;


/*
 * Lower-stratosphere lapse rate.
 *
 * In the simplified ISA model used here, temperature is constant
 * from 11 km to 20 km.
 *
 * Units: K/m
 */
static const double LOWER_STRATOSPHERE_LAPSE_RATE = 0.0;


/* --------------------------------------------------------------------------
 * Internal helper functions
 * -------------------------------------------------------------------------- */

/*
 * Validate an altitude input.
 *
 * Valid altitude range:
 *
 *     0 m <= altitude <= 20,000 m
 *
 * This model deliberately rejects negative altitude and altitudes
 * above 20 km rather than silently extrapolating the ISA equations.
 */
static int altitude_is_valid(double altitude_m)
{
    return isfinite(altitude_m) &&
           altitude_m >= 0.0 &&
           altitude_m <= ISA_MAX_ALTITUDE;
}


/*
 * Calculate temperature at altitude.
 *
 * This helper assumes the altitude has already been validated.
 */
static double temperature_internal(double altitude_m)
{
    /*
     * Troposphere:
     *
     *     T = T0 - L*h
     *
     * where L is the positive magnitude of the temperature lapse rate.
     */
    if (altitude_m <= TROPOPAUSE_ALTITUDE) {
        return ISA_SEA_LEVEL_TEMPERATURE
             - TROPOSPHERE_LAPSE_RATE * altitude_m;
    }

    /*
     * Lower stratosphere:
     *
     * Temperature remains constant at the tropopause in this
     * simplified ISA model.
     */
    return ISA_SEA_LEVEL_TEMPERATURE
         - TROPOSPHERE_LAPSE_RATE * TROPOPAUSE_ALTITUDE
         + LOWER_STRATOSPHERE_LAPSE_RATE
           * (altitude_m - TROPOPAUSE_ALTITUDE);
}


/*
 * Calculate pressure in a layer with a non-zero temperature lapse rate.
 *
 * Hydrostatic equilibrium:
 *
 *     dP/dh = -rho*g
 *
 * Ideal gas:
 *
 *     rho = P/(R*T)
 *
 * Combining these gives the standard ISA pressure relation:
 *
 *     P = P_base * (T/T_base)^(g/(R*L))
 *
 * Here L is the positive magnitude of the temperature lapse rate.
 */
static double pressure_gradient_layer(
    double pressure_base_pa,
    double temperature_base_k,
    double temperature_k,
    double lapse_rate_k_per_m)
{
    return pressure_base_pa
         * pow(
             temperature_k / temperature_base_k,
             GRAVITY / (GAS_CONSTANT_AIR * lapse_rate_k_per_m)
           );
}


/*
 * Calculate pressure in an isothermal layer.
 *
 * When L = 0:
 *
 *     P = P_base * exp[-g(h-h_base)/(R*T)]
 */
static double pressure_isothermal_layer(
    double pressure_base_pa,
    double temperature_base_k,
    double altitude_difference_m)
{
    return pressure_base_pa
         * exp(
             -GRAVITY * altitude_difference_m
             / (GAS_CONSTANT_AIR * temperature_base_k)
           );
}


/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

double atmosphere_temperature(double altitude_m)
{
    if (!altitude_is_valid(altitude_m)) {
        return NAN;
    }

    return temperature_internal(altitude_m);
}


double atmosphere_pressure(double altitude_m)
{
    double tropopause_temperature_k;
    double tropopause_pressure_pa;

    if (!altitude_is_valid(altitude_m)) {
        return NAN;
    }

    /*
     * Troposphere:
     *
     *     0 <= h <= 11 km
     */
    if (altitude_m <= TROPOPAUSE_ALTITUDE) {
        double temperature_k = temperature_internal(altitude_m);

        return pressure_gradient_layer(
            ISA_SEA_LEVEL_PRESSURE,
            ISA_SEA_LEVEL_TEMPERATURE,
            temperature_k,
            TROPOSPHERE_LAPSE_RATE
        );
    }

    /*
     * First calculate the pressure at the tropopause.
     *
     * This becomes the base pressure for the stratosphere.
     */
    tropopause_temperature_k =
        temperature_internal(TROPOPAUSE_ALTITUDE);

    tropopause_pressure_pa =
        pressure_gradient_layer(
            ISA_SEA_LEVEL_PRESSURE,
            ISA_SEA_LEVEL_TEMPERATURE,
            tropopause_temperature_k,
            TROPOSPHERE_LAPSE_RATE
        );

    /*
     * Lower stratosphere:
     *
     *     11 km < h <= 20 km
     *
     * Temperature is constant, therefore use the isothermal
     * barometric pressure equation.
     */
    return pressure_isothermal_layer(
        tropopause_pressure_pa,
        tropopause_temperature_k,
        altitude_m - TROPOPAUSE_ALTITUDE
    );
}


double atmosphere_density(double altitude_m)
{
    double temperature_k;
    double pressure_pa;

    if (!altitude_is_valid(altitude_m)) {
        return NAN;
    }

    temperature_k = atmosphere_temperature(altitude_m);
    pressure_pa = atmosphere_pressure(altitude_m);

    /*
     * Ideal gas law:
     *
     *     rho = P / (R*T)
     */
    return pressure_pa
         / (GAS_CONSTANT_AIR * temperature_k);
}


double atmosphere_speed_of_sound(double temperature_k)
{
    /*
     * Speed of sound in an ideal gas:
     *
     *     a = sqrt(gamma * R * T)
     *
     * Temperature must be greater than absolute zero.
     */
    if (!isfinite(temperature_k) || temperature_k <= 0.0) {
        return NAN;
    }

    return sqrt(
        GAMMA_AIR
        * GAS_CONSTANT_AIR
        * temperature_k
    );
}