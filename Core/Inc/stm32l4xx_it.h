/**
 * @file stm32l4xx_it.h
 * @brief Interrupt Handlers Header
 */

#ifndef STM32L4XX_IT_H
#define STM32L4XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
void DMA1_Channel7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32L4XX_IT_H */
