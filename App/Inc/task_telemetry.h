/**
 * @file task_telemetry.h
 * @brief Telemetry gateway task supporting ANSI terminal dashboard, JSON, and CSV streaming
 */

#ifndef TASK_TELEMETRY_H
#define TASK_TELEMETRY_H

#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main Telemetry Gateway task entry point
 */
void Task_Telemetry_Entry(void *argument);

/**
 * @brief Switch telemetry streaming mode
 * @param mode TELEMETRY_MODE_ANSI, TELEMETRY_MODE_JSON, or TELEMETRY_MODE_CSV
 */
void task_telemetry_set_mode(TelemetryMode_t mode);

/**
 * @brief Get currently active telemetry mode
 */
TelemetryMode_t task_telemetry_get_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TELEMETRY_H */
