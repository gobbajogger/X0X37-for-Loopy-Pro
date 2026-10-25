/* SPDX-License-Identifier: GPL-3.0-only */
/* Two direct SMC links, one HCI controller. TIMER5 is the sole bus writer. */
#pragma once
#include "ble_midi_central.h"
#define BLEC_PEERS 2u
struct blec_bus;
typedef struct {
    struct blec_bus *bus;
    uint8_t slot;
} blec_bus_context_t;
typedef void (*blec_bus_midi_fn)(uint8_t peer, const uint8_t *, uint8_t, void *);
typedef struct blec_bus {
    blec_t peer[BLEC_PEERS];
    blec_bus_context_t peer_ctx[BLEC_PEERS];
    blec_send_fn send;
    blec_bus_midi_fn midi;
    void *ctx;
    uint16_t command;
    uint16_t outstanding[BLEC_PEERS];
    uint32_t generation[BLEC_PEERS], rejected, command_deadline;
    uint8_t allowed[BLEC_PEERS], allowed_type[BLEC_PEERS], allowed_addr[BLEC_PEERS][6];
    uint8_t create_pending, scan_active, scan_value;
    uint8_t command_fault; /* Timeout retains owner: never guess a delayed opcode owner. */
    uint8_t command_owner, scan_owner, ready, started, stopping, turn;
    uint8_t total, available, command_credit;
} blec_bus_t;
void blec_bus_init(blec_bus_t *, blec_send_fn, blec_bus_midi_fn, void *, const uint8_t addr[6]);
void blec_bus_start(blec_bus_t *, uint32_t now_ms);
void blec_bus_stop(blec_bus_t *, uint32_t now_ms);
void blec_bus_poll(blec_bus_t *, uint32_t now_ms);
void blec_bus_rx(blec_bus_t *, uint8_t h4, const uint8_t *, uint16_t, uint32_t now_ms);
int blec_bus_midi_send(blec_bus_t *, uint8_t peer, const uint8_t *, uint8_t);

/* Boot/configuration context only, before start. Peer 1 is denied by default. */
int blec_bus_allow_peer(blec_bus_t *, uint8_t peer, const uint8_t addr[6], uint8_t type);
