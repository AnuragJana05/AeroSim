#ifndef AEROSIM_ATMOSPHERE_H
#define AEROSIM_ATMOSPHERE_H

/*
 * AeroSim - Simplified International Standard Atmosphere
 *
 * All quantities use SI units:
 *   altitude      : m
 *   temperature   : K
 *   pressure      : Pa
 *   density       : kg/m^3
 *   speed of sound: m/s
 *
 * Supported altitude range:
 *   0 m to 20,000 m
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Calculate atmospheric temperature at a given geometric altitude.
 *
 * Returns:
 *   Temperature in kelvin on success.
 *   NAN if altitude is outside the supported ISA range.
 */
double atmosphere_temperature(double altitude_m);

/*
 * Calculate atmospheric pressure at a given geometric altitude.
 *
 * Returns:
 *   Pressure in pascals on success.
 *   NAN if altitude is outside the supported ISA range.
 */
double atmosphere_pressure(double altitude_m);

/*
 * Calculate atmospheric density at a given geometric altitude.
 *
 * Returns:
 *   Density in kg/m^3 on success.
 *   NAN if altitude is outside the supported ISA range.
 */
double atmosphere_density(double altitude_m);

/*
 * Calculate speed of sound from atmospheric temperature.
 *
 * Returns:
 *   Speed of sound in m/s on success.
 *   NAN if temperature is not physically valid.
 */
double atmosphere_speed_of_sound(double temperature_k);

#ifdef __cplusplus
}
#endif

#endif /* AEROSIM_ATMOSPHERE_H */