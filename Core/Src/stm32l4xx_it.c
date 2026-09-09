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

volatile uint32_t g_rx_count = 0;
extern uint8_t g_rx_byte;

void USART2_IRQHandler(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    HAL_UART_IRQHandler(&huart2);
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    if (huart->Instance == USART2) {
        g_rx_count++;
        task_cli_on_rx_byte(g_rx_byte);
        HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
    }
#endif
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    if (huart->Instance == USART2) {
        __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
        HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
    }
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
