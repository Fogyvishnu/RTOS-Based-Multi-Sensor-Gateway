/**
 * @file main.h
 * @brief Master system include file and pin assignments
 */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
#endif

#include "app_config.h"
#include "sensor_types.h"
#include "fault_manager.h"
#include "bsp_nucleo_l433rc.h"

/* Error Handler declaration */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
