/**
 * @file task_supervisor.h
 * @brief System health monitor, stack watermark tracker, and hardware watchdog kicker
 */

#ifndef TASK_SUPERVISOR_H
#define TASK_SUPERVISOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register a task alive check-in
 * @param alive_bit Bit corresponding to task (e.g. ALIVE_BIT_MPU6050)
 */
void task_supervisor_check_in(uint32_t alive_bit);

/**
 * @brief Main FreeRTOS Supervisor Task function
 * @param argument Unused
 */
void Task_Supervisor_Entry(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* TASK_SUPERVISOR_H */
