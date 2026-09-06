#include "propulsion.h"

#include <math.h>
#include <stdio.h>


static int nearly_equal(
    double actual,
    double expected,
    double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}


static void print_result(
    const char *name,
    double actual,
    double expected,
    double tolerance,
    int *passed)
{
    if (nearly_equal(actual, expected, tolerance)) {
        printf(
            "PASS: %-35s actual = %.8f\n",
            name,
            actual
        );
    } else {
        printf(
            "FAIL: %-35s actual = %.8f expected = %.8f\n",
            name,
            actual,
            expected
        );

        *passed = 0;
    }
}


/*
 * ============================================================================
 * Thrust fraction tests
 * ============================================================================
 */


/*
 * Sea level, zero Mach:
 *
 * sigma = 1
 * M = 0
 *
 * thrust fraction = 1^0.7 * 1 = 1
 */
static void test_thrust_fraction_sea_level(int *passed)
{
    const double density_ratio = 1.0;
    const double mach = 0.0;
    const double expected_fraction = 1.0;

    double fraction =
        propulsion_thrust_fraction(
            density_ratio,
            mach
        );

    print_result(
        "Sea-level static thrust fraction",
        fraction,
        expected_fraction,
        1e-12,
        passed
    );
}


/*
 * Sea level, Mach 0.5:
 *
 * Mach factor = 1 - 0.3*0.5
 *             = 0.85
 *
 * sigma^0.7 = 1
 *
 * fraction = 0.85
 */
static void test_thrust_fraction_mach(int *passed)
{
    const double density_ratio = 1.0;
    const double mach = 0.5;
    const double expected_fraction = 0.85;

    double fraction =
        propulsion_thrust_fraction(
            density_ratio,
            mach
        );

    print_result(
        "Mach thrust correction",
        fraction,
        expected_fraction,
        1e-12,
        passed
    );
}


/*
 * Representative altitude condition:
 *
 * sigma = 0.5
 * M = 0
 *
 * fraction = 0.5^0.7
 */
static void test_thrust_fraction_altitude(int *passed)
{
    const double density_ratio = 0.5;
    const double mach = 0.0;

    const double expected_fraction =
        pow(0.5, 0.7);

    double fraction =
        propulsion_thrust_fraction(
            density_ratio,
            mach
        );

    print_result(
        "Altitude thrust correction",
        fraction,
        expected_fraction,
        1e-12,
        passed
    );
}


/*
 * ============================================================================
 * Thrust tests
 * ============================================================================
 */


/*
 * Maximum static thrust:
 *
 * T_SL = 100,000 N
 * throttle = 1
 * sigma = 1
 * M = 0
 *
 * T = 100,000 N
 */
static void test_static_thrust(int *passed)
{
    const double maximum_static_thrust_n = 100000.0;
    const double throttle = 1.0;
    const double density_ratio = 1.0;
    const double mach = 0.0;

    const double expected_thrust_n = 100000.0;

    double thrust_n =
        propulsion_thrust_available(
            maximum_static_thrust_n,
            throttle,
            density_ratio,
            mach
        );

    print_result(
        "Maximum static thrust",
        thrust_n,
        expected_thrust_n,
        1e-6,
        passed
    );
}


/*
 * Half throttle at sea level, zero Mach:
 *
 * T = 100,000 * 0.5
 *   = 50,000 N
 */
static void test_throttle(int *passed)
{
    const double maximum_static_thrust_n = 100000.0;
    const double throttle = 0.5;
    const double density_ratio = 1.0;
    const double mach = 0.0;

    const double expected_thrust_n = 50000.0;

    double thrust_n =
        propulsion_thrust_available(
            maximum_static_thrust_n,
            throttle,
            density_ratio,
            mach
        );

    print_result(
        "Throttle response",
        thrust_n,
        expected_thrust_n,
        1e-6,
        passed
    );
}


/*
 * Combined altitude and Mach test:
 *
 * T_SL = 100,000 N
 * throttle = 1
 * sigma = 0.5
 * M = 0.5
 *
 * T = 100000 * 0.5^0.7 * 0.85
 */
static void test_combined_thrust_lapse(int *passed)
{
    const double maximum_static_thrust_n = 100000.0;
    const double throttle = 1.0;
    const double density_ratio = 0.5;
    const double mach = 0.5;

    const double expected_thrust_n =
        100000.0
        * pow(0.5, 0.7)
        * 0.85;

    double thrust_n =
        propulsion_thrust_available(
            maximum_static_thrust_n,
            throttle,
            density_ratio,
            mach
        );

    print_result(
        "Combined altitude/Mach thrust lapse",
        thrust_n,
        expected_thrust_n,
        1e-6,
        passed
    );
}


/*
 * ============================================================================
 * Fuel-flow test
 * ============================================================================
 *
 * T = 50,000 N
 * TSFC = 0.00002 kg/(N*s)
 *
 * mdot = 50,000 * 0.00002
 *      = 1 kg/s
 */
static void test_fuel_mass_flow(int *passed)
{
    const double thrust_n = 50000.0;
    const double tsfc_kg_n_s = 0.00002;
    const double expected_mass_flow_kg_s = 1.0;

    double mass_flow_kg_s =
        propulsion_fuel_mass_flow(
            thrust_n,
            tsfc_kg_n_s
        );

    print_result(
        "Fuel mass flow",
        mass_flow_kg_s,
        expected_mass_flow_kg_s,
        1e-12,
        passed
    );
}


/*
 * ============================================================================
 * Invalid-input tests
 * ============================================================================
 */
static void test_invalid_inputs(int *passed)
{
    int invalid_tests_passed = 1;

    if (!isnan(
            propulsion_thrust_available(
                0.0,
                1.0,
                1.0,
                0.0))) {
        printf("FAIL: zero maximum thrust was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_thrust_available(
                -1000.0,
                1.0,
                1.0,
                0.0))) {
        printf("FAIL: negative maximum thrust was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_thrust_available(
                100000.0,
                -0.1,
                1.0,
                0.0))) {
        printf("FAIL: negative throttle was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_thrust_available(
                100000.0,
                1.1,
                1.0,
                0.0))) {
        printf("FAIL: throttle above 1 was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_thrust_available(
                100000.0,
                1.0,
                0.0,
                0.0))) {
        printf("FAIL: zero density ratio was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_thrust_available(
                100000.0,
                1.0,
                1.0,
                -0.1))) {
        printf("FAIL: negative Mach was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_fuel_mass_flow(
                -1000.0,
                0.00002))) {
        printf("FAIL: negative thrust was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_fuel_mass_flow(
                50000.0,
                0.0))) {
        printf("FAIL: zero TSFC was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            propulsion_fuel_mass_flow(
                50000.0,
                -0.00002))) {
        printf("FAIL: negative TSFC was accepted\n");
        invalid_tests_passed = 0;
    }

    printf(
        "Invalid-input tests: %s\n",
        invalid_tests_passed ? "PASS" : "FAIL"
    );

    if (!invalid_tests_passed) {
        *passed = 0;
    }
}


/*
 * ============================================================================
 * Main test runner
 * ============================================================================
 */
int main(void)
{
    int passed = 1;

    printf("============================================\n");
    printf(" AeroSim - Propulsion Module Tests\n");
    printf("============================================\n\n");

    test_thrust_fraction_sea_level(&passed);
    test_thrust_fraction_mach(&passed);
    test_thrust_fraction_altitude(&passed);

    test_static_thrust(&passed);
    test_throttle(&passed);
    test_combined_thrust_lapse(&passed);

    test_fuel_mass_flow(&passed);

    test_invalid_inputs(&passed);

    printf("\n============================================\n");

    if (passed) {
        printf(" ALL PROPULSION TESTS PASSED\n");
    } else {
        printf(" PROPULSION TESTS FAILED\n");
    }

    printf("============================================\n");

    return passed ? 0 : 1;
}