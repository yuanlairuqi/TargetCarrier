#ifndef TARGET_ARMOR_CAN_H
#define TARGET_ARMOR_CAN_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t ticket;
    uint8_t result;
    uint8_t color;
} armor_result_t;

void armor_can_init(void);
bool armor_can_ready(void);
uint8_t armor_can_color(void);
void armor_can_cancel(void);
bool armor_can_request(uint8_t color, uint32_t ticket, uint32_t now, armor_result_t *reply);
bool armor_can_poll(uint32_t now, armor_result_t *reply);

#endif
