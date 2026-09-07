/**
 * @file task_cli.c
 * @brief Implementation of interactive UART CLI shell
 */

#include "task_cli.h"
#include "app_config.h"
#include "fault_manager.h"
#include "task_telemetry.h"
#include "task_supervisor.h"
#include "ring_buffer.h"
#include "bsp_nucleo_l433rc.h"
#include <stdio.h>
#include <string.h>

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
#endif

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#else
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
static void vTaskDelay(TickType_t t) { (void)t; }
#endif

static RingBuffer_t s_cli_rb;
static uint8_t s_rb_storage[CLI_RX_BUFFER_SIZE];
static char s_line_buf[64];
static size_t s_line_idx = 0;

void task_cli_init(void) {
    ring_buffer_init(&s_cli_rb, s_rb_storage, CLI_RX_BUFFER_SIZE);
    s_line_idx = 0;
    memset(s_line_buf, 0, sizeof(s_line_buf));
}

void task_cli_on_rx_byte(uint8_t byte) {
    ring_buffer_push(&s_cli_rb, byte);
}

static void cli_print(const char *str) {
    bsp_uart_send_dma((const uint8_t*)str, (uint16_t)strlen(str));
}

static void execute_command(char *cmd) {
    char reply[256];

    /* Trim leading spaces */
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if (strcmp(cmd, "help") == 0) {
        cli_print(
            "\r\n--- Available CLI Commands ---\r\n"
            "  help                  Show this command list\r\n"
            "  status                Display system state and active faults\r\n"
            "  stream <ansi|json|csv> Switch telemetry streaming mode\r\n"
            "  top                   Display FreeRTOS task stats & watermarks\r\n"
            "  inject <fault>        Inject fault: mpu, dht, pot, task, or none\r\n"
            "  clear                 Clear all injected faults\r\n"
            "  reboot                Trigger MCU software reset\r\n"
            "------------------------------\r\n"
        );
    } else if (strcmp(cmd, "status") == 0) {
        SystemState_t s = fault_manager_get_state();
        uint16_t f = fault_manager_get_all();
        snprintf(reply, sizeof(reply),
                 "\r\nSystem State: %s (Code %d), Fault Mask: 0x%04X, I2C Recov: %u\r\n",
                 (s == SYSTEM_STATE_NORMAL) ? "NORMAL" :
                 (s == SYSTEM_STATE_DEGRADED) ? "DEGRADED" : "CRITICAL",
                 (int)s, f, (unsigned int)fault_manager_get_i2c_recovery_count());
        cli_print(reply);
    } else if (strncmp(cmd, "stream ", 7) == 0) {
        char *arg = cmd + 7;
        if (strcmp(arg, "json") == 0) {
            task_telemetry_set_mode(TELEMETRY_MODE_JSON);
            cli_print("\r\n[OK] Telemetry switched to JSON mode\r\n");
        } else if (strcmp(arg, "csv") == 0) {
            task_telemetry_set_mode(TELEMETRY_MODE_CSV);
            cli_print("\r\n[OK] Telemetry switched to CSV mode\r\n");
        } else if (strcmp(arg, "ansi") == 0) {
            task_telemetry_set_mode(TELEMETRY_MODE_ANSI);
            cli_print("\r\n[OK] Telemetry switched to ANSI Dashboard mode\r\n");
        } else {
            cli_print("\r\n[ERR] Unknown mode. Choose: ansi, json, csv\r\n");
        }
    } else if (strcmp(cmd, "top") == 0) {
#if (configUSE_TRACE_FACILITY == 1) && defined(FREERTOS)
        char stats_buf[512];
        vTaskList(stats_buf);
        cli_print("\r\nTask          State  Prio  Stack  Num\r\n");
        cli_print("--------------------------------------\r\n");
        cli_print(stats_buf);
#else
        cli_print("\r\nTask runtime stats: FreeRTOS trace facility enabled in target build.\r\n");
#endif
    } else if (strncmp(cmd, "inject ", 7) == 0) {
        char *arg = cmd + 7;
        if (strcmp(arg, "mpu") == 0) {
            fault_inject_enable(FAULT_MPU6050_COMM_TIMEOUT);
            cli_print("\r\n[INJECTED] MPU6050 communication timeout fault\r\n");
        } else if (strcmp(arg, "dht") == 0) {
            fault_inject_enable(FAULT_DHT11_CHECKSUM);
            cli_print("\r\n[INJECTED] DHT11 checksum failure fault\r\n");
        } else if (strcmp(arg, "pot") == 0) {
            fault_inject_enable(FAULT_ADC_OUT_OF_BOUNDS);
            cli_print("\r\n[INJECTED] Potentiometer ADC out-of-bounds fault\r\n");
        } else if (strcmp(arg, "task") == 0) {
            fault_inject_enable(FAULT_TASK_DEADLINE_MISS);
            cli_print("\r\n[INJECTED] Task starvation fault (watchdog test)\r\n");
        } else if (strcmp(arg, "none") == 0) {
            fault_inject_clear_all();
            cli_print("\r\n[CLEARED] Injected faults cleared\r\n");
        } else {
            cli_print("\r\n[ERR] Unknown fault target. Options: mpu, dht, pot, task, none\r\n");
        }
    } else if (strcmp(cmd, "clear") == 0) {
        fault_inject_clear_all();
        cli_print("\r\n[CLEARED] All faults cleared\r\n");
    } else if (strcmp(cmd, "reboot") == 0) {
        cli_print("\r\n[REBOOT] Initiating system reset...\r\n");
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
        NVIC_SystemReset();
#endif
    } else {
        snprintf(reply, sizeof(reply), "\r\n[ERR] Unknown command: '%s'. Type 'help'.\r\n", cmd);
        cli_print(reply);
    }
}

void Task_CLI_Entry(void *argument) {
    (void)argument;
    uint8_t ch = 0;

    task_cli_init();
    cli_print("\r\nGateway CLI Ready. Type 'help' for command list.\r\nCLI> ");

    for (;;) {
        while (ring_buffer_pop(&s_cli_rb, &ch)) {
            /* Echo character */
            char echo[2] = {(char)ch, '\0'};
            cli_print(echo);

            if (ch == '\r' || ch == '\n') {
                s_line_buf[s_line_idx] = '\0';
                execute_command(s_line_buf);
                s_line_idx = 0;
                cli_print("CLI> ");
            } else if (ch == '\b' || ch == 127) {
                /* Backspace */
                if (s_line_idx > 0) {
                    s_line_idx--;
                    cli_print(" \b");
                }
            } else {
                if (s_line_idx < sizeof(s_line_buf) - 1) {
                    s_line_buf[s_line_idx++] = (char)ch;
                }
            }
        }

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelay(pdMS_TO_TICKS(50));
#endif
        task_supervisor_check_in(ALIVE_BIT_CLI);
    }
}
