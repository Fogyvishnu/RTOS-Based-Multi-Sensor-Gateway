/**
 * @file bsp_nucleo_l433rc.c
 * @brief Implementation of Nucleo-L433RC-P board support functions
 */

#include "bsp_nucleo_l433rc.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
extern UART_HandleTypeDef huart2;

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
static SemaphoreHandle_t s_uart_mutex = NULL;
#endif
#endif

void bsp_init(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Enable GPIO Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure User LED (PB13) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED4_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED4_GPIO_PORT, LED4_PIN, GPIO_PIN_RESET);

    /* Configure User Button (PC13) */
    GPIO_InitStruct.Pin = USER_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_BTN_GPIO_PORT, &GPIO_InitStruct);

    /* Initialize DWT cycle counter */
    bsp_dwt_init();

#if (defined(FREERTOS) || defined(INC_FREERTOS_H))
    if (!s_uart_mutex) {
        s_uart_mutex = xSemaphoreCreateMutex();
    }
#endif
#endif
}

void bsp_dwt_init(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Enable TRC (Trace) in CoreDebug */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Reset and enable DWT cycle counter */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
}

void bsp_delay_us(uint32_t us) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    uint32_t start_tick = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start_tick) < ticks);
#else
    /* Host delay dummy */
    (void)us;
#endif
}

void bsp_led_set(bool on) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_GPIO_WritePin(LED4_GPIO_PORT, LED4_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    (void)on;
#endif
}

void bsp_led_toggle(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_GPIO_TogglePin(LED4_GPIO_PORT, LED4_PIN);
#endif
}

bool bsp_button_is_pressed(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Nucleo blue button is active LOW */
    return (HAL_GPIO_ReadPin(USER_BTN_GPIO_PORT, USER_BTN_PIN) == GPIO_PIN_RESET);
#else
    return false;
#endif
}

bool bsp_uart_send_dma(const uint8_t *data, uint16_t len) {
    if (!data || len == 0) return false;
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#if (defined(FREERTOS) || defined(INC_FREERTOS_H))
    if (s_uart_mutex && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (xSemaphoreTake(s_uart_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            HAL_StatusTypeDef res = HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 500);
            xSemaphoreGive(s_uart_mutex);
            return (res == HAL_OK);
        }
        return false;
    }
#endif
    return (HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 500) == HAL_OK);
#else
    (void)data;
    (void)len;
    return true;
#endif
}
