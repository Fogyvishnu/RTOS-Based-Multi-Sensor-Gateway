/**
 * @file sensor_fusion.c
 * @brief Implementation of Complementary filter, EWMA, and environmental formulas
 */

#include "sensor_fusion.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define RAD_TO_DEG (180.0f / (float)M_PI)

void sensor_fusion_init(Attitude_t *att) {
    if (!att) return;
    att->roll_deg = 0.0f;
    att->pitch_deg = 0.0f;
    att->roll_accel = 0.0f;
    att->pitch_accel = 0.0f;
    att->initialized = false;
}

void sensor_fusion_update(Attitude_t *att,
                          const float accel[3],
                          const float gyro_dps[3],
                          float dt_sec,
                          float alpha) {
    if (!att || !accel || !gyro_dps || dt_sec <= 0.0f) return;

    /* Clamp alpha */
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    /* Calculate Roll and Pitch from Accelerometer (using gravity vector) */
    /* Roll: rotation around X axis (Y vs Z) */
    float roll_accel = atan2f(accel[1], accel[2]) * RAD_TO_DEG;

    /* Pitch: rotation around Y axis (X vs sqrt(Y^2 + Z^2)) */
    float denom = sqrtf(accel[1] * accel[1] + accel[2] * accel[2]);
    float pitch_accel = atan2f(-accel[0], denom) * RAD_TO_DEG;

    att->roll_accel = roll_accel;
    att->pitch_accel = pitch_accel;

    /* First run initialization if angles are uninitialized */
    if (!att->initialized) {
        att->roll_deg = roll_accel;
        att->pitch_deg = pitch_accel;
        att->initialized = true;
        return;
    }

    /* Complementary Filter equation:
     * angle = alpha * (angle + gyro_rate * dt) + (1 - alpha) * accel_angle
     */
    att->roll_deg = alpha * (att->roll_deg + gyro_dps[0] * dt_sec) + (1.0f - alpha) * roll_accel;
    att->pitch_deg = alpha * (att->pitch_deg + gyro_dps[1] * dt_sec) + (1.0f - alpha) * pitch_accel;
}

float sensor_fusion_dew_point(float temp_c, float humidity_pct) {
    if (humidity_pct <= 0.0f) return -50.0f;
    if (humidity_pct > 100.0f) humidity_pct = 100.0f;

    const float a = 17.27f;
    const float b = 237.7f;

    float alpha = ((a * temp_c) / (b + temp_c)) + logf(humidity_pct / 100.0f);
    float dew_point = (b * alpha) / (a - alpha);
    return dew_point;
}

float sensor_fusion_heat_index(float temp_c, float humidity_pct) {
    /* Convert C to F for standard NOAA Rothfusz equation */
    float T = (temp_c * 1.8f) + 32.0f;
    float RH = humidity_pct;

    if (RH < 0.0f) RH = 0.0f;
    if (RH > 100.0f) RH = 100.0f;

    /* Simple formula */
    float hi_f = 0.5f * (T + 61.0f + ((T - 68.0f) * 1.2f) + (RH * 0.094f));

    /* Full regression if feels like > 80°F */
    if (hi_f >= 80.0f) {
        hi_f = -42.379f + 2.04901523f * T + 10.14333127f * RH
               - 0.22475541f * T * RH - 0.00683783f * T * T
               - 0.05481717f * RH * RH + 0.00122874f * T * T * RH
               + 0.00085282f * T * RH * RH - 0.00000199f * T * T * RH * RH;
    }

    /* Convert back to Celsius */
    return (hi_f - 32.0f) / 1.8f;
}

float sensor_fusion_ewma(float current_filtered, float new_sample, float beta) {
    if (beta <= 0.0f) return current_filtered;
    if (beta >= 1.0f) return new_sample;
    return (beta * new_sample) + ((1.0f - beta) * current_filtered);
}
