/**
 * @file sensor_fusion.h
 * @brief Attitude estimation (Complementary filter), EWMA filter, and environmental formulas
 */

#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the attitude structure
 * @param att Pointer to Attitude_t to initialize
 */
void sensor_fusion_init(Attitude_t *att);

/**
 * @brief Update attitude estimation using the Complementary Filter
 * @param att Pointer to current attitude state (updated in place)
 * @param accel Array of 3 floats: Accel X, Y, Z in units of g
 * @param gyro_dps Array of 3 floats: Gyro X, Y, Z in degrees per second
 * @param dt_sec Time elapsed since last update in seconds
 * @param alpha Gyro weighting coefficient (typically 0.96 - 0.98)
 */
void sensor_fusion_update(Attitude_t *att,
                          const float accel[3],
                          const float gyro_dps[3],
                          float dt_sec,
                          float alpha);

/**
 * @brief Calculate Dew Point using the Magnus Formula
 * @param temp_c Temperature in Celsius
 * @param humidity_pct Relative Humidity in % (0 - 100)
 * @return Calculated Dew Point in Celsius
 */
float sensor_fusion_dew_point(float temp_c, float humidity_pct);

/**
 * @brief Calculate Heat Index (feels-like temperature)
 * @param temp_c Temperature in Celsius
 * @param humidity_pct Relative Humidity in % (0 - 100)
 * @return Heat index in Celsius
 */
float sensor_fusion_heat_index(float temp_c, float humidity_pct);

/**
 * @brief Exponential Weighted Moving Average (EWMA) digital low-pass filter
 * @param current_filtered Previous filtered value
 * @param new_sample Current raw sample
 * @param beta Weight of new sample (0.0 < beta <= 1.0)
 * @return Updated filtered value
 */
float sensor_fusion_ewma(float current_filtered, float new_sample, float beta);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_FUSION_H */
