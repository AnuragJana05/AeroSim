#include "mission.h"

#include <math.h>
#include <stdio.h>


static void test_cruise_mission(int *passed)
{
    /*
     * Generic jet aircraft.
     *
     * These parameters are intentionally generic.
     */
    const AeroSimMissionConfig config = {
        .aircraft = {
            .mass_kg = 10000.0,
            .wing_area_m2 = 50.0,
            .wingspan_m = 20.0,
            .zero_lift_drag_coefficient = 0.02,
            .oswald_efficiency = 0.8,
            .maximum_lift_coefficient = 1.5,
            .maximum_static_thrust_n = 60000.0,
            .tsfc_kg_n_s = 0.00002
        },

        .initial_mass_kg = 10000.0,
        .dry_mass_kg = 9000.0,

        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 0.8,

        .timestep_s = 1.0,
        .maximum_time_s = 3600.0
    };


    /*
     * One hour at one-second resolution.
     */
    AeroSimMissionSample samples[3601];

    AeroSimMissionResult result;


    int status =
        aerosim_run_mission(
            &config,
            samples,
            3601U,
            &result
        );


    if (status != 0) {
        printf(
            "FAIL: mission returned error code %d\n",
            status
        );

        *passed = 0;
        return;
    }


    printf("\nMission result:\n\n");

    printf(
        "  Final time          : %.2f s\n",
        result.final_time_s
    );

    printf(
        "  Final distance      : %.2f km\n",
        result.final_distance_m / 1000.0
    );

    printf(
        "  Initial mass        : %.2f kg\n",
        result.initial_mass_kg
    );

    printf(
        "  Final mass          : %.2f kg\n",
        result.final_mass_kg
    );

    printf(
        "  Initial fuel        : %.2f kg\n",
        result.initial_fuel_kg
    );

    printf(
        "  Fuel burned         : %.2f kg\n",
        result.fuel_burned_kg
    );

    printf(
        "  Remaining fuel      : %.2f kg\n",
        result.remaining_fuel_kg
    );

    printf(
        "  Samples             : %zu\n",
        result.sample_count
    );


    /*
     * Basic physical checks.
     */
    if (result.final_mass_kg <
        config.dry_mass_kg) {

        printf("FAIL: final mass below dry mass\n");
        *passed = 0;
    }


    if (result.fuel_burned_kg < 0.0 ||
        result.fuel_burned_kg >
        result.initial_fuel_kg) {

        printf("FAIL: invalid fuel consumption\n");
        *passed = 0;
    }


    if (result.final_distance_m <= 0.0) {
        printf("FAIL: distance did not increase\n");
        *passed = 0;
    }


    if (result.sample_count < 2U) {
        printf("FAIL: insufficient mission samples\n");
        *passed = 0;
    }


    if (*passed) {
        printf("\nPASS: Cruise mission simulation\n");
    }
}


/*
 * ============================================================================
 * Verify mass decreases monotonically.
 * ============================================================================
 */
static void test_mass_decreases(
    const AeroSimMissionConfig *config,
    int *passed)
{
    AeroSimMissionSample samples[601];
    AeroSimMissionResult result;

    int status =
        aerosim_run_mission(
            config,
            samples,
            601U,
            &result
        );


    if (status != 0) {
        printf("FAIL: mass trend mission failed\n");
        *passed = 0;
        return;
    }


    for (size_t i = 1U;
         i < result.sample_count;
         ++i) {

        if (samples[i].aircraft_mass_kg
            > samples[i - 1U].aircraft_mass_kg) {

            printf(
                "FAIL: aircraft mass increased at sample %zu\n",
                i
            );

            *passed = 0;
            return;
        }
    }


    printf("PASS: Aircraft mass decreases monotonically\n");
}


/*
 * ============================================================================
 * Verify fuel decreases monotonically.
 * ============================================================================
 */
static void test_fuel_decreases(
    const AeroSimMissionConfig *config,
    int *passed)
{
    AeroSimMissionSample samples[601];
    AeroSimMissionResult result;

    int status =
        aerosim_run_mission(
            config,
            samples,
            601U,
            &result
        );


    if (status != 0) {
        printf("FAIL: fuel trend mission failed\n");
        *passed = 0;
        return;
    }


    for (size_t i = 1U;
         i < result.sample_count;
         ++i) {

        if (samples[i].fuel_remaining_kg
            > samples[i - 1U].fuel_remaining_kg) {

            printf(
                "FAIL: fuel increased at sample %zu\n",
                i
            );

            *passed = 0;
            return;
        }
    }


    printf("PASS: Fuel decreases monotonically\n");
}


/*
 * ============================================================================
 * Invalid-input tests
 * ============================================================================
 */
static void test_invalid_inputs(int *passed)
{
    AeroSimMissionSample samples[10];
    AeroSimMissionResult result;


    const AeroSimMissionConfig invalid_config = {
        .aircraft = {
            .mass_kg = 10000.0,
            .wing_area_m2 = 50.0,
            .wingspan_m = 20.0,
            .zero_lift_drag_coefficient = 0.02,
            .oswald_efficiency = 0.8,
            .maximum_lift_coefficient = 1.5,
            .maximum_static_thrust_n = 60000.0,
            .tsfc_kg_n_s = 0.00002
        },

        .initial_mass_kg = 8000.0,
        .dry_mass_kg = 9000.0,

        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 0.8,

        .timestep_s = 1.0,
        .maximum_time_s = 100.0
    };


    if (aerosim_run_mission(
            &invalid_config,
            samples,
            10U,
            &result) == 0) {

        printf(
            "FAIL: dry mass greater than initial mass accepted\n"
        );

        *passed = 0;
    }


    if (*passed) {
        printf("PASS: Mission invalid-input handling\n");
    }
}


/*
 * ============================================================================
 * Main
 * ============================================================================
 */
int main(void)
{
    int passed = 1;


    const AeroSimMissionConfig config = {
        .aircraft = {
            .mass_kg = 10000.0,
            .wing_area_m2 = 50.0,
            .wingspan_m = 20.0,
            .zero_lift_drag_coefficient = 0.02,
            .oswald_efficiency = 0.8,
            .maximum_lift_coefficient = 1.5,
            .maximum_static_thrust_n = 60000.0,
            .tsfc_kg_n_s = 0.00002
        },

        .initial_mass_kg = 10000.0,
        .dry_mass_kg = 9000.0,

        .altitude_m = 0.0,
        .mach = 0.5,
        .throttle = 0.8,

        .timestep_s = 1.0,
        .maximum_time_s = 600.0
    };


    printf("============================================\n");
    printf(" AeroSim - Mission Module Tests\n");
    printf("============================================\n");


    test_cruise_mission(&passed);

    test_mass_decreases(
        &config,
        &passed
    );

    test_fuel_decreases(
        &config,
        &passed
    );

    test_invalid_inputs(&passed);


    printf("\n============================================\n");


    if (passed) {
        printf(" ALL MISSION TESTS PASSED\n");
    } else {
        printf(" MISSION TESTS FAILED\n");
    }


    printf("============================================\n");


    return passed ? 0 : 1;
}