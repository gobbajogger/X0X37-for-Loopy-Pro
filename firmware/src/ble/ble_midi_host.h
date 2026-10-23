/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * X0X changes: Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
/* A minimal BLE-MIDI peripheral: one connection, a small GATT table, notifications and
 * incoming MIDI written by the central. From felucca-bsp (ble/ble_midi_host, MPL-2.0).
 *
 * It talks to a controller through standard HCI packets (H4 packet types), so it needs no vendor code:
 *   bleh_init()      set the name, the address and the send function
 *   bleh_start()     reset the controller and start advertising (the sequence goes on from bleh_rx)
 *   bleh_rx()        every packet that comes from the controller (event or ACL)
 *   bleh_midi_send() queue one MIDI message (1..3 bytes) with a 13-bit millisecond timestamp
 *   bleh_sysex_send() queue one whole SysEx message (F0 .. F7)
 *   bleh_midi_next() the next message the central wrote (SysEx goes to the sysex callback)
 *   bleh_poll()      call often: packs queued messages into notifications
 *   bleh_radio()     advertising off (and the link down) or on again
 * Fixed: ATT MTU 23, no pairing (a pairing request gets "pairing not supported"), no scanning,
 * one connection. No malloc, no vendor code.
 *
 * X0X changes: a random static address; running status without a timestamp byte; system real
 * time inside SysEx; SysEx in and out; SMP and L2CAP signalling answers; radio off and on. */
#pragma once
#include <stdint.h>

#define BLEH_MTU 23u
#define BLEH_NAME_MAX 20u
#define BLEH_MIDIQ 32u
#define BLEH_INQ 64u                         /* attempt 32: MIDI in (iPhone colour bursts); a power of 2 */
#define BLEH_SYSEX_MAX 64u               /* the longest SysEx bleh_sysex_send takes */

enum { BLEH_H4_CMD = 1, BLEH_H4_ACL = 2, BLEH_H4_EVT = 4 };
enum { BLEH_IDLE = 0, BLEH_STARTING, BLEH_ADVERTISING, BLEH_CONNECTED, BLEH_ERROR };

/* send one packet to the controller: h4_type, then the bytes after the H4 type byte; returns 0 when sent */
typedef int (*bleh_send_fn)(uint8_t h4_type, const uint8_t *p, uint16_t n, void *ctx);
/* incoming SysEx, in pieces: data bytes without F0 / F7; end = 1 with the last piece (F7 seen),
 * end = -1 when the message broke off (another status or a new connection) */
typedef void (*bleh_sysex_fn)(const uint8_t *p, uint32_t n, int end, void *ctx);

typedef struct {
    uint8_t len, b[3];
    uint16_t ts;
} bleh_msg_t;

typedef struct {
    bleh_send_fn send;
    bleh_sysex_fn sysex;
    void *ctx;
    char name[BLEH_NAME_MAX + 1];
    uint8_t namelen;
    uint8_t addr[6];                     /* random static address, little endian (top bits 11) */
    uint8_t state, step, cmd_wait, want_adv, radio_off, tx_running;
    uint16_t cmd_op;
    uint16_t conn;
    uint8_t cccd[2];
    /* Unsent ATT response. An ATT bearer has at most one outstanding request;
     * retry ahead of notifications when the HCI mailbox is full. */
    uint8_t att_rsp[BLEH_MTU], att_rsp_len;
    uint8_t credits, acl_total;
    uint8_t rx[BLEH_MTU + 8];            /* one L2CAP PDU being collected */
    uint8_t rx_have, rx_need, rx_on;
    uint16_t rx_cid;
    bleh_msg_t q[BLEH_MIDIQ];
    uint8_t q_r, q_w;
    bleh_msg_t in_q[BLEH_INQ];
    uint8_t in_r, in_w, in_running, in_sysex;
    uint8_t sx[BLEH_SYSEX_MAX];          /* outgoing SysEx: data bytes between F0 and F7 */
    uint8_t sx_len, sx_pos, sx_on;
    uint16_t sx_ts;
    uint32_t drops_in, sysex_in, smp_rejects;
    uint16_t conn_int, conn_lat;         /* attempt 34: the link's interval (1.25 ms) and latency in use */
    uint8_t cpu_state, cpu_result;
    uint16_t mtu_req;                    /* the MTU the central asked for (last link) */
    uint8_t disc_reason, sig_reqs;       /* the last link's end reason; L2CAP requests from the central */       /* parameter request: 0 not sent, 1 sent; result 0 none, 1 accepted, 2 rejected */
} bleh_t;

void bleh_init(bleh_t *h, bleh_send_fn send, void *ctx, const char *name, const uint8_t addr[6]);
void bleh_set_sysex(bleh_t *h, bleh_sysex_fn fn);
void bleh_start(bleh_t *h);
void bleh_rx(bleh_t *h, uint8_t h4_type, const uint8_t *p, uint16_t n);
int bleh_midi_send(bleh_t *h, const uint8_t *msg, uint8_t len, uint16_t ts13);
int bleh_sysex_send(bleh_t *h, const uint8_t *msg, uint32_t len, uint16_t ts13);
int bleh_midi_next(bleh_t *h, bleh_msg_t *msg);
void bleh_poll(bleh_t *h);
int bleh_conn_update(bleh_t *h, uint16_t min, uint16_t max, uint16_t lat, uint16_t to);
void bleh_radio(bleh_t *h, int on);
static inline int bleh_connected(const bleh_t *h) { return h->state == BLEH_CONNECTED; }
static inline int bleh_notifying(const bleh_t *h) { return h->state == BLEH_CONNECTED && (h->cccd[0] & 1u); }
