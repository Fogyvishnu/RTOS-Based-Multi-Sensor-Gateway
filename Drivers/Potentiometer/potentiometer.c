/**
 * @file potentiometer.c
 * @brief Implementation of ADC potentiometer sampling and EWMA filtering
 */

#include "potentiometer.h"
#include "app_config.h"
#include "fault_manager.h"
#include "sensor_fusion.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
extern ADC_HandleTypeDef hadc1;
static volatile uint32_t s_adc_dma_buffer[1];
#endif

bool potentiometer_init(PotentiometerHandle_t *handle) {
    if (!handle) return false;
    handle->filtered_percentage = 0.0f;
    handle->last_raw = 0;
    handle->raw_min = 900;   /* Default lower bound from physical wiper measurement */
    handle->raw_max = 3250;  /* Default upper bound from physical wiper measurement */
    handle->auto_calibrate = true;
    handle->is_initialized = true;

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Start ADC with DMA in circular mode */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_adc_dma_buffer, 1);
#endif

    return true;
}

void potentiometer_set_calibration(PotentiometerHandle_t *handle, uint16_t raw_min, uint16_t raw_max) {
    if (!handle) return;
    if (raw_min < raw_max) {
        handle->raw_min = raw_min;
        handle->raw_max = raw_max;
    }
}

void potentiometer_enable_auto_calibration(PotentiometerHandle_t *handle, bool enable) {
    if (!handle) return;
    handle->auto_calibrate = enable;
}

void potentiometer_get_calibration(const PotentiometerHandle_t *handle, uint16_t *raw_min, uint16_t *raw_max, bool *auto_cal) {
    if (!handle) return;
    if (raw_min) *raw_min = handle->raw_min;
    if (raw_max) *raw_max = handle->raw_max;
    if (auto_cal) *auto_cal = handle->auto_calibrate;
}

bool potentiometer_read(PotentiometerHandle_t *handle, PotentiometerData_t *data) {
    if (!handle || !data) return false;

    if (fault_inject_is_active(FAULT_ADC_OUT_OF_BOUNDS)) {
        data->raw_adc = 9999; /* Out of bounds */
        data->valid = false;
        fault_manager_set(FAULT_ADC_OUT_OF_BOUNDS);
        return false;
    }

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    data->raw_adc = (uint16_t)s_adc_dma_buffer[0];
#else
    /* Mock potentiometer reading: 50% = 2048 */
    data->raw_adc = 2048;
#endif

    if (data->raw_adc > 4095) {
        fault_manager_set(FAULT_ADC_OUT_OF_BOUNDS);
        data->valid = false;
        return false;
    }

    /* Auto-calibrate bounds dynamically if wiper reaches beyond current window */
    if (handle->auto_calibrate) {
        if (data->raw_adc < handle->raw_min && data->raw_adc < 4000) {
            handle->raw_min = data->raw_adc;
        }
        if (data->raw_adc > handle->raw_max && data->raw_adc <= 4095) {
            handle->raw_max = data->raw_adc;
        }
    }

    data->voltage_v = ((float)data->raw_adc / POT_ADC_RESOLUTION) * VREF_VOLTS;

    /* Map [raw_min, raw_max] -> [0.0%, 100.0%] */
    float span = (float)(handle->raw_max - handle->raw_min);
    if (span < 10.0f) span = 10.0f;

    float pct = ((float)data->raw_adc - (float)handle->raw_min) / span * 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    data->percentage = pct;

    /* Apply digital EWMA low-pass filter */
    handle->filtered_percentage = sensor_fusion_ewma(handle->filtered_percentage,
                                                     data->percentage,
                                                     EWMA_BETA);
    data->filtered_pct = handle->filtered_percentage;
    data->valid = true;
    handle->last_raw = data->raw_adc;

    fault_manager_clear(FAULT_ADC_OUT_OF_BOUNDS);
    return true;
}
