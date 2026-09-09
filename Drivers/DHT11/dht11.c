/**
 * @file dht11.c
 * @brief Implementation of DHT11 single-wire protocol with microsecond timing
 */

#include "dht11.h"
#include "app_config.h"
#include "fault_manager.h"
#include "sensor_fusion.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
#include "bsp_nucleo_l433rc.h"
#endif

void dht11_init(Dht11Handle_t *handle, void *gpio_port, uint16_t gpio_pin) {
    if (!handle) return;
    handle->gpio_port = gpio_port;
    handle->gpio_pin = gpio_pin;
    handle->last_read_ms = 0;
}

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
static void set_pin_output(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void set_pin_input(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}
#endif

bool dht11_read(Dht11Handle_t *handle, Dht11Data_t *data) {
    if (!handle || !data) return false;

    /* Check fault injection */
    if (fault_inject_is_active(FAULT_DHT11_TIMEOUT)) {
        data->valid = false;
        fault_manager_set(FAULT_DHT11_TIMEOUT);
        return false;
    }

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    GPIO_TypeDef *port = (GPIO_TypeDef *)handle->gpio_port;
    uint16_t pin = handle->gpio_pin;

    uint8_t dht_bytes[5] = {0};
    uint32_t timeout = 0;

    /* 1. Send Start Signal: Pull Low for 18 ms */
    set_pin_output(port, pin);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    bsp_delay_us(18000);

    /* 2. Pull High for 30 us and switch to Input */
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    bsp_delay_us(30);
    set_pin_input(port, pin);

    /* 3. Wait for DHT11 Response: 80us Low, 80us High */
    timeout = 10000;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            fault_manager_set(FAULT_DHT11_TIMEOUT);
            data->valid = false;
            return false;
        }
    }

    timeout = 10000;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
        if (--timeout == 0) {
            fault_manager_set(FAULT_DHT11_TIMEOUT);
            data->valid = false;
            return false;
        }
    }

    timeout = 10000;
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            fault_manager_set(FAULT_DHT11_TIMEOUT);
            data->valid = false;
            return false;
        }
    }

    /* 4. Read 40 bits (5 bytes) */
    for (int byte_idx = 0; byte_idx < 5; byte_idx++) {
        for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
            /* Wait for 50us low pulse to finish */
            timeout = 10000;
            while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {
                if (--timeout == 0) {
                    fault_manager_set(FAULT_DHT11_TIMEOUT);
                    data->valid = false;
                    return false;
                }
            }

            /* Measure high pulse duration */
            uint32_t start_cycles = DWT->CYCCNT;
            timeout = 10000;
            while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) {
                if (--timeout == 0) {
                    fault_manager_set(FAULT_DHT11_TIMEOUT);
                    data->valid = false;
                    return false;
                }
            }
            uint32_t duration_us = (DWT->CYCCNT - start_cycles) / (SystemCoreClock / 1000000);

            /* > 40 us means bit 1, else bit 0 */
            if (duration_us > 40) {
                dht_bytes[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    /* 5. Checksum verification */
    uint8_t calculated_checksum = dht_bytes[0] + dht_bytes[1] + dht_bytes[2] + dht_bytes[3];
    if (fault_inject_is_active(FAULT_DHT11_CHECKSUM)) {
        calculated_checksum += 1; /* Force mismatch */
    }

    if (calculated_checksum != dht_bytes[4]) {
        fault_manager_set(FAULT_DHT11_CHECKSUM);
        data->valid = false;
        return false;
    }

    data->humidity_pct = (float)dht_bytes[0] + ((float)dht_bytes[1] * 0.1f);
    data->temperature_c = (float)dht_bytes[2] + ((float)dht_bytes[3] * 0.1f);

#else
    /* Mock data for host testing */
    data->temperature_c = 24.5f;
    data->humidity_pct = 55.0f;
#endif

    data->dew_point_c = sensor_fusion_dew_point(data->temperature_c, data->humidity_pct);
    data->heat_index_c = sensor_fusion_heat_index(data->temperature_c, data->humidity_pct);
    data->valid = true;

    fault_manager_clear(FAULT_DHT11_TIMEOUT);
    fault_manager_clear(FAULT_DHT11_CHECKSUM);
    return true;
}
