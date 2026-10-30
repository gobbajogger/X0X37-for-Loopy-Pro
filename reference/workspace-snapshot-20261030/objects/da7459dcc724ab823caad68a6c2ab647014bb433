/* SPDX-License-Identifier: GPL-3.0-only
 * BLE-MIDI disconnect failure must not discard queued Note Off/SysEx or
 * strand radio-off after an HCI disconnect command is rejected.
 * Native full-source command (exact Git checkout required):
 *   cc -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble \
 *     tests/host/ble_disconnect_status_test.c firmware/src/ble/ble_midi_host.c \
 *     -o /tmp/ble_disconnect_status_test && /tmp/ble_disconnect_status_test
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    fprintf(stderr, "FAIL line %u: %s\n", (unsigned)__LINE__, #x); } } while (0)

typedef struct {
    unsigned commands, disconnects;
    uint16_t last_op;
} bus_t;

static int send_packet(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    bus_t *bus = ctx;
    if (type == BLEH_H4_CMD && n >= 3u) {
        bus->commands++;
        bus->last_op = (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8);
        if (bus->last_op == 0x0406u) bus->disconnects++;
    }
    return 0;
}

static void setup(bleh_t *h, bus_t *bus)
{
    memset(bus, 0, sizeof *bus);
    bleh_init(h, send_packet, bus, "X0X37", NULL);
    h->state = BLEH_CONNECTED;
    h->conn = 0x0041;
    h->acl_total = 4;
    h->credits = 4;
    h->q_r = 3; h->q_w = 6;
    h->sx_on = 3; h->sx_len = 5; h->sx_pos = 2;
    bleh_radio(h, 0);
}

static void rejected_command_status_is_retryable(void)
{
    bleh_t h; bus_t bus;
    static const uint8_t denied[] = {0x0F, 4, 0x0C, 1, 0x06, 0x04};
    static const uint8_t accepted[] = {0x0F, 4, 0, 1, 0x06, 0x04};
    setup(&h, &bus);
    bleh_poll(&h);
    CHECK(bus.disconnects == 1 && h.cmd_op == 0x0406u && h.cmd_wait == 1);
    bleh_rx(&h, BLEH_H4_EVT, denied, sizeof denied);
    CHECK(h.state == BLEH_CONNECTED && h.q_r == 3 && h.q_w == 6 && h.sx_on == 3);
    CHECK(h.cmd_wait == 0 && h.cmd_op == 0);
    bleh_poll(&h);
    CHECK(bus.disconnects == 2 && h.cmd_wait == 1 && h.cmd_op == 0x0406u);
    bleh_rx(&h, BLEH_H4_EVT, accepted, sizeof accepted);
    CHECK(h.cmd_wait == 0 && h.cmd_op == 0x0406u);
    bleh_poll(&h);
    CHECK(bus.disconnects == 2); /* accepted command awaits link teardown */
}

static void unsuccessful_disconnect_event_preserves_live_link(void)
{
    bleh_t h; bus_t bus;
    static const uint8_t denied[] = {0x05, 4, 0x0C, 0x41, 0, 0x13};
    static const uint8_t success[] = {0x05, 4, 0, 0x41, 0, 0x13};
    setup(&h, &bus);
    bleh_poll(&h);
    bleh_rx(&h, BLEH_H4_EVT, denied, sizeof denied);
    CHECK(h.state == BLEH_CONNECTED && h.conn == 0x0041);
    CHECK(h.q_r == 3 && h.q_w == 6);
    CHECK(h.sx_on == 3 && h.sx_len == 5 && h.sx_pos == 2);
    CHECK(h.disc_reason == 0 && h.cmd_op == 0 && h.cmd_wait == 0);
    bleh_poll(&h);
    CHECK(bus.disconnects == 2 && h.cmd_op == 0x0406u && h.cmd_wait == 1);
    bleh_rx(&h, BLEH_H4_EVT, success, sizeof success);
    CHECK(h.state == BLEH_ADVERTISING && h.disc_reason == 0x13);
    CHECK(h.q_r == h.q_w && h.sx_on == 0 && h.sx_len == 0 && h.sx_pos == 0);
}

static void unrelated_status_or_disconnect_does_not_change_pending_command(void)
{
    bleh_t h; bus_t bus;
    static const uint8_t other_handle[] = {0x05, 4, 0x0C, 0x42, 0, 0x13};
    static const uint8_t other_opcode[] = {0x0F, 4, 0x0C, 1, 0x00, 0x20};
    static const uint8_t truncated[] = {0x05, 4, 0x0C, 0x41};
    setup(&h, &bus);
    bleh_poll(&h);
    bleh_rx(&h, BLEH_H4_EVT, other_handle, sizeof other_handle);
    CHECK(h.state == BLEH_CONNECTED && h.cmd_op == 0x0406u && h.cmd_wait == 1);
    bleh_rx(&h, BLEH_H4_EVT, other_opcode, sizeof other_opcode);
    CHECK(h.cmd_op == 0x0406u && h.cmd_wait == 1);
    bleh_rx(&h, BLEH_H4_EVT, truncated, sizeof truncated);
    CHECK(h.state == BLEH_CONNECTED && h.cmd_wait == 1 && h.q_r == 3);
}

static void advertising_failure_is_ignored_but_success_restarts_advertising(void)
{
    bleh_t h; bus_t bus;
    static const uint8_t denied[] = {0x05, 4, 0x0C, 0x41, 0, 0x13};
    static const uint8_t success[] = {0x05, 4, 0, 0x41, 0, 0x13};
    setup(&h, &bus);
    h.state = BLEH_ADVERTISING; h.want_adv = 1;
    bleh_rx(&h, BLEH_H4_EVT, denied, sizeof denied);
    CHECK(h.state == BLEH_ADVERTISING && h.want_adv == 1 && h.disc_reason == 0);
    bleh_rx(&h, BLEH_H4_EVT, success, sizeof success);
    CHECK(h.state == BLEH_ADVERTISING && h.want_adv == 0);
}

int main(void)
{
    rejected_command_status_is_retryable();
    unsuccessful_disconnect_event_preserves_live_link();
    unrelated_status_or_disconnect_does_not_change_pending_command();
    advertising_failure_is_ignored_but_success_restarts_advertising();
    printf("BLE disconnect failure: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
