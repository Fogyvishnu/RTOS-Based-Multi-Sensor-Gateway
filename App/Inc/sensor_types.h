/**
 * @file sensor_types.h
 * @brief Unified sensor data structures, fault codes, and telemetry definitions
 */

#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensor type enumeration
 */
typedef enum {
    SENSOR_TYPE_MPU6050 = 0,
    SENSOR_TYPE_DHT11,
    SENSOR_TYPE_POTENTIOMETER,
    SENSOR_TYPE_COUNT
} SensorType_t;

/**
 * @brief System operating states
 */
typedef enum {
    SYSTEM_STATE_INIT = 0,
    SYSTEM_STATE_NORMAL,
    SYSTEM_STATE_DEGRADED,
    SYSTEM_STATE_CRITICAL,
    SYSTEM_STATE_FAULT
} SystemState_t;

/**
 * @brief Telemetry streaming format modes
 */
typedef enum {
    TELEMETRY_MODE_ANSI = 0,  /**< Live VT100 interactive dashboard */
    TELEMETRY_MODE_JSON,      /**< Machine-readable JSON string */
    TELEMETRY_MODE_CSV        /**< Comma-separated values */
} TelemetryMode_t;

/**
 * @brief Fault and error flags (bitmask)
 */
typedef enum {
    FAULT_NONE                 = 0x0000,
    FAULT_MPU6050_COMM_TIMEOUT = 0x0001,
    FAULT_MPU6050_WHO_AM_I     = 0x0002,
    FAULT_DHT11_TIMEOUT        = 0x0004,
    FAULT_DHT11_CHECKSUM       = 0x0008,
    FAULT_ADC_OUT_OF_BOUNDS    = 0x0010,
    FAULT_QUEUE_FULL           = 0x0020,
    FAULT_TASK_DEADLINE_MISS   = 0x0040,
    FAULT_I2C_BUS_STUCK        = 0x0080
} FaultFlags_t;

/**
 * @brief Raw and calibrated MPU6050 6-DOF data
 */
typedef struct {
    int16_t accel_raw[3];     /**< Raw X, Y, Z accelerometer */
    int16_t gyro_raw[3];      /**< Raw X, Y, Z gyroscope */
    float accel_g[3];         /**< Acceleration in units of g */
    float gyro_dps[3];        /**< Angular rate in degrees per second */
    float temperature_c;      /**< Internal die temperature in °C */
    bool valid;               /**< True if reading succeeded */
} Mpu6050Data_t;

/**
 * @brief DHT11 environment data
 */
typedef struct {
    float temperature_c;      /**< Temperature in °C */
    float humidity_pct;       /**< Relative humidity percentage */
    float heat_index_c;       /**< Calculated feels-like temperature in °C */
    float dew_point_c;        /**< Calculated dew point in °C */
    bool valid;               /**< True if reading passed checksum */
} Dht11Data_t;

/**
 * @brief Potentiometer analog data
 */
typedef struct {
    uint16_t raw_adc;         /**< 12-bit ADC value (0 - 4095) */
    float voltage_v;          /**< Scaled voltage (0.0V - 3.3V) */
    float percentage;         /**< 0.0% to 100.0% */
    float filtered_pct;       /**< Exponentially smoothed percentage */
    bool valid;               /**< True if conversion succeeded */
} PotentiometerData_t;

/**
 * @brief Estimated attitude orientation from sensor fusion
 */
typedef struct {
    float roll_deg;           /**< Estimated Roll angle (-180° to +180°) */
    float pitch_deg;          /**< Estimated Pitch angle (-90° to +90°) */
    float roll_accel;         /**< Accelerometer-derived roll */
    float pitch_accel;        /**< Accelerometer-derived pitch */
    bool initialized;         /**< True once initial angles are seeded */
} Attitude_t;

/**
 * @brief Raw sensor packet sent via FreeRTOS sensor_queue
 */
typedef struct {
    SensorType_t sensor_type;
    uint32_t timestamp_ms;
    union {
        Mpu6050Data_t mpu;
        Dht11Data_t dht;
        PotentiometerData_t pot;
    } data;
} SensorPacket_t;

/**
 * @brief Gateway consolidated system telemetry snapshot
 */
typedef struct {
    uint32_t timestamp_ms;
    uint32_t uptime_sec;
    SystemState_t system_state;
    uint16_t active_faults;
    
    /* Processed Sensor Data */
    Mpu6050Data_t mpu;
    Attitude_t attitude;
    Dht11Data_t dht;
    PotentiometerData_t pot;

    /* System Health */
    uint32_t free_heap_bytes;
    uint8_t cpu_usage_pct;
    uint16_t min_stack_watermark_words[8];
    uint32_t i2c_recovery_count;
} GatewayTelemetry_t;

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TYPES_H */
