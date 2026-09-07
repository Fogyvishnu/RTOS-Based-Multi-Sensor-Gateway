/**
 * @file test_sensor_fusion.c
 * @brief Unit tests for sensor fusion math, Complementary filter, EWMA, and environmental formulas
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include "sensor_fusion.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
            exit(1); \
        } \
    } while (0)

#define EPSILON 0.05f

static bool float_near(float a, float b, float tol) {
    return fabsf(a - b) <= tol;
}

void test_initialization(void) {
    printf("[RUN] test_initialization\n");
    Attitude_t att;
    sensor_fusion_init(&att);
    TEST_ASSERT(att.roll_deg == 0.0f, "Roll must be 0.0 at init");
    TEST_ASSERT(att.pitch_deg == 0.0f, "Pitch must be 0.0 at init");
    printf("[PASS] test_initialization\n");
}

void test_accel_level(void) {
    printf("[RUN] test_accel_level\n");
    Attitude_t att;
    sensor_fusion_init(&att);

    /* Flat surface: X=0g, Y=0g, Z=1.0g, no rotation */
    float accel[3] = {0.0f, 0.0f, 1.0f};
    float gyro[3]  = {0.0f, 0.0f, 0.0f};

    sensor_fusion_update(&att, accel, gyro, 0.01f, 0.98f);
    TEST_ASSERT(float_near(att.roll_deg, 0.0f, EPSILON), "Level roll must be ~0 deg");
    TEST_ASSERT(float_near(att.pitch_deg, 0.0f, EPSILON), "Level pitch must be ~0 deg");
    printf("[PASS] test_accel_level\n");
}

void test_accel_roll_45_deg(void) {
    printf("[RUN] test_accel_roll_45_deg\n");
    Attitude_t att;
    sensor_fusion_init(&att);

    /* 45 degree roll: Y=0.707g, Z=0.707g, X=0 */
    float accel[3] = {0.0f, 0.7071f, 0.7071f};
    float gyro[3]  = {0.0f, 0.0f, 0.0f};

    sensor_fusion_update(&att, accel, gyro, 0.01f, 0.98f);
    TEST_ASSERT(float_near(att.roll_deg, 45.0f, 0.5f), "Roll should be ~45 deg");
    TEST_ASSERT(float_near(att.pitch_deg, 0.0f, 0.5f), "Pitch should be ~0 deg");
    printf("[PASS] test_accel_roll_45_deg\n");
}

void test_gyro_integration(void) {
    printf("[RUN] test_gyro_integration\n");
    Attitude_t att;
    sensor_fusion_init(&att);

    float accel[3] = {0.0f, 0.0f, 1.0f};
    float gyro[3]  = {100.0f, 0.0f, 0.0f}; /* 100 deg/s roll rate */

    /* First step initializes */
    sensor_fusion_update(&att, accel, gyro, 0.01f, 1.0f);
    /* Second step with alpha=1.0 (pure gyro integration): delta = 100 * 0.01 = 1.0 deg */
    sensor_fusion_update(&att, accel, gyro, 0.01f, 1.0f);
    TEST_ASSERT(float_near(att.roll_deg, 1.0f, 0.01f), "Pure gyro integration step should increase roll by 1.0 deg");
    printf("[PASS] test_gyro_integration\n");
}

void test_dew_point(void) {
    printf("[RUN] test_dew_point\n");
    /* Standard test vector: T=20°C, RH=50% -> Dew point ~ 9.3°C */
    float dp = sensor_fusion_dew_point(20.0f, 50.0f);
    TEST_ASSERT(float_near(dp, 9.3f, 0.2f), "Dew point at 20C/50% should be ~9.3C");
    printf("[PASS] test_dew_point (Calculated: %.2f C)\n", dp);
}

void test_heat_index(void) {
    printf("[RUN] test_heat_index\n");
    /* Standard test vector: T=30°C (86°F), RH=70% -> HI ~ 35.2°C */
    float hi = sensor_fusion_heat_index(30.0f, 70.0f);
    TEST_ASSERT(hi > 30.0f && hi < 40.0f, "Heat index should reflect feels-like > 30C");
    printf("[PASS] test_heat_index (Calculated: %.2f C)\n", hi);
}

void test_ewma_filter(void) {
    printf("[RUN] test_ewma_filter\n");
    float filtered = 0.0f;
    float beta = 0.2f;

    /* Step response from 0 to 100 */
    filtered = sensor_fusion_ewma(filtered, 100.0f, beta);
    TEST_ASSERT(float_near(filtered, 20.0f, 0.01f), "First EWMA step should be 20.0");

    filtered = sensor_fusion_ewma(filtered, 100.0f, beta);
    TEST_ASSERT(float_near(filtered, 36.0f, 0.01f), "Second EWMA step should be 36.0");
    printf("[PASS] test_ewma_filter\n");
}

int main(void) {
    printf("\n=== RUNNING SENSOR FUSION UNIT TESTS ===\n");
    test_initialization();
    test_accel_level();
    test_accel_roll_45_deg();
    test_gyro_integration();
    test_dew_point();
    test_heat_index();
    test_ewma_filter();
    printf("=== ALL SENSOR FUSION TESTS PASSED! ===\n\n");
    return 0;
}
