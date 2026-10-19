/* SPDX-License-Identifier: GPL-3.0-only */
/* One-link HCI BLE-MIDI central for the SMC-Mixer. No SDK host or allocation. */
#pragma once
#include <stdint.h>

enum { BLEC_OFF, BLEC_STARTING, BLEC_SCANNING, BLEC_CONNECTING,
       BLEC_DISCOVERING, BLEC_CONNECTED, BLEC_DISCONNECTING, BLEC_FAILED };
typedef int (*blec_send_fn)(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx);
typedef void (*blec_midi_fn)(const uint8_t *p, uint8_t n, void *ctx);

typedef struct {
    blec_send_fn send;
    blec_midi_fn midi;
    void *ctx;
    uint8_t state, step, waiting, att_waiting, credits, acl_total, addr_type, link_name[21];
    uint8_t addr[6], target[6], target_type, found, running, sx[8], sx_len, in_sysex, rx[64], rx_have, rx_need;
    uint8_t q[32][3], qlen[32], qr, qw;
    uint16_t conn, cmd_op, svc_end, query_start, midi_handle, cccd_handle, ts;
    uint32_t deadline, now_ms, drops, errors, connects;
} blec_t;

void blec_init(blec_t *c, blec_send_fn send, blec_midi_fn midi, void *ctx, const uint8_t addr[6]);
void blec_start(blec_t *c, uint32_t now_ms);
void blec_stop(blec_t *c);
void blec_rx(blec_t *c, uint8_t h4, const uint8_t *p, uint16_t n, uint32_t now_ms);
void blec_poll(blec_t *c, uint32_t now_ms);
int blec_midi_send(blec_t *c, const uint8_t *p, uint8_t n);
static inline int blec_connected(const blec_t *c) { return c->state == BLEC_CONNECTED; }
