#include "performance.h"

#include "aerodynamics.h"

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
 * Excess thrust
 * ============================================================================
 */
static void test_excess_thrust(int *passed)
{
    double result =
        performance_excess_thrust(
            50000.0,
            30000.0
        );

    print_result(
        "Excess thrust",
        result,
        20000.0,
        1e-9,
        passed
    );
}


/*
 * ============================================================================
 * Excess power
 * ============================================================================
 *
 * P = (T-D)V
 *
 * = 20,000 * 200
 * = 4,000,000 W
 */
static void test_excess_power(int *passed)
{
    double result =
        performance_excess_power(
            50000.0,
            30000.0,
            200.0
        );

    print_result(
        "Excess power",
        result,
        4000000.0,
        1e-6,
        passed
    );
}


/*
 * ============================================================================
 * Rate of climb
 * ============================================================================
 *
 * P_excess = 4,000,000 W
 *
 * m = 10,000 kg
 *
 * W = 10,000 * 9.80665
 *
 * ROC = P/W
 */
static void test_rate_of_climb(int *passed)
{
    const double expected =
        4000000.0
        / (10000.0 * 9.80665);

    double result =
        performance_rate_of_climb(
            50000.0,
            30000.0,
            200.0,
            10000.0
        );

    print_result(
        "Rate of climb",
        result,
        expected,
        1e-10,
        passed
    );
}


/*
 * ============================================================================
 * Stall speed
 * ============================================================================
 *
 * Generic aircraft:
 *
 * mass = 10,000 kg
 * rho = 1.225 kg/m^3
 * S = 50 m^2
 * CLmax = 1.5
 */
static void test_stall_speed(int *passed)
{
    const double expected =
        sqrt(
            (2.0 * 10000.0 * 9.80665)
            / (1.225 * 50.0 * 1.5)
        );

    double result =
        performance_stall_speed(
            10000.0,
            1.225,
            50.0,
            1.5
        );

    print_result(
        "1-g stall speed",
        result,
        expected,
        1e-10,
        passed
    );

    /*
     * This generic aircraft should have a stall speed
     * in a reasonable light/medium aircraft performance range.
     */
    printf(
        "       Stall speed = %.2f m/s (%.2f km/h)\n",
        result,
        result * 3.6
    );
}


/*
 * ============================================================================
 * Maximum level-flight speed
 * ============================================================================
 *
 * Generic jet:
 *
 * mass = 10,000 kg
 * rho = 1.225 kg/m^3
 * a = 340.292 m/s
 * S = 50 m^2
 * CD0 = 0.02
 * e = 0.8
 * AR = 8
 * maximum thrust = 60,000 N
 * throttle = 1
 *
 * Search:
 *
 * 50 m/s -> 500 m/s
 *
 * The solver should find a high-speed intersection between
 * available thrust and aerodynamic drag.
 */
static void test_max_level_speed(int *passed)
{
    const double speed_of_sound_m_s = 340.292286;

    double result =
        performance_max_level_speed(
            10000.0,
            1.225,
            speed_of_sound_m_s,
            50.0,
            0.02,
            0.8,
            8.0,
            60000.0,
            1.0,
            50.0,
            500.0,
            1000
        );

    if (isfinite(result) &&
        result > 100.0 &&
        result < 500.0) {

        printf(
            "PASS: %-35s Vmax = %.8f m/s\n",
            "Maximum level-flight speed",
            result
        );

    } else {
        printf(
            "FAIL: Maximum level-flight speed = %.8f m/s\n",
            result
        );

        *passed = 0;
    }
}


/*
 * ============================================================================
 * Analytical jet range
 * ============================================================================
 *
 * Example:
 *
 * V = 250 m/s
 * L/D = 12
 * TSFC = 0.00002 kg/(N*s)
 * initial mass = 10,000 kg
 * final mass = 8,000 kg
 *
 * Expected range is roughly 1,037 km.
 */
static void test_jet_range(int *passed)
{
    const double velocity_m_s = 250.0;
    const double lift_to_drag_ratio = 12.0;
    const double tsfc_kg_n_s = 0.00002;
    const double initial_mass_kg = 10000.0;
    const double final_mass_kg = 8000.0;

    const double expected =
        velocity_m_s
        / (9.80665 * tsfc_kg_n_s)
        * lift_to_drag_ratio
        * log(initial_mass_kg / final_mass_kg);

    double result =
        performance_jet_range(
            velocity_m_s,
            lift_to_drag_ratio,
            tsfc_kg_n_s,
            initial_mass_kg,
            final_mass_kg
        );

    print_result(
        "Analytical jet range",
        result,
        expected,
        1e-6,
        passed
    );

    printf(
        "       Range = %.2f km\n",
        result / 1000.0
    );
}


/*
 * ============================================================================
 * Analytical jet endurance
 * ============================================================================
 */
static void test_jet_endurance(int *passed)
{
    const double lift_to_drag_ratio = 12.0;
    const double tsfc_kg_n_s = 0.00002;
    const double initial_mass_kg = 10000.0;
    const double final_mass_kg = 8000.0;

    const double expected =
        1.0
        / (9.80665 * tsfc_kg_n_s)
        * lift_to_drag_ratio
        * log(initial_mass_kg / final_mass_kg);

    double result =
        performance_jet_endurance(
            lift_to_drag_ratio,
            tsfc_kg_n_s,
            initial_mass_kg,
            final_mass_kg
        );

    print_result(
        "Analytical jet endurance",
        result,
        expected,
        1e-10,
        passed
    );

    printf(
        "       Endurance = %.2f minutes\n",
        result / 60.0
    );
}


/*
 * ============================================================================
 * Trend tests
 * ============================================================================
 */
static void test_physical_trends(int *passed)
{
    double lower_mass_stall;
    double higher_mass_stall;

    double lower_ld_range;
    double higher_ld_range;

    /*
     * Heavier aircraft should stall faster.
     */
    lower_mass_stall =
        performance_stall_speed(
            8000.0,
            1.225,
            50.0,
            1.5
        );

    higher_mass_stall =
        performance_stall_speed(
            12000.0,
            1.225,
            50.0,
            1.5
        );

    if (higher_mass_stall > lower_mass_stall) {
        printf(
            "PASS: Stall-speed mass trend "
            "(%.2f -> %.2f m/s)\n",
            lower_mass_stall,
            higher_mass_stall
        );
    } else {
        printf("FAIL: Stall-speed mass trend\n");
        *passed = 0;
    }

    /*
     * Higher L/D should produce greater Breguet range,
     * assuming all other variables remain constant.
     */
    lower_ld_range =
        performance_jet_range(
            250.0,
            10.0,
            0.00002,
            10000.0,
            8000.0
        );

    higher_ld_range =
        performance_jet_range(
            250.0,
            15.0,
            0.00002,
            10000.0,
            8000.0
        );

    if (higher_ld_range > lower_ld_range) {
        printf(
            "PASS: L/D range trend "
            "(%.2f -> %.2f km)\n",
            lower_ld_range / 1000.0,
            higher_ld_range / 1000.0
        );
    } else {
        printf("FAIL: L/D range trend\n");
        *passed = 0;
    }
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
            performance_excess_thrust(
                -1.0,
                1000.0))) {
        printf("FAIL: negative thrust accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            performance_excess_power(
                10000.0,
                5000.0,
                0.0))) {
        printf("FAIL: zero velocity accepted for power\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            performance_rate_of_climb(
                10000.0,
                5000.0,
                100.0,
                0.0))) {
        printf("FAIL: zero mass accepted for climb rate\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            performance_stall_speed(
                10000.0,
                1.225,
                50.0,
                0.0))) {
        printf("FAIL: zero CLmax accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            performance_jet_range(
                250.0,
                12.0,
                0.00002,
                8000.0,
                8000.0))) {
        printf("FAIL: equal initial/final mass accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            performance_jet_endurance(
                12.0,
                0.00002,
                8000.0,
                9000.0))) {
        printf("FAIL: final mass greater than initial mass accepted\n");
        invalid_tests_passed = 0;
    }

    if (!invalid_tests_passed) {
        *passed = 0;
    }

    printf(
        "Invalid-input tests: %s\n",
        invalid_tests_passed ? "PASS" : "FAIL"
    );
}


/*
 * ============================================================================
 * Main
 * ============================================================================
 */
int main(void)
{
    int passed = 1;

    printf("============================================\n");
    printf(" AeroSim - Performance Module Tests\n");
    printf("============================================\n\n");

    test_excess_thrust(&passed);
    test_excess_power(&passed);
    test_rate_of_climb(&passed);
    test_stall_speed(&passed);
    test_max_level_speed(&passed);
    test_jet_range(&passed);
    test_jet_endurance(&passed);
    test_physical_trends(&passed);
    test_invalid_inputs(&passed);

    printf("\n============================================\n");

    if (passed) {
        printf(" ALL PERFORMANCE TESTS PASSED\n");
    } else {
        printf(" PERFORMANCE TESTS FAILED\n");
    }

    printf("============================================\n");

    return passed ? 0 : 1;
}