/**
 * @file task_processing.h
 * @brief Data fusion, filtering, anomaly detection, and state machine task
 */

#ifndef TASK_PROCESSING_H
#define TASK_PROCESSING_H

#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main Processing & Sensor Fusion task entry point
 */
void Task_Processing_Entry(void *argument);

/**
 * @brief Get latest consolidated telemetry snapshot (thread-safe read)
 * @param telem Pointer to destination structure
 */
void task_processing_get_latest_telemetry(GatewayTelemetry_t *telem);

#ifdef __cplusplus
}
#endif

#endif /* TASK_PROCESSING_H */
