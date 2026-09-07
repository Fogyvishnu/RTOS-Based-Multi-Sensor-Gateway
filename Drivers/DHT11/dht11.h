/**
 * @file dht11.h
 * @brief Driver for DHT11 single-wire digital temperature and humidity sensor
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *gpio_port;
    uint16_t gpio_pin;
    uint32_t last_read_ms;
} Dht11Handle_t;

/**
 * @brief Initialize the DHT11 sensor handle
 * @param handle Driver handle
 * @param gpio_port Pointer to GPIO port (e.g. GPIOA)
 * @param gpio_pin GPIO pin mask (e.g. GPIO_PIN_1)
 */
void dht11_init(Dht11Handle_t *handle, void *gpio_port, uint16_t gpio_pin);

/**
 * @brief Read temperature and humidity using precise single-wire timing
 * @param handle Driver handle
 * @param data Output data structure
 * @return true if 40 bits were read and checksum matched
 */
bool dht11_read(Dht11Handle_t *handle, Dht11Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* DHT11_H */
