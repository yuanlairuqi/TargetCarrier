#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t uptime;
    uint16_t current_hz,target_hz;uint8_t stage;
    uint8_t motion, speed, error, source, stm_motion, stm_error, direction, limits;
    bool ble, stm_online, confirmed, output_valid, latched;
} oled_status_t;
void oled_status_publish(const oled_status_t *s);
void oled_status_start(void);
