/**
 * @file task_supervisor.c
 * @brief Implementation of system supervisor, health check-in matrix, and watchdog handling
 */

#include "task_supervisor.h"
#include "app_config.h"
#include "fault_manager.h"
#include "bsp_nucleo_l433rc.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
extern IWDG_HandleTypeDef hiwdg;
#endif

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#else
/* Dummy definitions for host testing */
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
static void vTaskDelay(TickType_t t) { (void)t; }
#endif

static volatile uint32_t s_alive_flags = 0;

void task_supervisor_check_in(uint32_t alive_bit) {
    s_alive_flags |= alive_bit;
}

void Task_Supervisor_Entry(void *argument) {
    (void)argument;
    uint32_t tick_count = 0;

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS_SUPERVISOR));
#endif
        tick_count++;

        /* 1. Evaluate whether all critical tasks checked in */
        bool all_healthy = ((s_alive_flags & ALIVE_BITS_ALL_CRITICAL) == ALIVE_BITS_ALL_CRITICAL);

        if (!all_healthy) {
            /* At least one critical task missed its execution window */
            fault_manager_set(FAULT_TASK_DEADLINE_MISS);
            /* Intentionally do NOT refresh hardware IWDG to allow clean watchdog reboot if persistent */
        } else {
            fault_manager_clear(FAULT_TASK_DEADLINE_MISS);

            /* 2. Feed Hardware Independent Watchdog (IWDG) */
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
            HAL_IWDG_Refresh(&hiwdg);
#endif
        }

        /* 3. Reset alive flags for the next supervision epoch */
        s_alive_flags = 0;

        /* 4. Visual LED Heartbeat based on System State */
        SystemState_t state = fault_manager_get_state();
        if (state == SYSTEM_STATE_NORMAL) {
            /* 1 Hz toggle (every 500 ms -> 2.5 supervisor ticks) */
            if ((tick_count % 2) == 0) {
                bsp_led_toggle();
            }
        } else if (state == SYSTEM_STATE_DEGRADED) {
            /* Rapid warning blink (every 200 ms) */
            bsp_led_toggle();
        } else {
            /* Critical or Fault: Solid LED ON */
            bsp_led_set(true);
        }
    }
}
