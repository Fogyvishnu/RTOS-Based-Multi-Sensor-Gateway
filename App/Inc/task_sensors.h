/**
 * @file task_sensors.h
 * @brief FreeRTOS tasks for sensor acquisition (MPU6050, DHT11, Potentiometer)
 */

#ifndef TASK_SENSORS_H
#define TASK_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "potentiometer.h"

/**
 * @brief Initialize sensor queues and driver instances
 */
void task_sensors_init(void);

/**
 * @brief MPU6050 IMU acquisition task (100 Hz)
 */
void Task_MPU6050_Entry(void *argument);

/**
 * @brief DHT11 environmental acquisition task (0.5 Hz)
 */
void Task_DHT11_Entry(void *argument);

/**
 * @brief Potentiometer analog acquisition task (50 Hz)
 */
void Task_ADC_Entry(void *argument);

/**
 * @brief Get pointer to active potentiometer handle
 */
PotentiometerHandle_t* task_sensors_get_pot_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_SENSORS_H */
