/**
 * @file test_fault_manager.c
 * @brief Unit tests for system state machine, fault tracking, and fault injection
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "fault_manager.h"
#include "app_config.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
            exit(1); \
        } \
    } while (0)

void test_initial_state(void) {
    printf("[RUN] test_initial_state\n");
    fault_manager_init();
    TEST_ASSERT(fault_manager_get_all() == FAULT_NONE, "Initial fault mask must be FAULT_NONE");
    TEST_ASSERT(fault_manager_evaluate_state() == SYSTEM_STATE_NORMAL, "Initial state evaluate should be NORMAL");
    printf("[PASS] test_initial_state\n");
}

void test_degraded_state(void) {
    printf("[RUN] test_degraded_state\n");
    fault_manager_init();

    /* Inject MPU6050 consecutive errors */
    for (int i = 0; i < MAX_CONSECUTIVE_ERRORS; i++) {
        fault_manager_set(FAULT_MPU6050_COMM_TIMEOUT);
    }

    SystemState_t state = fault_manager_evaluate_state();
    TEST_ASSERT(state == SYSTEM_STATE_DEGRADED, "System state must transition to DEGRADED on single sensor failure");
    printf("[PASS] test_degraded_state\n");
}

void test_fault_recovery(void) {
    printf("[RUN] test_fault_recovery\n");
    fault_manager_init();

    fault_manager_set(FAULT_DHT11_CHECKSUM);
    TEST_ASSERT(fault_manager_is_active(FAULT_DHT11_CHECKSUM), "Fault must be active");

    fault_manager_clear(FAULT_DHT11_CHECKSUM);
    TEST_ASSERT(!fault_manager_is_active(FAULT_DHT11_CHECKSUM), "Fault must be cleared");
    TEST_ASSERT(fault_manager_evaluate_state() == SYSTEM_STATE_NORMAL, "System state must recover to NORMAL");
    printf("[PASS] test_fault_recovery\n");
}

void test_critical_failure(void) {
    printf("[RUN] test_critical_failure\n");
    fault_manager_init();

    /* Task deadline miss (watchdog starved) -> Critical */
    fault_manager_set(FAULT_TASK_DEADLINE_MISS);
    SystemState_t state = fault_manager_evaluate_state();
    TEST_ASSERT(state == SYSTEM_STATE_CRITICAL, "Task deadline miss must trigger CRITICAL state");

    fault_manager_clear(FAULT_TASK_DEADLINE_MISS);

    /* I2C bus lockup -> Critical */
    fault_manager_set(FAULT_I2C_BUS_STUCK);
    state = fault_manager_evaluate_state();
    TEST_ASSERT(state == SYSTEM_STATE_CRITICAL, "I2C bus stuck must trigger CRITICAL state");
    printf("[PASS] test_critical_failure\n");
}

void test_fault_injection_engine(void) {
    printf("[RUN] test_fault_injection_engine\n");
    fault_manager_init();

    fault_inject_enable(FAULT_MPU6050_WHO_AM_I);
    TEST_ASSERT(fault_inject_is_active(FAULT_MPU6050_WHO_AM_I), "Injected fault must be active");
    TEST_ASSERT(fault_manager_is_active(FAULT_MPU6050_WHO_AM_I), "Fault manager must report injected fault");

    fault_inject_disable(FAULT_MPU6050_WHO_AM_I);
    TEST_ASSERT(!fault_inject_is_active(FAULT_MPU6050_WHO_AM_I), "Injected fault must be inactive");

    fault_inject_enable(FAULT_DHT11_TIMEOUT);
    fault_inject_enable(FAULT_ADC_OUT_OF_BOUNDS);
    TEST_ASSERT(fault_manager_get_all() == (FAULT_DHT11_TIMEOUT | FAULT_ADC_OUT_OF_BOUNDS), "Both injected faults should be in mask");

    fault_inject_clear_all();
    TEST_ASSERT(fault_manager_get_all() == FAULT_NONE, "All injected faults must be cleared");
    printf("[PASS] test_fault_injection_engine\n");
}

void test_i2c_recovery_counter(void) {
    printf("[RUN] test_i2c_recovery_counter\n");
    fault_manager_init();
    TEST_ASSERT(fault_manager_get_i2c_recovery_count() == 0, "Initial counter should be 0");

    fault_manager_increment_i2c_recovery();
    fault_manager_increment_i2c_recovery();
    TEST_ASSERT(fault_manager_get_i2c_recovery_count() == 2, "Counter should be 2");
    printf("[PASS] test_i2c_recovery_counter\n");
}

int main(void) {
    printf("\n=== RUNNING FAULT MANAGER UNIT TESTS ===\n");
    test_initial_state();
    test_degraded_state();
    test_fault_recovery();
    test_critical_failure();
    test_fault_injection_engine();
    test_i2c_recovery_counter();
    printf("=== ALL FAULT MANAGER TESTS PASSED! ===\n\n");
    return 0;
}
