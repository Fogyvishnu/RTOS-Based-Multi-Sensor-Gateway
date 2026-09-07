/**
 * @file mpu6050.h
 * @brief Driver for InvenSense MPU6050 6-Axis MotionTracking IMU
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MPU6050 I2C 7-bit Address (AD0 = 0 -> 0x68, AD0 = 1 -> 0x69) */
#define MPU6050_I2C_ADDR_DEFAULT    (0x68 << 1)
#define MPU6050_WHO_AM_I_VAL        (0x68)

/* Register Map */
#define MPU6050_REG_SMPLRT_DIV      (0x19)
#define MPU6050_REG_CONFIG          (0x1A)
#define MPU6050_REG_GYRO_CONFIG     (0x1B)
#define MPU6050_REG_ACCEL_CONFIG    (0x1C)
#define MPU6050_REG_FIFO_EN         (0x23)
#define MPU6050_REG_INT_PIN_CFG     (0x37)
#define MPU6050_REG_INT_ENABLE      (0x38)
#define MPU6050_REG_INT_STATUS      (0x3A)
#define MPU6050_REG_ACCEL_XOUT_H    (0x3B)
#define MPU6050_REG_TEMP_OUT_H      (0x41)
#define MPU6050_REG_GYRO_XOUT_H     (0x43)
#define MPU6050_REG_USER_CTRL       (0x6A)
#define MPU6050_REG_PWR_MGMT_1      (0x6B)
#define MPU6050_REG_PWR_MGMT_2      (0x6C)
#define MPU6050_REG_WHO_AM_I        (0x75)

/* Sensitivity Scaling Factors */
/* Accel +/-2g -> 16384 LSB/g */
#define MPU6050_ACCEL_SENS_2G       (16384.0f)
/* Gyro +/-250 deg/s -> 131.0 LSB/(deg/s) */
#define MPU6050_GYRO_SENS_250DPS    (131.0f)

typedef struct {
    float accel_offset[3];
    float gyro_offset[3];
    bool is_calibrated;
    uint8_t dev_addr;
} Mpu6050Handle_t;

/**
 * @brief Initialize the MPU6050 sensor
 * @param handle Driver handle
 * @return true if WHO_AM_I matched and setup succeeded
 */
bool mpu6050_init(Mpu6050Handle_t *handle);

/**
 * @brief Read 14 bytes burst (Accel X/Y/Z, Temp, Gyro X/Y/Z)
 * @param handle Driver handle
 * @param data Output data structure
 * @return true if read was successful
 */
bool mpu6050_read_all(Mpu6050Handle_t *handle, Mpu6050Data_t *data);

/**
 * @brief Calibrate gyro and accel offsets by averaging stationary samples
 * @param handle Driver handle
 * @param samples Number of samples to average
 * @return true on success
 */
bool mpu6050_calibrate(Mpu6050Handle_t *handle, uint16_t samples);

/**
 * @brief Perform I2C bus clearing sequence (9 clocks on SCL to unstick slave)
 */
void mpu6050_bus_recovery(void);

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H */
