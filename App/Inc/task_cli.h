/**
 * @file task_cli.h
 * @brief Interactive command line interface task over UART
 */

#ifndef TASK_CLI_H
#define TASK_CLI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the CLI ring buffer and task resources
 */
void task_cli_init(void);

/**
 * @brief Main CLI Task entry point
 */
void Task_CLI_Entry(void *argument);

/**
 * @brief Receive character callback from UART ISR
 * @param byte Character received
 */
void task_cli_on_rx_byte(uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* TASK_CLI_H */
