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
    uint16_t raw_min;         /**< Raw ADC corresponding to 0% (default: 900) */
    uint16_t raw_max;         /**< Raw ADC corresponding to 100% (default: 3250) */
    bool auto_calibrate;      /**< If true, dynamically adjusts min/max when new extremes are seen */
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

/**
 * @brief Set two-point calibration bounds for potentiometer
 * @param handle Driver handle
 * @param raw_min ADC raw value corresponding to 0%
 * @param raw_max ADC raw value corresponding to 100%
 */
void potentiometer_set_calibration(PotentiometerHandle_t *handle, uint16_t raw_min, uint16_t raw_max);

/**
 * @brief Enable or disable dynamic auto-calibration
 * @param handle Driver handle
 * @param enable true to auto-expand range as potentiometer is rotated
 */
void potentiometer_enable_auto_calibration(PotentiometerHandle_t *handle, bool enable);

/**
 * @brief Get current potentiometer calibration parameters
 * @param handle Driver handle
 * @param raw_min Output pointer for raw_min
 * @param raw_max Output pointer for raw_max
 * @param auto_cal Output pointer for auto_calibrate flag
 */
void potentiometer_get_calibration(const PotentiometerHandle_t *handle, uint16_t *raw_min, uint16_t *raw_max, bool *auto_cal);

#ifdef __cplusplus
}
#endif

#endif /* POTENTIOMETER_H */
