/**
 * @file task_processing.c
 * @brief Implementation of sensor fusion, filtering, and telemetry consolidation
 */

#include "task_processing.h"
#include "app_config.h"
#include "fault_manager.h"
#include "sensor_fusion.h"
#include "task_supervisor.h"
#include <string.h>

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
extern QueueHandle_t g_sensor_queue;
extern QueueHandle_t g_telemetry_queue;
#else
typedef void* QueueHandle_t;
QueueHandle_t g_telemetry_queue = 0;
extern QueueHandle_t g_sensor_queue;
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define pdTRUE 1
static int xQueueReceive(QueueHandle_t q, void *p, TickType_t t) { (void)q; (void)p; (void)t; return 0; }
static int xQueueSend(QueueHandle_t q, const void *p, TickType_t t) { (void)q; (void)p; (void)t; return 1; }
static uint32_t xTaskGetTickCount(void) { return 0; }
#endif

static GatewayTelemetry_t s_latest_telemetry;

void task_processing_get_latest_telemetry(GatewayTelemetry_t *telem) {
    if (telem) {
        memcpy(telem, &s_latest_telemetry, sizeof(GatewayTelemetry_t));
    }
}

void Task_Processing_Entry(void *argument) {
    (void)argument;
    SensorPacket_t packet;
    uint32_t last_mpu_time_ms = 0;

    memset(&s_latest_telemetry, 0, sizeof(GatewayTelemetry_t));
    sensor_fusion_init(&s_latest_telemetry.attitude);

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        /* Block until a sensor packet is available (timeout 20ms) */
        if (xQueueReceive(g_sensor_queue, &packet, pdMS_TO_TICKS(PERIOD_MS_PROCESSING)) == pdTRUE) {
#else
        if (0) {
#endif
            switch (packet.sensor_type) {
                case SENSOR_TYPE_MPU6050: {
                    s_latest_telemetry.mpu = packet.data.mpu;
                    if (packet.data.mpu.valid) {
                        float dt_sec = 0.01f;
                        if (last_mpu_time_ms > 0 && packet.timestamp_ms > last_mpu_time_ms) {
                            dt_sec = (float)(packet.timestamp_ms - last_mpu_time_ms) / 1000.0f;
                        }
                        last_mpu_time_ms = packet.timestamp_ms;

                        /* Run Complementary Filter for Roll & Pitch */
                        sensor_fusion_update(&s_latest_telemetry.attitude,
                                             packet.data.mpu.accel_g,
                                             packet.data.mpu.gyro_dps,
                                             dt_sec,
                                             COMPLEMENTARY_ALPHA);
                    }
                    break;
                }
                case SENSOR_TYPE_DHT11: {
                    s_latest_telemetry.dht = packet.data.dht;
                    break;
                }
                case SENSOR_TYPE_POTENTIOMETER: {
                    s_latest_telemetry.pot = packet.data.pot;
                    break;
                }
                default:
                    break;
            }
        }

        /* Update consolidated health metrics */
        s_latest_telemetry.system_state = fault_manager_evaluate_state();
        s_latest_telemetry.active_faults = fault_manager_get_all();
        s_latest_telemetry.i2c_recovery_count = fault_manager_get_i2c_recovery_count();

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        s_latest_telemetry.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        s_latest_telemetry.uptime_sec = s_latest_telemetry.timestamp_ms / 1000;
        s_latest_telemetry.free_heap_bytes = (uint32_t)xPortGetFreeHeapSize();
#endif

        /* Send consolidated telemetry packet to Telemetry Gateway queue */
        if (g_telemetry_queue) {
            xQueueSend(g_telemetry_queue, &s_latest_telemetry, 0);
        }

        /* Report healthy check-in to supervisor */
        task_supervisor_check_in(ALIVE_BIT_PROCESSING);
    }
}
