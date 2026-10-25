/* SPDX-License-Identifier: GPL-3.0-only */
/* One TIMER5 producer and one main-loop consumer per input. No allocation.
 * This is an ownership primitive, not a second BLE connection or mixer record.
 * A future peer must receive its own object; never multiplex peers into one. */
#pragma once
#include <stdint.h>

#define SMC_BLE_INPUT_CAPACITY 32u
/* Preset-at-ingress uses spare length bits; valid payload length is <= 6. */
#define SMC_EVENT_USER 0x40u
#define SMC_EVENT_LEN_MASK 0x3Fu
typedef struct {
    uint8_t n, b[6];
} smc_event_t;
typedef struct {
    smc_event_t q[SMC_BLE_INPUT_CAPACITY];
    volatile uint8_t w, r;
    /* Producer-owned: generation | cutoff << 1 | connected. */
    volatile uint32_t link;
    volatile uint32_t release_lost, link_release_boundary;
} smc_ble_input_t;

/* Validate before touching this peer. accepted_release must match the caller's
 * active role/link/preset/channel policy; the queue does not decode MIDI. */
static inline int smc_ble_input_push(smc_ble_input_t *in, const uint8_t *msg,
                                     uint8_t len, int user, int accepted_release)
{
    uint8_t w = in->w;
    if (!msg || !len || len > 6u) return 0;
    if ((uint8_t)(w - in->r) >= SMC_BLE_INPUT_CAPACITY) {
        if (accepted_release) in->release_lost++;
        return 0;
    }
    in->q[w % SMC_BLE_INPUT_CAPACITY].n =
        (uint8_t)(len | (user ? SMC_EVENT_USER : 0u));
    for (uint8_t i = 0; i < len; i++)
        in->q[w % SMC_BLE_INPUT_CAPACITY].b[i] = msg[i];
    __asm__ volatile("" ::: "memory");
    in->w = (uint8_t)(w + 1u);
    return 1;
}

/* Call inside the caller's publication seqlock when other input watermarks
 * accompany this boundary. The consumer never clears producer-owned state.
 * Publish before accepting the new session's first MIDI notification. */
static inline void smc_ble_input_link(smc_ble_input_t *in, int up)
{
    in->link_release_boundary = in->release_lost;
    uint32_t next = (in->link + 0x200u) & ~0x1FFu;
    __asm__ volatile("" ::: "memory");
    in->link = next | ((uint32_t)in->w << 1) | (uint32_t)(up != 0);
}
