/**
 * @file potentiometer.h
 * @brief Driver for Potentiometer analog acquisition via ADC & circular DMA
 */

#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float filtered_percentage;
    uint16_t last_raw;
    bool is_initialized;
} PotentiometerHandle_t;

/**
 * @brief Initialize the potentiometer driver and start ADC DMA conversions
 * @param handle Driver handle
 */
bool potentiometer_init(PotentiometerHandle_t *handle);

/**
 * @brief Read analog value, voltage, percentage, and apply EWMA filtering
 * @param handle Driver handle
 * @param data Output data structure
 * @return true if ADC reading was within valid limits
 */
bool potentiometer_read(PotentiometerHandle_t *handle, PotentiometerData_t *data);

#ifdef __cplusplus
}
#endif

#endif /* POTENTIOMETER_H */
