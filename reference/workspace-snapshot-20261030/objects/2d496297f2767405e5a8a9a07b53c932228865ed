/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: an accepted new LE connection is a new BLE-MIDI epoch.
 * Queued Note Off and outgoing SysEx from before that link must never replay.
 * Native full-source command (when the exact Git root is available):
 *   cc -std=c99 -O1 -g -Wall -Wextra -Werror -Ifirmware/src/ble \
 *     tests/host/ble_link_epoch_reset_test.c firmware/src/ble/ble_midi_host.c \
 *     -o /tmp/ble_link_epoch_reset_test && /tmp/ble_link_epoch_reset_test
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %u: %s\n",(unsigned)__LINE__,#x); } } while (0)
static int send_packet(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{ (void)type; (void)p; (void)n; (void)ctx; return 0; }
static void setup(bleh_t *h)
{
    bleh_init(h, send_packet, NULL, "X0X37", NULL);
    h->state = BLEH_ADVERTISING;
    h->acl_total = 4;
}
static void dirty(bleh_t *h)
{
    h->q_r = 3; h->q_w = 6; /* queued Note Off and other messages */
    h->sx_on = 3; h->sx_len = 5; h->sx_pos = 2;
    h->cccd[0] = 1; h->att_rsp_len = 3;
    h->rx_on = 1; h->in_r = 3; h->in_w = 4; h->in_running = 0x90;
    h->cpu_state = 1;
}
static void accepted(bleh_t *h, unsigned conn)
{
    /* LE Meta Event, legacy Connection Complete: success, handle, peripheral role. */
    uint8_t event[7] = {0x3e, 5, 1, 0, (uint8_t)conn, (uint8_t)(conn >> 8), 1};
    bleh_rx(h, BLEH_H4_EVT, event, sizeof event);
}
static void test_new_advertising_link(void)
{
    bleh_t h;
    setup(&h); dirty(&h);
    accepted(&h, 0x0041);
    CHECK(h.state == BLEH_CONNECTED);
    CHECK(h.conn == 0x0041);
    CHECK(h.q_r == h.q_w); /* must not send Note Off from before link */
    CHECK(h.sx_on == 0);    /* must not restart old SysEx fragment */
    CHECK(h.sx_len == 0 && h.sx_pos == 0);
    CHECK(h.cccd[0] == 0 && h.att_rsp_len == 0);
    CHECK(h.rx_on == 0 && h.in_r == h.in_w);
    CHECK(h.cpu_state == 0);
}
static void test_new_handle_replaces_old_connection(void)
{
    bleh_t h;
    setup(&h); h.state = BLEH_CONNECTED; h.conn = 0x0040; dirty(&h);
    accepted(&h, 0x0042);
    CHECK(h.conn == 0x0042);
    CHECK(h.q_r == h.q_w);
    CHECK(h.sx_on == 0 && h.sx_len == 0 && h.sx_pos == 0);
}
static void test_failed_connection_does_not_erase_outbox(void)
{
    bleh_t h;
    const uint8_t failed[] = {0x3e, 5, 1, 1, 0x43, 0, 1};
    setup(&h); dirty(&h);
    bleh_rx(&h, BLEH_H4_EVT, failed, sizeof failed);
    CHECK(h.state == BLEH_ADVERTISING);
    CHECK(h.q_r == 3 && h.q_w == 6);
    CHECK(h.sx_on == 3 && h.sx_len == 5 && h.sx_pos == 2);
}
static void test_disconnection_and_unrelated_handle(void)
{
    bleh_t h;
    const uint8_t unrelated[] = {0x05, 4, 0, 0x43, 0, 0x13};
    const uint8_t matched[] = {0x05, 4, 0, 0x41, 0, 0x13};
    setup(&h); h.state = BLEH_CONNECTED; h.conn = 0x41; dirty(&h);
    bleh_rx(&h, BLEH_H4_EVT, unrelated, sizeof unrelated);
    CHECK(h.state == BLEH_CONNECTED);
    CHECK(h.q_r == 3 && h.sx_on == 3);
    bleh_rx(&h, BLEH_H4_EVT, matched, sizeof matched);
    CHECK(h.state == BLEH_ADVERTISING);
    CHECK(h.q_r == h.q_w && h.sx_on == 0);
}
static void test_truncated_event_ignored(void)
{
    bleh_t h;
    const uint8_t truncated[] = {0x3e, 5, 1, 0, 0x44};
    setup(&h); dirty(&h);
    bleh_rx(&h, BLEH_H4_EVT, truncated, sizeof truncated);
    CHECK(h.state == BLEH_ADVERTISING);
    CHECK(h.q_r == 3 && h.q_w == 6 && h.sx_on == 3);
}
int main(void)
{
    test_new_advertising_link();
    test_new_handle_replaces_old_connection();
    test_failed_connection_does_not_erase_outbox();
    test_disconnection_and_unrelated_handle();
    test_truncated_event_ignored();
    printf("BLE link-epoch reset: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
