/* SPDX-License-Identifier: GPL-3.0-only */
/* HCI BLE-MIDI central for the SMC-Mixer. No SDK host or allocation.
 * One central is the proven path. A second central can scan without resetting the first. */
#pragma once
#include <stdint.h>

enum { BLEC_OFF, BLEC_STOPPING, BLEC_STARTING, BLEC_SCANNING, BLEC_CONNECTING,
       BLEC_DISCOVERING, BLEC_CONNECTED, BLEC_DISCONNECTING, BLEC_FAILED };
/* ATT MTU is the 64-byte assembly buffer. One Exchange MTU request follows
 * subscribe. A peer MTU request is answered and does not drop the link. */
enum { BLEC_RX_OK, BLEC_RX_HEADER, BLEC_RX_LINK, BLEC_RX_LENGTH,
       BLEC_RX_FRAGMENT, BLEC_RX_CID, BLEC_RX_SIZE, BLEC_RX_HANDLE, BLEC_RX_MIDI };
typedef int (*blec_send_fn)(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx);
typedef void (*blec_midi_fn)(const uint8_t *p, uint8_t n, void *ctx);

typedef struct {
    blec_send_fn send;
    blec_midi_fn midi;
    void *ctx;
    uint8_t state, step, waiting, att_waiting, credits, acl_total, addr_type, link_name[21];
    uint8_t addr[6], target[6], target_type, found, running, sx[8], sx_len, in_sysex, rx[64], rx_cid;
    uint16_t rx_have, rx_need; /* Oversized signaling is drained; only rx[] is stored. */
    /* TIMER5 owns this bounded reply queue. Entries contain code, identifier, reason. */
    uint8_t sig[4][3], sig_read, sig_count;
    uint8_t mtu_need, mtu_rsp, smp_reply; /* subscribe request, owed MTU answer, owed SMP refusal */
    uint8_t disconnect_flags; /* Transport acceptance and stop intent, not link completion. */
    /* Modulo-256 counters, retained across reconnects; TIMER5 is sole writer. */
    uint8_t rx_acl, rx_notify, rx_midi, rx_reject, rx_reason;
    /* Last rejected ACL header, zero-padded; bounded, no payload log or allocation. */
    uint8_t reject_head[8], reject_reason, att_op;
    uint16_t reject_n, att_handle;
    uint8_t q[32][3], qlen[32];
    volatile uint8_t qr, qw; /* main-loop producer, TIMER5 consumer */
    uint16_t conn, cmd_op, svc_end, query_start, midi_handle, cccd_handle, ts;
    uint32_t deadline, now_ms, drops, errors, connects, sig_deadline;
} blec_t;

void blec_init(blec_t *c, blec_send_fn send, blec_midi_fn midi, void *ctx, const uint8_t addr[6]);
void blec_start(blec_t *c, uint32_t now_ms);
void blec_stop(blec_t *c);
void blec_rx(blec_t *c, uint8_t h4, const uint8_t *p, uint16_t n, uint32_t now_ms);
void blec_poll(blec_t *c, uint32_t now_ms);
int blec_midi_send(blec_t *c, const uint8_t *p, uint8_t n);
/* Join an already-running controller. Enables scan once. Does not send HCI Reset. */
void blec_scan_join(blec_t *c, uint32_t now_ms);
/* While set, a lost scan does not enable a second scan. NULL or 0 restores the one-central path. */
void blec_avoid(const uint8_t addr[6]);
void blec_scan_hold(int hold);
/* Other centrals share this object's controller buffer count. NULL keeps each central on its own count. */
void blec_credit_pool(blec_t *owner);
/* Deliver one controller frame to the central that owns it. The second must already be active. */
void blec_rx_pair(blec_t *a, blec_t *b, uint8_t h4, const uint8_t *p, uint16_t n, uint32_t now_ms);
/* Start a scan on idle when held is connected or discovering and is not waiting for a command. */
void blec_scan_other(blec_t *idle, const blec_t *held, blec_t *pool_owner, uint32_t now_ms);
static inline int blec_connected(const blec_t *c) { return c->state == BLEC_CONNECTED; }
