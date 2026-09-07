/**
 * @file fault_manager.c
 * @brief Implementation of system fault detection, states, and fault injection
 */

#include "fault_manager.h"
#include "app_config.h"

static volatile uint16_t s_active_faults = FAULT_NONE;
static volatile uint16_t s_injected_faults = FAULT_NONE;
static volatile uint32_t s_i2c_recovery_count = 0;

static uint8_t s_mpu_error_counter = 0;
static uint8_t s_dht_error_counter = 0;
static uint8_t s_adc_error_counter = 0;

static SystemState_t s_current_state = SYSTEM_STATE_INIT;

void fault_manager_init(void) {
    s_active_faults = FAULT_NONE;
    s_injected_faults = FAULT_NONE;
    s_i2c_recovery_count = 0;
    s_mpu_error_counter = 0;
    s_dht_error_counter = 0;
    s_adc_error_counter = 0;
    s_current_state = SYSTEM_STATE_INIT;
}

void fault_manager_set(FaultFlags_t fault) {
    s_active_faults |= (uint16_t)fault;

    if (fault & (FAULT_MPU6050_COMM_TIMEOUT | FAULT_MPU6050_WHO_AM_I)) {
        if (s_mpu_error_counter < 255) s_mpu_error_counter++;
    }
    if (fault & (FAULT_DHT11_TIMEOUT | FAULT_DHT11_CHECKSUM)) {
        if (s_dht_error_counter < 255) s_dht_error_counter++;
    }
    if (fault & FAULT_ADC_OUT_OF_BOUNDS) {
        if (s_adc_error_counter < 255) s_adc_error_counter++;
    }
}

void fault_manager_clear(FaultFlags_t fault) {
    s_active_faults &= ~(uint16_t)fault;

    if (fault & (FAULT_MPU6050_COMM_TIMEOUT | FAULT_MPU6050_WHO_AM_I)) {
        s_mpu_error_counter = 0;
    }
    if (fault & (FAULT_DHT11_TIMEOUT | FAULT_DHT11_CHECKSUM)) {
        s_dht_error_counter = 0;
    }
    if (fault & FAULT_ADC_OUT_OF_BOUNDS) {
        s_adc_error_counter = 0;
    }
}

bool fault_manager_is_active(FaultFlags_t fault) {
    return ((s_active_faults | s_injected_faults) & (uint16_t)fault) != 0;
}

uint16_t fault_manager_get_all(void) {
    return (s_active_faults | s_injected_faults);
}

SystemState_t fault_manager_evaluate_state(void) {
    uint16_t faults = fault_manager_get_all();

    /* Check for critical failures: I2C bus lockup or task starvation */
    if (faults & (FAULT_TASK_DEADLINE_MISS | FAULT_I2C_BUS_STUCK)) {
        s_current_state = SYSTEM_STATE_CRITICAL;
        return s_current_state;
    }

    uint8_t failed_sensor_count = 0;
    if (s_mpu_error_counter >= MAX_CONSECUTIVE_ERRORS || (faults & FAULT_MPU6050_COMM_TIMEOUT)) {
        failed_sensor_count++;
    }
    if (s_dht_error_counter >= MAX_CONSECUTIVE_ERRORS || (faults & FAULT_DHT11_TIMEOUT)) {
        failed_sensor_count++;
    }
    if (s_adc_error_counter >= MAX_CONSECUTIVE_ERRORS || (faults & FAULT_ADC_OUT_OF_BOUNDS)) {
        failed_sensor_count++;
    }

    if (failed_sensor_count == 0) {
        s_current_state = SYSTEM_STATE_NORMAL;
    } else if (failed_sensor_count == 1) {
        s_current_state = SYSTEM_STATE_DEGRADED;
    } else {
        s_current_state = SYSTEM_STATE_CRITICAL;
    }

    return s_current_state;
}

SystemState_t fault_manager_get_state(void) {
    return s_current_state;
}

void fault_manager_increment_i2c_recovery(void) {
    s_i2c_recovery_count++;
}

uint32_t fault_manager_get_i2c_recovery_count(void) {
    return s_i2c_recovery_count;
}

/* ========================================================================== */
/*                           Fault Injection Engine                           */
/* ========================================================================== */

void fault_inject_enable(FaultFlags_t fault) {
    s_injected_faults |= (uint16_t)fault;
}

void fault_inject_disable(FaultFlags_t fault) {
    s_injected_faults &= ~(uint16_t)fault;
}

bool fault_inject_is_active(FaultFlags_t fault) {
    return (s_injected_faults & (uint16_t)fault) != 0;
}

void fault_inject_clear_all(void) {
    s_injected_faults = FAULT_NONE;
}
