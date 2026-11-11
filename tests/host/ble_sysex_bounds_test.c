/* SPDX-License-Identifier: GPL-3.0-only */
/* Boundary and failed-admission tests for the exact BLE-MIDI peripheral source. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_host.h"
static int sent(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{ (void)t; (void)p; (void)n; (void)ctx; return 0; }
int main(void)
{
    bleh_t h;
    uint8_t addr[6] = {0};
    uint8_t short_msg[2] = {0xF0, 0xF7};
    uint8_t maximal[BLEH_SYSEX_MAX + 2] = {0};
    uint8_t note_off[3] = {0x80, 60, 0};
    bleh_init(&h, sent, 0, "X0X", addr);
    /* A length larger than the documented API bound must not read its
     * advertised tail from outside the caller's actual buffer. */
    assert(bleh_sysex_send(&h, short_msg, BLEH_SYSEX_MAX + 3, 7) == -1);
    assert(!h.sx_on && !h.sx_len && !h.sx_pos);
    assert(bleh_sysex_send(&h, short_msg, UINT32_MAX, 7) == -1);
    assert(!h.sx_on && !h.sx_len);
    assert(bleh_sysex_send(&h, 0, 2, 7) == -1);
    assert(bleh_sysex_send(&h, short_msg, 1, 7) == -1);
    assert(bleh_sysex_send(&h, short_msg, 2, 8193) == 0);
    assert(h.sx_on && h.sx_len == 0 && h.sx_ts == 1);
    assert(bleh_sysex_send(&h, short_msg, 2, 1) == -2);
    h.sx_on = 0;
    maximal[0] = 0xF0; maximal[sizeof maximal - 1] = 0xF7;
    for (unsigned i = 1; i + 1 < sizeof maximal; i++) maximal[i] = (uint8_t)(i & 0x7f);
    assert(bleh_sysex_send(&h, maximal, sizeof maximal, 0x1fff) == 0);
    assert(h.sx_on && h.sx_len == BLEH_SYSEX_MAX && h.sx_ts == 0x1fff);
    assert(!memcmp(h.sx, maximal + 1, BLEH_SYSEX_MAX));
    h.sx_on = 0;
    maximal[2] = 0x80;
    assert(bleh_sysex_send(&h, maximal, sizeof maximal, 123) == -1);
    assert(!h.sx_on);
    assert(bleh_midi_send(&h, note_off, 3, 88) == 0);
    assert(h.q_w == 1 && h.q[0].b[0] == 0x80 && h.q[0].b[1] == 60 && h.q[0].b[2] == 0);
    puts("PASS: oversized/null/busy/invalid/max BLE SysEx and preserved Note Off");
    return 0;
}
