/* SPDX-License-Identifier: GPL-3.0-only */
#include "ble_midi_bus.h"
#define BLEC_NO_OWNER 255u
static unsigned blecb_rd16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static void blecb_credit_view(blec_bus_t *b)
{
    for (unsigned i = 0; i < BLEC_PEERS; i++) {
        b->peer[i].acl_total = b->total;
        b->peer[i].credits = b->available;
    }
}
static int blecb_send(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx)
{
    blec_bus_context_t *pc = ctx;
    blec_bus_t *b = pc->bus;
    unsigned op;
    if (h4 == 1u) {
        if (n < 3u || p[2] != n - 3u || b->command || !b->command_credit) return -1;
        op = blecb_rd16(p);
        /* Scan parameters/enables, initiation and cancellation belong to one
         * operation, even after Command Status released the HCI command slot. */
        if (op >= 0x200bu && op <= 0x200eu &&
            b->scan_owner != BLEC_NO_OWNER && b->scan_owner != pc->slot) return -1;
        /* Accepted initiation remains owned through the asynchronous result,
         * including failed local cancellation and a successful Cancel Complete. */
        if (b->create_pending && op >= 0x200bu && op <= 0x200du) return -1;
        /* Reset is controller-global. Only initial bootstrap may submit it. */
        if (op == 0x0c03u && (b->ready || pc->slot != 0u)) return -1;
        if (b->send(h4, p, n, b->ctx)) return -1;
        if (op == 0x200du) b->create_pending = 1;
        if (op == 0x200cu && n == 5u) b->scan_value = p[3];
        b->command_deadline = b->peer[pc->slot].now_ms + 3000u;
        b->command_credit--;
        b->command = (uint16_t)op;
        b->command_owner = pc->slot;
        if (op >= 0x200bu && op <= 0x200eu) b->scan_owner = pc->slot;
        return 0;
    }
    if (h4 == 2u) {
        if (n < 4u || blecb_rd16(p + 2) != n - 4u || !b->available ||
            !blec_has_conn(&b->peer[pc->slot]) || (blecb_rd16(p) & 0xfffu) != b->peer[pc->slot].conn) return -1;
        if (b->send(h4, p, n, b->ctx)) return -1;
        b->available--;
        b->outstanding[pc->slot]++;
        return 0;
    }
    return -1;
}
static int blecb_target(const uint8_t *addr, uint8_t type, void *ctx)
{
    blec_bus_context_t *pc = ctx;
    blec_bus_t *b = pc->bus;
    if (b->allowed[pc->slot]) {
        if (type != b->allowed_type[pc->slot]) return 0;
        for (unsigned j = 0; j < 6u; j++) if (addr[j] != b->allowed_addr[pc->slot][j]) return 0;
    } else if (pc->slot != 0u) return 0;
    for (unsigned i = 0; i < BLEC_PEERS; i++) {
        blec_t *c = &b->peer[i];
        if (i == pc->slot || !blec_has_conn(c) || c->target_type != type) continue;
        unsigned j;
        for (j = 0; j < 6u && c->target[j] == addr[j]; j++) {}
        if (j == 6u) return 0;
    }
    return 1;
}
static void blecb_midi(const uint8_t *p, uint8_t n, void *ctx)
{
    blec_bus_context_t *pc = ctx;
    if (pc->bus->midi) pc->bus->midi(pc->slot, p, n, pc->bus->ctx);
}
void blec_bus_init(blec_bus_t *b, blec_send_fn send, blec_bus_midi_fn midi, void *ctx, const uint8_t addr[6])
{
    uint8_t *z = (uint8_t *)b;
    for (unsigned i = 0; i < sizeof *b; i++) z[i] = 0;
    b->send = send; b->midi = midi; b->ctx = ctx;
    b->command_owner = b->scan_owner = BLEC_NO_OWNER;
    b->command_credit = 1;
    for (unsigned i = 0; i < BLEC_PEERS; i++) {
        b->peer_ctx[i].bus = b; b->peer_ctx[i].slot = (uint8_t)i;
        blec_init(&b->peer[i], blecb_send, blecb_midi, &b->peer_ctx[i], addr);
        b->peer[i].target_ok = blecb_target;
    }
}
int blec_bus_allow_peer(blec_bus_t *b, uint8_t peer, const uint8_t addr[6], uint8_t type)
{
    if (b->started || peer >= BLEC_PEERS || !addr || type > 1u) return 0;
    for (unsigned j = 0; j < 6u; j++) b->allowed_addr[peer][j] = addr[j];
    b->allowed_type[peer] = type; b->allowed[peer] = 1;
    return 1;
}
void blec_bus_start(blec_bus_t *b, uint32_t now)
{
    if (b->started) {
        if (b->stopping && !b->command &&
            b->peer[0].state == BLEC_OFF && b->peer[1].state == BLEC_OFF) b->stopping = 0;
        return;
    }
    b->started = 1; b->stopping = 0;
    blec_start(&b->peer[0], now);
}
void blec_bus_stop(blec_bus_t *b, uint32_t now)
{
    b->stopping = 1;
    for (unsigned i = 0; i < BLEC_PEERS; i++) {
        b->peer[i].now_ms = now;
        blec_stop(&b->peer[i]);
    }
}
static void blecb_complete(blec_bus_t *b, const uint8_t *p, unsigned n)
{
    if (n < 3u || p[1] != n - 2u || p[1] != 1u + 4u * p[2]) { b->rejected++; return; }
    for (unsigned k = 0; k < p[2]; k++) {
        unsigned handle = blecb_rd16(p + 3u + 4u*k) & 0xfffu;
        unsigned count = blecb_rd16(p + 5u + 4u*k);
        for (unsigned i = 0; i < BLEC_PEERS; i++) if (blec_has_conn(&b->peer[i]) && b->peer[i].conn == handle) {
            /* Clamp overreport to tracked debt. An in-range duplicate is not
             * distinguishable on the wire: ordered nonduplicating HCI is required. */
            if (count > b->outstanding[i]) { b->rejected++; count = b->outstanding[i]; }
            b->outstanding[i] -= (uint16_t)count;
            b->available = (uint8_t)(b->available + count);
            break;
        }
    }
    blecb_credit_view(b);
}
void blec_bus_rx(blec_bus_t *b, uint8_t h4, const uint8_t *p, uint16_t n, uint32_t now)
{
    unsigned owner = BLEC_NO_OWNER;
    if (h4 == 4u) {
        if (n < 2u || p[1] != n - 2u) { b->rejected++; return; }
        if (p[0] == 0x13u) { blecb_complete(b, p, n); return; }
        /* Core HCI command flow control may announce credits with opcode0
         * without completing an owned command (Complete has no return params). */
        if ((p[0] == 0x0eu && n == 5u && !blecb_rd16(p + 3)) ||
            (p[0] == 0x0fu && n == 6u && !blecb_rd16(p + 4))) {
            b->command_credit = p[p[0] == 0x0eu ? 2u : 3u];
            return;
        }
        if ((p[0] == 0x0eu || p[0] == 0x0fu) && n >= 6u) {
            unsigned op = blecb_rd16(p + (p[0] == 0x0eu ? 3u : 4u));
            unsigned status = p[p[0] == 0x0eu ? 5u : 2u];
            if (!b->command || op != b->command) { b->rejected++; return; }
            owner = b->command_owner;
            b->command_credit = p[p[0] == 0x0eu ? 2u : 3u];
            /* Synchronous commands finish at Complete. Create/Disconnect/Update
             * finish command arbitration at Status, then await their own event. */
            if (p[0] == 0x0eu || status || op == 0x200du || op == 0x0406u || op == 0x2013u) {
                b->command = 0; b->command_owner = BLEC_NO_OWNER; b->command_fault = 0;
            }
            if (op == 0x200du && status) b->create_pending = 0;
            if (op == 0x200cu && p[0] == 0x0eu && !status) b->scan_active = b->scan_value != 0u;
            if (op == 0x2002u && p[0] == 0x0eu && !status && n >= 9u) {
                b->total = b->available = p[8];
            }
        } else if (p[0] == 0x05u && n == 6u && !p[2]) {
            unsigned handle = blecb_rd16(p + 3) & 0xfffu;
            for (unsigned i = 0; i < BLEC_PEERS; i++) if (blec_has_conn(&b->peer[i]) && b->peer[i].conn == handle) {
                owner = i;
                b->available = (uint8_t)(b->available + b->outstanding[i]);
                b->outstanding[i] = 0;
                break;
            }
        } else if (p[0] == 0x3eu && n >= 3u && (p[2] == 1u || p[2] == 0x0au || p[2] == 2u)) {
            if ((p[2] == 1u && n != 21u) || (p[2] == 0x0au && n != 33u)) { b->rejected++; return; }
            owner = b->scan_owner;
            if ((p[2] == 1u || p[2] == 0x0au) && !b->create_pending) { b->rejected++; return; }
            /* Do not admit an unsolicited peer or wrong-role completion. */
            if ((p[2] == 1u || p[2] == 0x0au) && n >= 21u && !p[3] && owner < BLEC_PEERS) {
                blec_t *c = &b->peer[owner];
                if (p[6] != 0u || p[7] != c->target_type) { b->rejected++; return; }
                for (unsigned j = 0; j < 6u; j++) if (p[8+j] != c->target[j]) { b->rejected++; return; }
            }
        } else return;
    } else if (h4 == 2u && n >= 4u) {
        unsigned handle = blecb_rd16(p) & 0xfffu;
        for (unsigned i = 0; i < BLEC_PEERS; i++) if (blec_has_conn(&b->peer[i]) && b->peer[i].conn == handle) { owner = i; break; }
    }
    if (owner >= BLEC_PEERS) { b->rejected++; return; }
    blecb_credit_view(b);
    unsigned old = blec_has_conn(&b->peer[owner]);
    if (h4 == 4u && p[0] == 0x3eu && (p[2] == 1u || p[2] == 0x0au)) {
        blec_t *c = &b->peer[owner];
        b->create_pending = 0;
        /* An eventual success after local failure/cancel still owns a real
         * controller handle. Retire it rather than silently abandoning it. */
        if (!blec_connect_expected(c) || b->stopping)
            blec_abandon_connect(c, now);
    }
    blec_rx(&b->peer[owner], h4, p, n, now);
    if (!old && blec_has_conn(&b->peer[owner])) {
        b->generation[owner]++;
        if (b->scan_owner == owner) b->scan_owner = BLEC_NO_OWNER;
    }
    if (owner == 0u && b->peer[0].state >= BLEC_SCANNING &&
        b->peer[0].state <= BLEC_CONNECTED && b->total) b->ready = 1;
    blecb_credit_view(b);
}
void blec_bus_poll(blec_bus_t *b, uint32_t now)
{
    if (!b->started) return;
    if (b->command && (int32_t)(now - b->command_deadline) >= 0) b->command_fault = 1;
    if (b->scan_owner < BLEC_PEERS) {
        blec_t *c = &b->peer[b->scan_owner];
        if (!b->create_pending && !b->scan_active && !blec_has_conn(c) &&
            (c->state == BLEC_OFF || c->state == BLEC_FAILED) &&
            b->command_owner != b->scan_owner) b->scan_owner = BLEC_NO_OWNER;
    }
    for (unsigned k = 0; k < BLEC_PEERS; k++) {
        unsigned i = (b->turn + k) % BLEC_PEERS;
        blec_t *c = &b->peer[i];
        blecb_credit_view(b);
        if (!b->stopping && b->ready && (i == 0u || b->allowed[i]) &&
            b->scan_owner == BLEC_NO_OWNER && !blec_has_conn(c) &&
            (c->state == BLEC_OFF || c->state == BLEC_FAILED)) blec_join(c, now);
        blec_poll(c, now);
    }
    b->turn ^= 1u;
    blecb_credit_view(b);
}
int blec_bus_midi_send(blec_bus_t *b, uint8_t peer, const uint8_t *p, uint8_t n)
{
    return peer < BLEC_PEERS && !b->stopping && blec_midi_send(&b->peer[peer], p, n);
}
