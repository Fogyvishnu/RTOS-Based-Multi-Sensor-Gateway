/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS Kernel configuration for STM32L433RC-P (ARM Cortex-M4 @ 80MHz)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (7)
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)(24 * 1024))
#define configMAX_TASK_NAME_LEN                  (16)
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_APPLICATION_TASK_TAG           0
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1

/* Software timer definitions */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (2)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             (configMINIMAL_STACK_SIZE * 2)

/* Cortex-M specific definitions: STM32L4 has 4 priority bits */
#ifdef __NVIC_PRIO_BITS
 #define configPRIO_BITS                         __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS                         4
#endif

/* Lowest interrupt priority */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* Maximum interrupt priority from which FreeRTOS API functions can be called */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY          (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Critical nesting assert */
#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* Map FreeRTOS port handlers to CMSIS standard vector names */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
