/**
 * @file task_sensors.c
 * @brief Implementation of FreeRTOS sensor acquisition tasks
 */

#include "task_sensors.h"
#include "app_config.h"
#include "sensor_types.h"
#include "fault_manager.h"
#include "task_supervisor.h"
#include "mpu6050.h"
#include "dht11.h"
#include "potentiometer.h"
#include "bsp_nucleo_l433rc.h"

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
extern QueueHandle_t g_sensor_queue;
#else
/* Stub types for host testing */
typedef void* QueueHandle_t;
QueueHandle_t g_sensor_queue = 0;
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define pdTRUE 1
static TickType_t xTaskGetTickCount(void) { return 0; }
static void vTaskDelayUntil(TickType_t *p, TickType_t d) { (void)p; (void)d; }
static int xQueueSend(QueueHandle_t q, const void *p, TickType_t t) { (void)q; (void)p; (void)t; return 1; }
#endif

static Mpu6050Handle_t s_mpu_handle;
static Dht11Handle_t s_dht_handle;
static PotentiometerHandle_t s_pot_handle;

void task_sensors_init(void) {
    mpu6050_init(&s_mpu_handle);
    dht11_init(&s_dht_handle, DHT11_GPIO_PORT, DHT11_PIN);
    potentiometer_init(&s_pot_handle);
}

PotentiometerHandle_t* task_sensors_get_pot_handle(void) {
    return &s_pot_handle;
}

void Task_MPU6050_Entry(void *argument) {
    (void)argument;
    SensorPacket_t packet;
    packet.sensor_type = SENSOR_TYPE_MPU6050;

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
    TickType_t last_wake_time = xTaskGetTickCount();
#endif

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PERIOD_MS_MPU6050));
        packet.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
        packet.timestamp_ms = 0;
#endif

        /* Read IMU */
        bool success = mpu6050_read_all(&s_mpu_handle, &packet.data.mpu);
        if (!success) {
            /* Try bus recovery if bus locked */
            if (fault_manager_is_active(FAULT_I2C_BUS_STUCK)) {
                mpu6050_bus_recovery();
            }
        }

        /* Post to sensor queue */
        if (g_sensor_queue) {
            xQueueSend(g_sensor_queue, &packet, 0);
        }

        /* Report healthy check-in to supervisor */
        task_supervisor_check_in(ALIVE_BIT_MPU6050);
    }
}

void Task_DHT11_Entry(void *argument) {
    (void)argument;
    SensorPacket_t packet;
    packet.sensor_type = SENSOR_TYPE_DHT11;

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
    TickType_t last_wake_time = xTaskGetTickCount();
#endif

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PERIOD_MS_DHT11));
        packet.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
        packet.timestamp_ms = 0;
#endif

        /* Read DHT11 */
        dht11_read(&s_dht_handle, &packet.data.dht);

        /* Post to sensor queue */
        if (g_sensor_queue) {
            xQueueSend(g_sensor_queue, &packet, 0);
        }

        /* Report healthy check-in to supervisor */
        task_supervisor_check_in(ALIVE_BIT_DHT11);
    }
}

void Task_ADC_Entry(void *argument) {
    (void)argument;
    SensorPacket_t packet;
    packet.sensor_type = SENSOR_TYPE_POTENTIOMETER;

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
    TickType_t last_wake_time = xTaskGetTickCount();
#endif

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PERIOD_MS_ADC));
        packet.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
        packet.timestamp_ms = 0;
#endif

        /* Read Potentiometer */
        potentiometer_read(&s_pot_handle, &packet.data.pot);

        /* Post to sensor queue */
        if (g_sensor_queue) {
            xQueueSend(g_sensor_queue, &packet, 0);
        }

        /* Report healthy check-in to supervisor */
        task_supervisor_check_in(ALIVE_BIT_ADC);
    }
}
