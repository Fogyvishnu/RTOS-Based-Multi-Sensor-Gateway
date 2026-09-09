/**
 * @file stm32l4xx_hal_msp.c
 * @brief HAL MSP (MCU Support Package) peripheral initialization callbacks
 *        Configures peripheral clocks, GPIO alternate functions, and DMA channels.
 */

#include "main.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)

extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_adc1;

/**
 * @brief Initializes the Global MSP.
 */
void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

/**
 * @brief UART MSP Initialization
 *        Enables USART2 clock, GPIO alternate functions (PA2 TX / PA3 RX),
 *        links DMA Channel 7, and configures NVIC interrupts.
 * @param huart: UART handle pointer
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART2) {
        /* 1. Peripheral clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        /* 2. USART2 GPIO Configuration: PA2 -> TX, PA3 -> RX */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* 3. USART2 DMA Init (DMA1 Channel 7, Request 2) */
        hdma_usart2_tx.Instance = DMA1_Channel7;
        hdma_usart2_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart2_tx);

        /* 4. USART2 and DMA Interrupt Priorities (subordinate to FreeRTOS syscall priority 5) */
        HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);

        HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    }
}

/**
 * @brief UART MSP De-Initialization
 * @param huart: UART handle pointer
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
        HAL_DMA_DeInit(huart->hdmatx);
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        HAL_NVIC_DisableIRQ(DMA1_Channel7_IRQn);
    }
}

/**
 * @brief I2C MSP Initialization
 *        Enables I2C1 clock and configures PB8 (SCL) and PB9 (SDA) as Open-Drain AF4
 * @param hi2c: I2C handle pointer
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hi2c->Instance == I2C1) {
        /* 1. Peripheral clock enable */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();

        /* 2. I2C1 GPIO Configuration: PB8 -> SCL, PB9 -> SDA */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

/**
 * @brief I2C MSP De-Initialization
 * @param hi2c: I2C handle pointer
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c) {
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_I2C1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    }
}

/**
 * @brief ADC MSP Initialization
 *        Enables ADC1 clock, configures PA0 as Analog ADC input,
 *        and initializes DMA1 Channel 1 for continuous sampling.
 * @param hadc: ADC handle pointer
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        /* 1. Peripheral clock enable */
        __HAL_RCC_ADC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        /* 2. ADC1 GPIO Configuration: PA0 -> IN5 */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* 3. ADC1 DMA Init (DMA1 Channel 1, Request 0) */
        hdma_adc1.Instance = DMA1_Channel1;
        hdma_adc1.Init.Request = DMA_REQUEST_0;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_adc1.Init.Mode = DMA_CIRCULAR;
        hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);

        /* 4. DMA1 Channel 1 runs autonomously without CPU interrupts */
    }
}

/**
 * @brief ADC MSP De-Initialization
 * @param hadc: ADC handle pointer
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);
        HAL_DMA_DeInit(hadc->DMA_Handle);
        HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    }
}

#endif /* STM32L433xx || USE_HAL_DRIVER */
