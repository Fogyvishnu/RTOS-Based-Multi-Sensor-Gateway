/**
 * @file bsp_nucleo_l433rc.h
 * @brief Board Support Package for STM32 NUCLEO-L433RC-P
 */

#ifndef BSP_NUCLEO_L433RC_H
#define BSP_NUCLEO_L433RC_H

#include <stdint.h>
#include <stdbool.h>

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Pin definitions for Nucleo-L433RC-P */
#define LED4_PIN                    (GPIO_PIN_13)
#define LED4_GPIO_PORT              (GPIOB)
#define USER_BTN_PIN                (GPIO_PIN_13)
#define USER_BTN_GPIO_PORT          (GPIOC)

#define DHT11_PIN                   (GPIO_PIN_1)
#define DHT11_GPIO_PORT             (GPIOA)

#define POT_ADC_PIN                 (GPIO_PIN_0)
#define POT_ADC_GPIO_PORT           (GPIOA)

#define I2C1_SCL_PIN                (GPIO_PIN_8)
#define I2C1_SDA_PIN                (GPIO_PIN_9)
#define I2C1_GPIO_PORT              (GPIOB)

/**
 * @brief Initialize Board Support Package (GPIOs, Clocks, DWT cycle counter)
 */
void bsp_init(void);

/**
 * @brief Initialize DWT (Data Watchpoint and Trace) cycle counter for microsecond delays
 */
void bsp_dwt_init(void);

/**
 * @brief Accurate microsecond delay using ARM DWT cycle counter
 * @param us Microseconds to delay
 */
void bsp_delay_us(uint32_t us);

/**
 * @brief Set green user LED state
 */
void bsp_led_set(bool on);

/**
 * @brief Toggle green user LED
 */
void bsp_led_toggle(void);

/**
 * @brief Read state of user button B1 (PC13)
 * @return true if pressed
 */
bool bsp_button_is_pressed(void);

/**
 * @brief Transmit data over USART2 using DMA
 * @param data Pointer to buffer
 * @param len Length in bytes
 * @return true if DMA transmit started successfully
 */
bool bsp_uart_send_dma(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* BSP_NUCLEO_L433RC_H */
