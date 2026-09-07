/**
 * @file task_telemetry.c
 * @brief Implementation of Telemetry Gateway (ANSI Dashboard, JSON, CSV)
 */

#include "task_telemetry.h"
#include "app_config.h"
#include "fault_manager.h"
#include "task_supervisor.h"
#include "bsp_nucleo_l433rc.h"
#include <stdio.h>
#include <string.h>

#if defined(FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
extern QueueHandle_t g_telemetry_queue;
#else
typedef void* QueueHandle_t;
extern QueueHandle_t g_telemetry_queue;
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define pdTRUE 1
static int xQueueReceive(QueueHandle_t q, void *p, TickType_t t) { (void)q; (void)p; (void)t; return 0; }
#endif

static TelemetryMode_t s_telemetry_mode = TELEMETRY_MODE_ANSI;
static char s_tx_buffer[UART_TX_DMA_BUFFER_SIZE];

void task_telemetry_set_mode(TelemetryMode_t mode) {
    s_telemetry_mode = mode;
}

TelemetryMode_t task_telemetry_get_mode(void) {
    return s_telemetry_mode;
}

static void make_gauge(char *buf, size_t len, float val, float min_val, float max_val, int width) {
    if (len < (size_t)(width + 3)) return;
    if (val < min_val) val = min_val;
    if (val > max_val) val = max_val;

    float ratio = (val - min_val) / (max_val - min_val);
    int filled = (int)(ratio * (float)width);

    buf[0] = '[';
    for (int i = 0; i < width; i++) {
        buf[1 + i] = (i < filled) ? '=' : ' ';
    }
    buf[width + 1] = ']';
    buf[width + 2] = '\0';
}

static void make_horizon(char *buf, size_t len, float angle_deg, int width) {
    if (len < (size_t)(width + 3)) return;
    /* angle between -90 and +90 */
    if (angle_deg < -90.0f) angle_deg = -90.0f;
    if (angle_deg > 90.0f) angle_deg = 90.0f;

    int center = width / 2;
    float ratio = angle_deg / 90.0f; /* -1.0 to 1.0 */
    int pos = center + (int)(ratio * (float)center);
    if (pos < 0) pos = 0;
    if (pos >= width) pos = width - 1;

    buf[0] = '[';
    for (int i = 0; i < width; i++) {
        if (i == center && i == pos) {
            buf[1 + i] = '#';
        } else if (i == center) {
            buf[1 + i] = '|';
        } else if (i == pos) {
            buf[1 + i] = '^';
        } else {
            buf[1 + i] = '-';
        }
    }
    buf[width + 1] = ']';
    buf[width + 2] = '\0';
}

static int format_ansi_dashboard(char *out, size_t max_len, const GatewayTelemetry_t *t) {
    char roll_bar[32];
    char pitch_bar[32];
    char pot_bar[32];

    make_horizon(roll_bar, sizeof(roll_bar), t->attitude.roll_deg, 18);
    make_horizon(pitch_bar, sizeof(pitch_bar), t->attitude.pitch_deg, 18);
    make_gauge(pot_bar, sizeof(pot_bar), t->pot.filtered_pct, 0.0f, 100.0f, 18);

    const char *state_str = "UNKNOWN";
    const char *state_color = "\x1b[0m";
    switch (t->system_state) {
        case SYSTEM_STATE_NORMAL:
            state_str = "NORMAL";
            state_color = "\x1b[1;32m"; /* Green */
            break;
        case SYSTEM_STATE_DEGRADED:
            state_str = "DEGRADED";
            state_color = "\x1b[1;33m"; /* Yellow */
            break;
        case SYSTEM_STATE_CRITICAL:
            state_str = "CRITICAL";
            state_color = "\x1b[1;31m"; /* Red */
            break;
        default:
            state_str = "INIT";
            state_color = "\x1b[1;36m";
            break;
    }

    uint32_t hours = t->uptime_sec / 3600;
    uint32_t mins = (t->uptime_sec % 3600) / 60;
    uint32_t secs = t->uptime_sec % 60;

    return snprintf(out, max_len,
        "\x1b[2J\x1b[H" /* Clear screen & Home */
        "================================================================================\r\n"
        "       STM32L433 REAL-TIME MULTI-SENSOR GATEWAY (FreeRTOS ARM Cortex-M4)       \r\n"
        "================================================================================\r\n"
        " State: %s[%s]\x1b[0m  | Uptime: %02u:%02u:%02u | Heap Free: %u B | Faults: 0x%04X\r\n"
        " Watchdog: \x1b[1;32m[ARMED]\x1b[0m | I2C Bus Recoveries: %u\r\n"
        "--------------------------------------------------------------------------------\r\n"
        " [MPU6050] Accel (g):   X:%+0.2f  Y:%+0.2f  Z:%+0.2f | Status: %s\r\n"
        "           Gyro (dps):  X:%+0.1f  Y:%+0.1f  Z:%+0.1f | Die Temp: %.1f C\r\n"
        "           Orientation: Roll:  %+5.1f deg  %s\r\n"
        "                        Pitch: %+5.1f deg  %s\r\n"
        "--------------------------------------------------------------------------------\r\n"
        " [DHT11]   Temp: %.1f C | Humidity: %.1f %% | Dew Point: %.1f C | Feels-Like: %.1f C\r\n"
        "           Status: %s\r\n"
        "--------------------------------------------------------------------------------\r\n"
        " [POT]     ADC Raw: %-4u (%.2f V)  %s  %.1f %%\r\n"
        "================================================================================\r\n"
        " CLI Commands: type 'help' for command list | 'stream [json|ansi|csv]'\r\n"
        " CLI> ",
        state_color, state_str,
        (unsigned int)hours, (unsigned int)mins, (unsigned int)secs,
        (unsigned int)t->free_heap_bytes, t->active_faults,
        (unsigned int)t->i2c_recovery_count,
        t->mpu.accel_g[0], t->mpu.accel_g[1], t->mpu.accel_g[2],
        t->mpu.valid ? "OK" : "\x1b[1;31mERR\x1b[0m",
        t->mpu.gyro_dps[0], t->mpu.gyro_dps[1], t->mpu.gyro_dps[2],
        t->mpu.temperature_c,
        t->attitude.roll_deg, roll_bar,
        t->attitude.pitch_deg, pitch_bar,
        t->dht.temperature_c, t->dht.humidity_pct, t->dht.dew_point_c, t->dht.heat_index_c,
        t->dht.valid ? "OK" : "\x1b[1;31mERR\x1b[0m",
        t->pot.raw_adc, t->pot.voltage_v, pot_bar, t->pot.filtered_pct
    );
}

static int format_json(char *out, size_t max_len, const GatewayTelemetry_t *t) {
    const char *state_str = (t->system_state == SYSTEM_STATE_NORMAL) ? "NORMAL" :
                            (t->system_state == SYSTEM_STATE_DEGRADED) ? "DEGRADED" : "CRITICAL";

    return snprintf(out, max_len,
        "{\"uptime\":%u,\"state\":\"%s\",\"faults\":%u,"
        "\"mpu\":{\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.1f,\"gy\":%.1f,\"gz\":%.1f,\"roll\":%.1f,\"pitch\":%.1f},"
        "\"dht\":{\"temp\":%.1f,\"hum\":%.1f,\"dew\":%.1f,\"hi\":%.1f},"
        "\"pot\":{\"raw\":%u,\"v\":%.2f,\"pct\":%.1f},"
        "\"i2c_rec\":%u}\r\n",
        (unsigned int)t->uptime_sec, state_str, t->active_faults,
        t->mpu.accel_g[0], t->mpu.accel_g[1], t->mpu.accel_g[2],
        t->mpu.gyro_dps[0], t->mpu.gyro_dps[1], t->mpu.gyro_dps[2],
        t->attitude.roll_deg, t->attitude.pitch_deg,
        t->dht.temperature_c, t->dht.humidity_pct, t->dht.dew_point_c, t->dht.heat_index_c,
        t->pot.raw_adc, t->pot.voltage_v, t->pot.filtered_pct,
        (unsigned int)t->i2c_recovery_count
    );
}

static int format_csv(char *out, size_t max_len, const GatewayTelemetry_t *t) {
    return snprintf(out, max_len,
        "%u,%d,%04X,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f\r\n",
        (unsigned int)t->timestamp_ms, (int)t->system_state, t->active_faults,
        t->attitude.roll_deg, t->attitude.pitch_deg,
        t->dht.temperature_c, t->dht.humidity_pct,
        t->pot.voltage_v, t->pot.filtered_pct
    );
}

void Task_Telemetry_Entry(void *argument) {
    (void)argument;
    GatewayTelemetry_t telem;

    for (;;) {
#if defined(FREERTOS) || defined(INC_FREERTOS_H)
        if (xQueueReceive(g_telemetry_queue, &telem, pdMS_TO_TICKS(PERIOD_MS_TELEMETRY)) == pdTRUE) {
#else
        if (0) {
#endif
            int len = 0;
            if (s_telemetry_mode == TELEMETRY_MODE_ANSI) {
                len = format_ansi_dashboard(s_tx_buffer, sizeof(s_tx_buffer), &telem);
            } else if (s_telemetry_mode == TELEMETRY_MODE_JSON) {
                len = format_json(s_tx_buffer, sizeof(s_tx_buffer), &telem);
            } else {
                len = format_csv(s_tx_buffer, sizeof(s_tx_buffer), &telem);
            }

            if (len > 0) {
                bsp_uart_send_dma((const uint8_t*)s_tx_buffer, (uint16_t)len);
            }
        }

        /* Report healthy check-in to supervisor */
        task_supervisor_check_in(ALIVE_BIT_TELEMETRY);
    }
}
