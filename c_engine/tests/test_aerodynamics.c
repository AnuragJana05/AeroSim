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
        printf("PASS: %-35s actual = %.8f\n", name, actual);
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
 * Weight test
 * ============================================================================
 */
static void test_weight(int *passed)
{
    const double mass_kg = 10000.0;
    const double expected_weight_n = 98066.5;

    double weight_n =
        aero_weight_from_mass(mass_kg);

    print_result(
        "Weight from mass",
        weight_n,
        expected_weight_n,
        0.001,
        passed
    );
}


/*
 * ============================================================================
 * Dynamic pressure test
 * ============================================================================
 *
 * rho = 1.225 kg/m^3
 * V   = 100 m/s
 *
 * q = 0.5 * 1.225 * 100^2
 *   = 6125 Pa
 */
static void test_dynamic_pressure(int *passed)
{
    const double density_kg_m3 = 1.225;
    const double velocity_m_s = 100.0;
    const double expected_q_pa = 6125.0;

    double q_pa =
        aero_dynamic_pressure(
            density_kg_m3,
            velocity_m_s
        );

    print_result(
        "Dynamic pressure",
        q_pa,
        expected_q_pa,
        0.001,
        passed
    );
}


/*
 * ============================================================================
 * Lift coefficient test
 * ============================================================================
 *
 * Example aircraft:
 *
 * mass = 10,000 kg
 * rho  = 1.225 kg/m^3
 * V    = 100 m/s
 * S    = 50 m^2
 *
 * W = m*g
 *
 * CL = W/(q*S)
 */
static void test_lift_coefficient(int *passed)
{
    const double mass_kg = 10000.0;
    const double density_kg_m3 = 1.225;
    const double velocity_m_s = 100.0;
    const double wing_area_m2 = 50.0;

    const double expected_cl =
        98066.5 / (6125.0 * 50.0);

    double cl =
        aero_lift_coefficient_level_flight(
            mass_kg,
            density_kg_m3,
            velocity_m_s,
            wing_area_m2
        );

    print_result(
        "Level-flight lift coefficient",
        cl,
        expected_cl,
        1e-10,
        passed
    );
}


/*
 * ============================================================================
 * Induced drag factor test
 * ============================================================================
 *
 * e  = 0.8
 * AR = 8
 *
 * k = 1/(pi*e*AR)
 */
static void test_induced_drag_factor(int *passed)
{
    const double oswald_efficiency = 0.8;
    const double aspect_ratio = 8.0;

    const double expected_k =
        1.0 / (3.14159265358979323846
             * oswald_efficiency
             * aspect_ratio);

    double k =
        aero_induced_drag_factor(
            oswald_efficiency,
            aspect_ratio
        );

    print_result(
        "Induced drag factor",
        k,
        expected_k,
        1e-12,
        passed
    );
}


/*
 * ============================================================================
 * Drag coefficient test
 * ============================================================================
 *
 * CD0 = 0.02
 * k   = 0.05
 * CL  = 0.5
 *
 * CD = 0.02 + 0.05*(0.5)^2
 *    = 0.0325
 */
static void test_drag_coefficient(int *passed)
{
    const double cd0 = 0.02;
    const double k = 0.05;
    const double cl = 0.5;
    const double expected_cd = 0.0325;

    double cd =
        aero_drag_coefficient(
            cd0,
            k,
            cl
        );

    print_result(
        "Parabolic drag coefficient",
        cd,
        expected_cd,
        1e-12,
        passed
    );
}


/*
 * ============================================================================
 * Lift test
 * ============================================================================
 *
 * q  = 6125 Pa
 * S  = 50 m^2
 * CL = 0.32
 *
 * L = q*S*CL
 *   = 98,000 N
 */
static void test_lift(int *passed)
{
    const double q_pa = 6125.0;
    const double wing_area_m2 = 50.0;
    const double cl = 0.32;
    const double expected_lift_n = 98000.0;

    double lift_n =
        aero_lift(
            q_pa,
            wing_area_m2,
            cl
        );

    print_result(
        "Lift",
        lift_n,
        expected_lift_n,
        1e-6,
        passed
    );
}


/*
 * ============================================================================
 * Drag test
 * ============================================================================
 *
 * q  = 6125 Pa
 * S  = 50 m^2
 * CD = 0.0325
 *
 * D = q*S*CD
 *   = 9945.3125 N
 */
static void test_drag(int *passed)
{
    const double q_pa = 6125.0;
    const double wing_area_m2 = 50.0;
    const double cd = 0.0325;
    const double expected_drag_n = 9953.125;

    double drag_n =
        aero_drag(
            q_pa,
            wing_area_m2,
            cd
        );

    print_result(
        "Drag",
        drag_n,
        expected_drag_n,
        1e-6,
        passed
    );
}


/*
 * ============================================================================
 * Lift-to-drag ratio test
 * ============================================================================
 */
static void test_lift_to_drag_ratio(int *passed)
{
    const double lift_n = 98000.0;
    const double drag_n = 9945.3125;

    const double expected_ratio =
        lift_n / drag_n;

    double ratio =
        aero_lift_to_drag_ratio(
            lift_n,
            drag_n
        );

    print_result(
        "Lift-to-drag ratio",
        ratio,
        expected_ratio,
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

    if (!isnan(aero_weight_from_mass(0.0))) {
        printf("FAIL: zero mass was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(aero_weight_from_mass(-100.0))) {
        printf("FAIL: negative mass was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(aero_dynamic_pressure(1.225, 0.0))) {
        printf("FAIL: zero velocity was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(aero_dynamic_pressure(1.225, -100.0))) {
        printf("FAIL: negative velocity was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(aero_dynamic_pressure(-1.0, 100.0))) {
        printf("FAIL: negative density was accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            aero_lift_coefficient_level_flight(
                10000.0,
                1.225,
                0.0,
                50.0))) {
        printf("FAIL: zero velocity accepted for CL\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            aero_induced_drag_factor(
                0.0,
                8.0))) {
        printf("FAIL: zero Oswald efficiency accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            aero_induced_drag_factor(
                0.8,
                0.0))) {
        printf("FAIL: zero aspect ratio accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            aero_drag_coefficient(
                -0.01,
                0.05,
                0.5))) {
        printf("FAIL: negative CD0 accepted\n");
        invalid_tests_passed = 0;
    }

    if (!isnan(
            aero_lift_to_drag_ratio(
                100000.0,
                0.0))) {
        printf("FAIL: zero drag accepted for L/D\n");
        invalid_tests_passed = 0;
    }

    printf(
        "Invalid-input tests: %s\n",
        invalid_tests_passed ? "PASS" : "FAIL"
    );

    /*
     * Only mark the overall test suite as failed if one of the
     * invalid-input tests actually failed.
     */
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
    printf(" AeroSim - Aerodynamics Module Tests\n");
    printf("============================================\n\n");

    test_weight(&passed);
    test_dynamic_pressure(&passed);
    test_lift_coefficient(&passed);
    test_induced_drag_factor(&passed);
    test_drag_coefficient(&passed);
    test_lift(&passed);
    test_drag(&passed);
    test_lift_to_drag_ratio(&passed);
    test_invalid_inputs(&passed);

    printf("\n============================================\n");

    if (passed) {
        printf(" ALL AERODYNAMICS TESTS PASSED\n");
    } else {
        printf(" AERODYNAMICS TESTS FAILED\n");
    }

    printf("============================================\n");

    return passed ? 0 : 1;
}