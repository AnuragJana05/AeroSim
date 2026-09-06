#include "scenario.h"

#include <math.h>
#include <stdio.h>


static int nearly_equal(
    double actual,
    double expected,
    double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}


static void print_value(
    const char *name,
    double value,
    const char *unit)
{
    printf(
        "  %-25s %12.4f %s\n",
        name,
        value,
        unit
    );
}


static void test_complete_scenario(int *passed)
{
    /*
     * Generic jet aircraft.
     *
     * These values are intentionally generic and are NOT intended
     * to reproduce any real aircraft.
     */
    const AeroSimAircraft aircraft = {
        .mass_kg = 10000.0,
        .wing_area_m2 = 50.0,
        .wingspan_m = 20.0,
        .zero_lift_drag_coefficient = 0.02,
        .oswald_efficiency = 0.8,
        .maximum_lift_coefficient = 1.5,
        .maximum_static_thrust_n = 60000.0,
        .tsfc_kg_n_s = 0.00002
    };


    /*
     * Sea-level Mach 0.5, full throttle.
     */
    const AeroSimFlightCondition condition = {
        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 1.0
    };


    AeroSimPerformanceResult result;

    int status =
        aerosim_calculate_scenario(
            &aircraft,
            &condition,
            &result
        );


    if (status != 0) {
        printf(
            "FAIL: scenario returned error code %d\n",
            status
        );

        *passed = 0;
        return;
    }


    printf("\nComplete scenario:\n\n");

    printf("Atmosphere:\n");

    print_value(
        "Temperature",
        result.temperature_k,
        "K"
    );

    print_value(
        "Pressure",
        result.pressure_pa,
        "Pa"
    );

    print_value(
        "Density",
        result.density_kg_m3,
        "kg/m^3"
    );

    print_value(
        "Speed of sound",
        result.speed_of_sound_m_s,
        "m/s"
    );


    printf("\nFlight condition:\n");

    print_value(
        "True airspeed",
        result.true_airspeed_m_s,
        "m/s"
    );

    print_value(
        "Dynamic pressure",
        result.dynamic_pressure_pa,
        "Pa"
    );


    printf("\nAircraft:\n");

    print_value(
        "Weight",
        result.weight_n,
        "N"
    );

    print_value(
        "Aspect ratio",
        result.aspect_ratio,
        "-"
    );


    printf("\nAerodynamics:\n");

    print_value(
        "Lift coefficient",
        result.lift_coefficient,
        "-"
    );

    print_value(
        "Drag coefficient",
        result.drag_coefficient,
        "-"
    );

    print_value(
        "Lift",
        result.lift_n,
        "N"
    );

    print_value(
        "Drag",
        result.drag_n,
        "N"
    );

    print_value(
        "L/D",
        result.lift_to_drag_ratio,
        "-"
    );


    printf("\nPropulsion:\n");

    print_value(
        "Available thrust",
        result.thrust_available_n,
        "N"
    );

    print_value(
        "Fuel mass flow",
        result.fuel_mass_flow_kg_s,
        "kg/s"
    );


    printf("\nPerformance:\n");

    print_value(
        "Excess thrust",
        result.excess_thrust_n,
        "N"
    );

    print_value(
        "Excess power",
        result.excess_power_w,
        "W"
    );

    print_value(
        "Rate of climb",
        result.rate_of_climb_m_s,
        "m/s"
    );


    /*
     * Basic physical consistency checks.
     */

    /*
     * Level-flight model requires:
     *
     *     L approximately equals W
     */
    if (!nearly_equal(
            result.lift_n,
            result.weight_n,
            1.0)) {

        printf("FAIL: Lift does not equal weight\n");
        *passed = 0;
    }


    /*
     * Full throttle should produce positive thrust.
     */
    if (result.thrust_available_n <= 0.0) {
        printf("FAIL: thrust is not positive\n");
        *passed = 0;
    }


    /*
     * Drag must be positive.
     */
    if (result.drag_n <= 0.0) {
        printf("FAIL: drag is not positive\n");
        *passed = 0;
    }


    /*
     * Fuel flow must be positive when thrust is positive.
     */
    if (result.fuel_mass_flow_kg_s <= 0.0) {
        printf("FAIL: fuel flow is not positive\n");
        *passed = 0;
    }


    /*
     * The generic aircraft should have positive excess thrust
     * under this particular test condition.
     */
    if (result.excess_thrust_n <= 0.0) {
        printf("FAIL: expected positive excess thrust\n");
        *passed = 0;
    }


    /*
     * Positive excess thrust at positive speed implies positive
     * excess power and rate of climb.
     */
    if (result.excess_power_w <= 0.0) {
        printf("FAIL: expected positive excess power\n");
        *passed = 0;
    }


    if (result.rate_of_climb_m_s <= 0.0) {
        printf("FAIL: expected positive rate of climb\n");
        *passed = 0;
    }


    if (*passed) {
        printf("\nPASS: Complete scenario consistency\n");
    }
}


static void test_throttle_trend(int *passed)
{
    const AeroSimAircraft aircraft = {
        .mass_kg = 10000.0,
        .wing_area_m2 = 50.0,
        .wingspan_m = 20.0,
        .zero_lift_drag_coefficient = 0.02,
        .oswald_efficiency = 0.8,
        .maximum_lift_coefficient = 1.5,
        .maximum_static_thrust_n = 60000.0,
        .tsfc_kg_n_s = 0.00002
    };


    const AeroSimFlightCondition low_throttle = {
        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 0.5
    };


    const AeroSimFlightCondition high_throttle = {
        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 1.0
    };


    AeroSimPerformanceResult low_result;
    AeroSimPerformanceResult high_result;


    int low_status =
        aerosim_calculate_scenario(
            &aircraft,
            &low_throttle,
            &low_result
        );


    int high_status =
        aerosim_calculate_scenario(
            &aircraft,
            &high_throttle,
            &high_result
        );


    if (low_status != 0 ||
        high_status != 0) {

        printf("FAIL: throttle trend scenario calculation\n");
        *passed = 0;
        return;
    }


    /*
     * Increasing throttle should increase thrust.
     */
    if (high_result.thrust_available_n
        <= low_result.thrust_available_n) {

        printf("FAIL: throttle does not increase thrust\n");
        *passed = 0;
        return;
    }


    /*
     * With the same flight condition, increasing thrust should
     * increase excess thrust and rate of climb.
     */
    if (high_result.excess_thrust_n
        <= low_result.excess_thrust_n) {

        printf("FAIL: throttle does not increase excess thrust\n");
        *passed = 0;
        return;
    }


    if (high_result.rate_of_climb_m_s
        <= low_result.rate_of_climb_m_s) {

        printf("FAIL: throttle does not increase climb rate\n");
        *passed = 0;
        return;
    }


    printf("PASS: Throttle performance trend\n");
}


static void test_invalid_inputs(int *passed)
{
    const AeroSimAircraft aircraft = {
        .mass_kg = 10000.0,
        .wing_area_m2 = 50.0,
        .wingspan_m = 20.0,
        .zero_lift_drag_coefficient = 0.02,
        .oswald_efficiency = 0.8,
        .maximum_lift_coefficient = 1.5,
        .maximum_static_thrust_n = 60000.0,
        .tsfc_kg_n_s = 0.00002
    };


    AeroSimPerformanceResult result;


    const AeroSimFlightCondition invalid_throttle = {
        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 1.5
    };


    const AeroSimFlightCondition invalid_altitude = {
        .altitude_m = -100.0,
        .mach = 0.5,
        .throttle = 1.0
    };


    const AeroSimFlightCondition zero_mach = {
        .altitude_m = 0.0,
        .mach = 0.0,
        .throttle = 1.0
    };


    if (aerosim_calculate_scenario(
            &aircraft,
            &invalid_throttle,
            &result) == 0) {

        printf("FAIL: invalid throttle accepted\n");
        *passed = 0;
    }


    if (aerosim_calculate_scenario(
            &aircraft,
            &invalid_altitude,
            &result) == 0) {

        printf("FAIL: invalid altitude accepted\n");
        *passed = 0;
    }


    if (aerosim_calculate_scenario(
            &aircraft,
            &zero_mach,
            &result) == 0) {

        printf("FAIL: zero Mach accepted for level-flight scenario\n");
        *passed = 0;
    }


    if (*passed) {
        printf("PASS: Scenario invalid-input handling\n");
    }
}


int main(void)
{
    int passed = 1;

    printf("============================================\n");
    printf(" AeroSim - Complete Scenario Tests\n");
    printf("============================================\n");

    test_complete_scenario(&passed);
    test_throttle_trend(&passed);
    test_invalid_inputs(&passed);

    printf("\n============================================\n");

    if (passed) {
        printf(" ALL SCENARIO TESTS PASSED\n");
    } else {
        printf(" SCENARIO TESTS FAILED\n");
    }

    printf("============================================\n");

    return passed ? 0 : 1;
}