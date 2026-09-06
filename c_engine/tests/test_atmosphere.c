#include <math.h>
#include <stdio.h>

#include "atmosphere.h"


/*
 * Compare two floating-point values using an absolute tolerance.
 */
static int nearly_equal(double actual, double expected, double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}


/*
 * Test one ISA altitude.
 */
static int test_altitude(
    double altitude_m,
    double expected_temperature_k,
    double expected_pressure_pa,
    double expected_density_kg_m3,
    double expected_speed_of_sound_m_s)
{
    double temperature_k;
    double pressure_pa;
    double density_kg_m3;
    double speed_of_sound_m_s;

    temperature_k = atmosphere_temperature(altitude_m);
    pressure_pa = atmosphere_pressure(altitude_m);
    density_kg_m3 = atmosphere_density(altitude_m);

    speed_of_sound_m_s =
        atmosphere_speed_of_sound(temperature_k);

    printf("\nAltitude: %.0f m\n", altitude_m);

    printf("  Temperature : %.6f K\n", temperature_k);
    printf("  Pressure    : %.3f Pa\n", pressure_pa);
    printf("  Density     : %.6f kg/m^3\n", density_kg_m3);
    printf("  Sound speed : %.6f m/s\n", speed_of_sound_m_s);

    if (!nearly_equal(
            temperature_k,
            expected_temperature_k,
            0.01)) {
        printf("  FAIL: temperature\n");
        return 0;
    }

    if (!nearly_equal(
            pressure_pa,
            expected_pressure_pa,
            10.0)) {
        printf("  FAIL: pressure\n");
        return 0;
    }

    if (!nearly_equal(
            density_kg_m3,
            expected_density_kg_m3,
            0.001)) {
        printf("  FAIL: density\n");
        return 0;
    }

    if (!nearly_equal(
            speed_of_sound_m_s,
            expected_speed_of_sound_m_s,
            0.1)) {
        printf("  FAIL: speed of sound\n");
        return 0;
    }

    printf("  PASS\n");

    return 1;
}


static int test_invalid_altitudes(void)
{
    double below_sea_level;
    double above_model_limit;
    double nan_value;

    below_sea_level = atmosphere_temperature(-1.0);
    above_model_limit = atmosphere_temperature(20001.0);
    nan_value = atmosphere_temperature(NAN);

    if (!isnan(below_sea_level)) {
        printf("FAIL: negative altitude was not rejected\n");
        return 0;
    }

    if (!isnan(above_model_limit)) {
        printf("FAIL: altitude above 20 km was not rejected\n");
        return 0;
    }

    if (!isnan(nan_value)) {
        printf("FAIL: NaN altitude was not rejected\n");
        return 0;
    }

    printf("Invalid-input tests: PASS\n");

    return 1;
}


int main(void)
{
    int passed = 1;

    printf("============================================\n");
    printf(" AeroSim - Atmosphere Module Tests\n");
    printf("============================================\n");

    /*
     * Sea level ISA:
     *
     * T    = 288.15 K
     * P    = 101325 Pa
     * rho  = approximately 1.225 kg/m^3
     * a    = approximately 340.3 m/s
     */
    passed &= test_altitude(
        0.0,
        288.15,
        101325.0,
        1.225,
        340.3
    );

    /*
     * Representative tropospheric point:
     * 5 km
     */
    passed &= test_altitude(
        5000.0,
        255.65,
        54019.0,
        0.736,
        320.5
    );

    /*
     * Tropopause:
     * 11 km
     */
    passed &= test_altitude(
        11000.0,
        216.65,
        22632.0,
        0.364,
        295.1
    );

    /*
     * Representative lower-stratosphere point:
     * 20 km
     */
    passed &= test_altitude(
        20000.0,
        216.65,
        5475.0,
        0.088,
        295.1
    );

    passed &= test_invalid_altitudes();

    printf("\n============================================\n");

    if (passed) {
        printf(" ALL ATMOSPHERE TESTS PASSED\n");
        printf("============================================\n");
        return 0;
    }

    printf(" ATMOSPHERE TESTS FAILED\n");
    printf("============================================\n");

    return 1;
}