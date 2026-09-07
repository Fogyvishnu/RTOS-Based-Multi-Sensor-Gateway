/**
 * @file main.c
 * @brief Gateway entry point, clock configuration, peripheral setup, and RTOS task boot
 */

#include "main.h"
#include "task_supervisor.h"
#include "task_sensors.h"
#include "task_processing.h"
#include "task_telemetry.h"
#include "task_cli.h"

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Global IPC Queue Handles */
QueueHandle_t g_sensor_queue = NULL;
QueueHandle_t g_telemetry_queue = NULL;

/* Task Handles */
static TaskHandle_t s_handle_supervisor = NULL;
static TaskHandle_t s_handle_mpu = NULL;
static TaskHandle_t s_handle_dht = NULL;
static TaskHandle_t s_handle_adc = NULL;
static TaskHandle_t s_handle_processing = NULL;
static TaskHandle_t s_handle_telemetry = NULL;
static TaskHandle_t s_handle_cli = NULL;
#endif

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;
I2C_HandleTypeDef hi2c1;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
IWDG_HandleTypeDef hiwdg;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_IWDG_Init(void);
#endif

int main(void) {
#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* Reset all peripherals, initialize Flash interface and Systick */
    HAL_Init();

    /* Configure system clock to 80 MHz */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();
    MX_IWDG_Init();
#endif

    /* Initialize BSP and Fault Management Subsystems */
    bsp_init();
    fault_manager_init();
    task_sensors_init();

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
    /* Create Inter-Task Communication Queues */
    g_sensor_queue = xQueueCreate(SENSOR_QUEUE_LEN, sizeof(SensorPacket_t));
    g_telemetry_queue = xQueueCreate(TELEMETRY_QUEUE_LEN, sizeof(GatewayTelemetry_t));

    if (!g_sensor_queue || !g_telemetry_queue) {
        Error_Handler();
    }

    /* Create FreeRTOS Tasks */
    xTaskCreate(Task_Supervisor_Entry, "Supervisor", STACK_SIZE_SUPERVISOR, NULL, TASK_PRIO_SUPERVISOR, &s_handle_supervisor);
    xTaskCreate(Task_Processing_Entry, "Processing", STACK_SIZE_PROCESSING, NULL, TASK_PRIO_PROCESSING, &s_handle_processing);
    xTaskCreate(Task_MPU6050_Entry,    "MPU6050",    STACK_SIZE_MPU6050,    NULL, TASK_PRIO_MPU6050,    &s_handle_mpu);
    xTaskCreate(Task_ADC_Entry,        "ADC_Pot",    STACK_SIZE_ADC,        NULL, TASK_PRIO_ADC,        &s_handle_adc);
    xTaskCreate(Task_DHT11_Entry,      "DHT11",      STACK_SIZE_DHT11,      NULL, TASK_PRIO_DHT11,      &s_handle_dht);
    xTaskCreate(Task_Telemetry_Entry,  "Telemetry",  STACK_SIZE_TELEMETRY,  NULL, TASK_PRIO_TELEMETRY,  &s_handle_telemetry);
    xTaskCreate(Task_CLI_Entry,        "CLI",        STACK_SIZE_CLI,        NULL, TASK_PRIO_CLI,        &s_handle_cli);

    /* Start FreeRTOS Scheduler */
    vTaskStartScheduler();
#endif

    /* Should never reach here */
    while (1) {
    }
}

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
/* FreeRTOS Stack Overflow Hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    fault_manager_set(FAULT_TASK_DEADLINE_MISS);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* FreeRTOS Malloc Failed Hook */
void vApplicationMallocFailedHook(void) {
    fault_manager_set(FAULT_QUEUE_FULL);
    taskDISABLE_INTERRUPTS();
    for (;;);
}
#endif

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; /* 4 MHz */
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40; /* 4MHz * 40 = 160 MHz VCO */
    RCC_OscInitStruct.PLL.PLLR = 2;  /* 160MHz / 2 = 80 MHz SYSCLK */
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}

static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1 Channel 1 for ADC1 */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    /* DMA1 Channel 7 for USART2 TX */
    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00702991; /* 400 kHz Fast Mode @ 80 MHz */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_5; /* PA0 */
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_IWDG_Init(void) {
    /* LSI = 32 kHz. Prescaler 32 -> 1 kHz (1 ms per tick). Reload = 1500 -> 1.5s */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = WATCHDOG_TIMEOUT_MS;
    hiwdg.Init.Window = WATCHDOG_TIMEOUT_MS;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}
#endif
