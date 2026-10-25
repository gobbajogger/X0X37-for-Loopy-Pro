/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#define SMC_DIRECT_PEERS 2u
/* TIMER5 producer: validate peer before indexing; publish link before ingress. */
void smc_ble_midi_peer(uint8_t peer, const uint8_t *msg, uint8_t len);
void smc_ble_link_peer(uint8_t peer, int up);
/* Main-loop neutral page target lock. Returns zero for invalid slots. */
int smc_peer_lock(uint8_t peer, int locked);
int smc_peer_locked(uint8_t peer);
