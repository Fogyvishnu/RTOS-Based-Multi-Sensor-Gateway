/**
 * @file app_config.h
 * @brief System constants, RTOS task priorities, stack sizes, and thresholds
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           FreeRTOS Task Priorities                        */
/* ========================================================================== */
#define TASK_PRIO_SUPERVISOR        (5)   /**< Highest priority: health & IWDG */
#define TASK_PRIO_PROCESSING        (4)   /**< Real-time sensor fusion */
#define TASK_PRIO_MPU6050           (4)   /**< High-rate IMU sampling */
#define TASK_PRIO_ADC               (3)   /**< Medium-rate analog sampling */
#define TASK_PRIO_TELEMETRY         (3)   /**< Telemetry UART formatting/DMA */
#define TASK_PRIO_DHT11             (2)   /**< Low-rate environmental sampling */
#define TASK_PRIO_CLI               (2)   /**< Interactive shell */

/* ========================================================================== */
/*                          FreeRTOS Task Stack Sizes                         */
/* ========================================================================== */
#define STACK_SIZE_SUPERVISOR       (256) /**< Words (32-bit) */
#define STACK_SIZE_PROCESSING       (512)
#define STACK_SIZE_MPU6050          (384)
#define STACK_SIZE_ADC              (256)
#define STACK_SIZE_TELEMETRY        (512)
#define STACK_SIZE_DHT11            (256)
#define STACK_SIZE_CLI              (512)

/* ========================================================================== */
/*                             Task Timing (ms)                              */
/* ========================================================================== */
#define PERIOD_MS_MPU6050           (10)   /**< 100 Hz */
#define PERIOD_MS_ADC               (20)   /**< 50 Hz */
#define PERIOD_MS_PROCESSING        (20)   /**< 50 Hz */
#define PERIOD_MS_TELEMETRY         (500)  /**< 2 Hz for smooth ANSI terminal without UART saturation */
#define PERIOD_MS_SUPERVISOR        (200)  /**< 5 Hz */
#define PERIOD_MS_DHT11             (2000) /**< 0.5 Hz */

/* ========================================================================== */
/*                            IPC Queue Dimensions                           */
/* ========================================================================== */
#define SENSOR_QUEUE_LEN            (16)
#define TELEMETRY_QUEUE_LEN         (4)
#define CLI_RX_BUFFER_SIZE          (128)
#define UART_TX_DMA_BUFFER_SIZE     (1536)

/* ========================================================================== */
/*                         Supervisor Alive Bitmask Flags                    */
/* ========================================================================== */
#define ALIVE_BIT_MPU6050           (1 << 0)
#define ALIVE_BIT_DHT11             (1 << 1)
#define ALIVE_BIT_ADC               (1 << 2)
#define ALIVE_BIT_PROCESSING        (1 << 3)
#define ALIVE_BIT_TELEMETRY         (1 << 4)
#define ALIVE_BIT_CLI               (1 << 5)

#define ALIVE_BITS_ALL_CRITICAL     (ALIVE_BIT_MPU6050 | \
                                     ALIVE_BIT_ADC | \
                                     ALIVE_BIT_PROCESSING)

/* ========================================================================== */
/*                       Sensor Fusion & Filtering Constants                 */
/* ========================================================================== */
#define COMPLEMENTARY_ALPHA         (0.98f) /**< Gyro trust factor (0.95 - 0.99) */
#define EWMA_BETA                   (0.20f) /**< ADC low-pass smoothing factor */
#define POT_ADC_RESOLUTION          (4095.0f)
#define VREF_VOLTS                  (3.30f)

/* ========================================================================== */
/*                          Watchdog & Bus Timeouts                          */
/* ========================================================================== */
#define WATCHDOG_TIMEOUT_MS         (1500)
#define I2C_TIMEOUT_MS              (25)
#define MAX_CONSECUTIVE_ERRORS      (5)

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
