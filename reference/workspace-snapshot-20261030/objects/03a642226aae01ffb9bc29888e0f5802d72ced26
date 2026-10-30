/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Stock User preset MMC commands: 32 strip keys in M/S/R/SELECT order, then
 * eleven global keys. The DAW note values are the same button identifiers. */
static inline uint8_t smc_mmc_note(uint8_t cmd)
{
    static const uint8_t base[4] = {0x10, 0x08, 0x00, 0x18};
    static const uint8_t tail[11] = {0x5E, 0x5D, 0x5F, 0x5B, 0x5C, 0x2E, 0x2F,
                                      0x60, 0x61, 0x62, 0x63};
    if (cmd >= 1u && cmd <= 32u)
        return (uint8_t)(base[(cmd - 1u) & 3u] + ((cmd - 1u) >> 2));
    return cmd >= 33u && cmd <= 43u ? tail[cmd - 33u] : 0xFFu;
}

static inline int smc_pickup_reached(uint8_t prior, int seen, uint8_t target, uint8_t current)
{
    if (seen && ((prior < target) != (current < target)))
        return 1;
    return current + 2u >= target && current <= target + 2u;
}

static inline int smc_relative(uint8_t value)
{
    return (value & 0x40u) ? -(int)(value & 0x3Fu) : (int)(value & 0x3Fu);
}

/* Host USB/peripheral BLE has no controller identity: opt-in User mode and
 * channel 16 isolate these messages from instruments and APC40 feedback. */
static inline int smc_host_accept(uint8_t status, uint8_t data1, int user_mode)
{
    uint8_t type = status & 0xF0u, a = data1 & 0x7Fu;
    if (!user_mode || (status & 15u) != 15u)
        return 0;
    if (type == 0xB0u)
        return (a >= 16u && a < 24u) || (a >= 40u && a < 48u);
    if (type != 0x90u && type != 0x80u)
        return 0;
    return a < 32u || a == 0x2Eu || a == 0x2Fu || a == 0x30u || a == 0x31u ||
           (a >= 0x5Bu && a <= 0x5Fu) || (a >= 0x60u && a <= 0x63u);
}
