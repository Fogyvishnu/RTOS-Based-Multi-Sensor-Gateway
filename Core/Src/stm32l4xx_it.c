/**
 * @file stm32l4xx_it.c
 * @brief Interrupt Service Routines implementation
 */

#include "stm32l4xx_it.h"
#include "main.h"
#include "task_cli.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_adc1;
#endif

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
extern void xPortSysTickHandler(void);
#endif

void NMI_Handler(void) {
    while (1) {}
}

void HardFault_Handler(void) {
    /* Capture fault for diagnostics */
    while (1) {}
}

void MemManage_Handler(void) {
    while (1) {}
}

void BusFault_Handler(void) {
    while (1) {}
}

void UsageFault_Handler(void) {
    while (1) {}
}

void SysTick_Handler(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_IncTick();
#endif

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
#endif
}

void USART2_IRQHandler(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Check RX Not Empty flag directly */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        uint8_t ch = (uint8_t)(huart2.Instance->RDR & 0xFF);
        task_cli_on_rx_byte(ch);
    }
    HAL_UART_IRQHandler(&huart2);
#endif
}

void DMA1_Channel1_IRQHandler(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_DMA_IRQHandler(&hdma_adc1);
#endif
}

void DMA1_Channel7_IRQHandler(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
#endif
}
