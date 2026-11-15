#ifndef TARGET_ARMOR_WIRE_H
#define TARGET_ARMOR_WIRE_H

#include "protocol.h"
#include <string.h>

/* Existing 17-byte target envelope. Bytes 5..8 / 6..9 carry a host ticket,
 * independent of the outer radio sequence and of the motor ARM token. */
enum {
    ARMOR_SET = 0x08, ARMOR_STATUS = 0x09,
    ARMOR_OBSERVED = 0, ARMOR_CONFIRMED = 1, ARMOR_TIMEOUT = 2,
    ARMOR_CAN_ERROR = 3, ARMOR_BUSY = 4, ARMOR_UNKNOWN = 0xff,
    ARMOR_SUPPORTED_FLAG = 0x20, ARMOR_READY_FLAG = 0x40
};

static inline uint32_t armor_ticket(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline bool armor_valid_command(const uint8_t *f)
{
    if (f[0] != PROTO_HEAD || f[1] != 13 || f[2] != ARMOR_SET ||
        f[4] > 2 || !armor_ticket(f + 5) || f[16] != PROTO_TAIL ||
        f[15] != (uint8_t)proto_crc16(f + 1, 14)) return false;
    for (unsigned i = 9; i < 15; ++i)
        if (f[i]) return false;
    return true;
}

static inline void armor_encode_status(uint8_t *f, uint8_t result, uint8_t color,
                                       uint32_t ticket)
{
    memset(f, 0, 17);
    f[0] = PROTO_HEAD;
    f[1] = 13;
    f[2] = ARMOR_STATUS;
    f[4] = result;
    f[5] = color;
    for (unsigned i = 0; i < 4; ++i)
        f[6 + i] = (uint8_t)(ticket >> (8 * i));
    f[15] = (uint8_t)proto_crc16(f + 1, 14);
    f[16] = PROTO_TAIL;
}
#endif
