/**
 * @file fault_manager.h
 * @brief Fault detection, state machine, degraded modes, and fault injection engine
 */

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the fault management subsystem
 */
void fault_manager_init(void);

/**
 * @brief Report an error occurrence for a given fault code
 * @param fault The fault flag to set
 */
void fault_manager_set(FaultFlags_t fault);

/**
 * @brief Clear an error condition when communication succeeds
 * @param fault The fault flag to clear
 */
void fault_manager_clear(FaultFlags_t fault);

/**
 * @brief Check if a specific fault is active
 * @param fault Fault to check
 * @return true if fault is active
 */
bool fault_manager_is_active(FaultFlags_t fault);

/**
 * @brief Get the bitmask of all currently active faults
 * @return 16-bit fault bitmask
 */
uint16_t fault_manager_get_all(void);

/**
 * @brief Compute and update the overall system state (Normal, Degraded, Critical)
 * @return Current system state
 */
SystemState_t fault_manager_evaluate_state(void);

/**
 * @brief Get the current system state
 */
SystemState_t fault_manager_get_state(void);

/**
 * @brief Increment I2C bus recovery counter
 */
void fault_manager_increment_i2c_recovery(void);

/**
 * @brief Get total I2C bus recovery attempts
 */
uint32_t fault_manager_get_i2c_recovery_count(void);

/* ========================================================================== */
/*                           Fault Injection Engine                           */
/* ========================================================================== */

/**
 * @brief Injects an artificial fault for testing and demo purposes
 * @param fault Fault to simulate
 */
void fault_inject_enable(FaultFlags_t fault);

/**
 * @brief Disables simulated fault
 * @param fault Fault to stop simulating
 */
void fault_inject_disable(FaultFlags_t fault);

/**
 * @brief Check if a fault is artificially injected
 */
bool fault_inject_is_active(FaultFlags_t fault);

/**
 * @brief Clear all injected faults
 */
void fault_inject_clear_all(void);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_MANAGER_H */
