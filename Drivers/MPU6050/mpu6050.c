/**
 * @file mpu6050.c
 * @brief Implementation of MPU6050 I2C driver and 9-clock bus unsticking algorithm
 */

#include "mpu6050.h"
#include "app_config.h"
#include "fault_manager.h"

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
#include "stm32l4xx_hal.h"
extern I2C_HandleTypeDef hi2c1;
#else
/* Provide stub types for host unit tests/simulation */
typedef struct { int dummy; } I2C_HandleTypeDef;
static I2C_HandleTypeDef hi2c1;
#endif

bool mpu6050_init(Mpu6050Handle_t *handle) {
    if (!handle) return false;
    handle->dev_addr = MPU6050_I2C_ADDR_DEFAULT;
    handle->is_calibrated = false;
    handle->accel_offset[0] = 0.0f;
    handle->accel_offset[1] = 0.0f;
    handle->accel_offset[2] = 0.0f;
    handle->gyro_offset[0] = 0.0f;
    handle->gyro_offset[1] = 0.0f;
    handle->gyro_offset[2] = 0.0f;

    /* Check fault injection */
    if (fault_inject_is_active(FAULT_MPU6050_WHO_AM_I)) {
        fault_manager_set(FAULT_MPU6050_WHO_AM_I);
        return false;
    }

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    uint8_t who_am_i = 0;
    HAL_StatusTypeDef status;

    /* Read WHO_AM_I register (0x75) */
    status = HAL_I2C_Mem_Read(&hi2c1, handle->dev_addr, MPU6050_REG_WHO_AM_I,
                              I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, I2C_TIMEOUT_MS);
    if (status != HAL_OK || who_am_i != MPU6050_WHO_AM_I_VAL) {
        fault_manager_set(FAULT_MPU6050_WHO_AM_I);
        return false;
    }

    /* Wake device from sleep: write 0x00 to PWR_MGMT_1 */
    uint8_t pwr_mgmt = 0x00;
    status = HAL_I2C_Mem_Write(&hi2c1, handle->dev_addr, MPU6050_REG_PWR_MGMT_1,
                               I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, I2C_TIMEOUT_MS);
    if (status != HAL_OK) {
        fault_manager_set(FAULT_MPU6050_COMM_TIMEOUT);
        return false;
    }

    /* Set sample rate divider = 7 (1kHz / (1 + 7) = 125 Hz) */
    uint8_t smplrt = 0x07;
    HAL_I2C_Mem_Write(&hi2c1, handle->dev_addr, MPU6050_REG_SMPLRT_DIV,
                      I2C_MEMADD_SIZE_8BIT, &smplrt, 1, I2C_TIMEOUT_MS);

    /* Set DLPF (Digital Low Pass Filter) to ~44Hz: CONFIG = 0x03 */
    uint8_t config = 0x03;
    HAL_I2C_Mem_Write(&hi2c1, handle->dev_addr, MPU6050_REG_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &config, 1, I2C_TIMEOUT_MS);

    /* Set Gyro Full Scale to +/-250 dps (GYRO_CONFIG = 0x00) */
    uint8_t gyro_cfg = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, handle->dev_addr, MPU6050_REG_GYRO_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &gyro_cfg, 1, I2C_TIMEOUT_MS);

    /* Set Accel Full Scale to +/-2g (ACCEL_CONFIG = 0x00) */
    uint8_t accel_cfg = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, handle->dev_addr, MPU6050_REG_ACCEL_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &accel_cfg, 1, I2C_TIMEOUT_MS);
#endif

    fault_manager_clear(FAULT_MPU6050_WHO_AM_I);
    fault_manager_clear(FAULT_MPU6050_COMM_TIMEOUT);
    return true;
}

bool mpu6050_read_all(Mpu6050Handle_t *handle, Mpu6050Data_t *data) {
    if (!handle || !data) return false;

    if (fault_inject_is_active(FAULT_MPU6050_COMM_TIMEOUT)) {
        data->valid = false;
        fault_manager_set(FAULT_MPU6050_COMM_TIMEOUT);
        return false;
    }

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    uint8_t raw_buf[14];
    HAL_StatusTypeDef status;

    /* Burst read 14 bytes from ACCEL_XOUT_H (0x3B) */
    status = HAL_I2C_Mem_Read(&hi2c1, handle->dev_addr, MPU6050_REG_ACCEL_XOUT_H,
                              I2C_MEMADD_SIZE_8BIT, raw_buf, 14, I2C_TIMEOUT_MS);
    if (status != HAL_OK) {
        data->valid = false;
        fault_manager_set(FAULT_MPU6050_COMM_TIMEOUT);
        return false;
    }

    /* Convert big-endian bytes to signed 16-bit integers */
    data->accel_raw[0] = (int16_t)((raw_buf[0]  << 8) | raw_buf[1]);
    data->accel_raw[1] = (int16_t)((raw_buf[2]  << 8) | raw_buf[3]);
    data->accel_raw[2] = (int16_t)((raw_buf[4]  << 8) | raw_buf[5]);

    int16_t temp_raw   = (int16_t)((raw_buf[6]  << 8) | raw_buf[7]);

    data->gyro_raw[0]  = (int16_t)((raw_buf[8]  << 8) | raw_buf[9]);
    data->gyro_raw[1]  = (int16_t)((raw_buf[10] << 8) | raw_buf[11]);
    data->gyro_raw[2]  = (int16_t)((raw_buf[12] << 8) | raw_buf[13]);

    /* Convert to engineering units */
    data->accel_g[0] = ((float)data->accel_raw[0] / MPU6050_ACCEL_SENS_2G) - handle->accel_offset[0];
    data->accel_g[1] = ((float)data->accel_raw[1] / MPU6050_ACCEL_SENS_2G) - handle->accel_offset[1];
    data->accel_g[2] = ((float)data->accel_raw[2] / MPU6050_ACCEL_SENS_2G) - handle->accel_offset[2];

    /* Temperature formula from MPU6050 datasheet: Temp = (TEMP_OUT / 340) + 36.53 */
    data->temperature_c = ((float)temp_raw / 340.0f) + 36.53f;

    data->gyro_dps[0] = ((float)data->gyro_raw[0] / MPU6050_GYRO_SENS_250DPS) - handle->gyro_offset[0];
    data->gyro_dps[1] = ((float)data->gyro_raw[1] / MPU6050_GYRO_SENS_250DPS) - handle->gyro_offset[1];
    data->gyro_dps[2] = ((float)data->gyro_raw[2] / MPU6050_GYRO_SENS_250DPS) - handle->gyro_offset[2];
#else
    /* Mock simulation data for host tests */
    data->accel_raw[0] = 0;
    data->accel_raw[1] = 0;
    data->accel_raw[2] = 16384;
    data->accel_g[0] = 0.0f;
    data->accel_g[1] = 0.0f;
    data->accel_g[2] = 1.0f;
    data->gyro_raw[0] = 0;
    data->gyro_raw[1] = 0;
    data->gyro_raw[2] = 0;
    data->gyro_dps[0] = 0.0f;
    data->gyro_dps[1] = 0.0f;
    data->gyro_dps[2] = 0.0f;
    data->temperature_c = 25.0f;
#endif

    data->valid = true;
    fault_manager_clear(FAULT_MPU6050_COMM_TIMEOUT);
    return true;
}

bool mpu6050_calibrate(Mpu6050Handle_t *handle, uint16_t samples) {
    if (!handle || samples == 0) return false;

    float sum_accel[3] = {0.0f, 0.0f, 0.0f};
    float sum_gyro[3]  = {0.0f, 0.0f, 0.0f};
    Mpu6050Data_t d;

    /* Temporarily zero offsets during calibration */
    handle->accel_offset[0] = 0.0f;
    handle->accel_offset[1] = 0.0f;
    handle->accel_offset[2] = 0.0f;
    handle->gyro_offset[0]  = 0.0f;
    handle->gyro_offset[1]  = 0.0f;
    handle->gyro_offset[2]  = 0.0f;

    for (uint16_t i = 0; i < samples; i++) {
        if (!mpu6050_read_all(handle, &d)) {
            return false;
        }
        sum_accel[0] += d.accel_g[0];
        sum_accel[1] += d.accel_g[1];
        sum_accel[2] += (d.accel_g[2] - 1.0f); /* 1g should remain on Z */
        sum_gyro[0]  += d.gyro_dps[0];
        sum_gyro[1]  += d.gyro_dps[1];
        sum_gyro[2]  += d.gyro_dps[2];
    }

    handle->accel_offset[0] = sum_accel[0] / (float)samples;
    handle->accel_offset[1] = sum_accel[1] / (float)samples;
    handle->accel_offset[2] = sum_accel[2] / (float)samples;
    handle->gyro_offset[0]  = sum_gyro[0]  / (float)samples;
    handle->gyro_offset[1]  = sum_gyro[1]  / (float)samples;
    handle->gyro_offset[2]  = sum_gyro[2]  / (float)samples;
    handle->is_calibrated = true;

    return true;
}

void mpu6050_bus_recovery(void) {
    fault_manager_increment_i2c_recovery();

#if defined(STM32L433xx) || defined(USE_HAL_DRIVER)
    /* 9-clock I2C unstick sequence for PB8 (SCL) and PB9 (SDA) */
    HAL_I2C_DeInit(&hi2c1);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure SCL and SDA as GPIO open-drain with pull-ups */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);

    /* Pulse SCL 9 times to let slave release SDA */
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        for (volatile int d = 0; d < 100; d++);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        for (volatile int d = 0; d < 100; d++);
    }

    /* Generate STOP condition: SDA low -> high while SCL is high */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    for (volatile int d = 0; d < 100; d++);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    for (volatile int d = 0; d < 100; d++);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    for (volatile int d = 0; d < 100; d++);

    /* Re-initialize I2C1 peripheral */
    HAL_I2C_Init(&hi2c1);
#endif

    fault_manager_clear(FAULT_I2C_BUS_STUCK);
}
