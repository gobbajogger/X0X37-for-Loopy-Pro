/* SPDX-License-Identifier: GPL-3.0-only
 * Host regression: full core-to-controller HCI mailbox must not consume queued
 * BLE-MIDI Note Off, SysEx, ACL credits, or pending HCI-command state.
 *
 * gcc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined \
 *   -Ifirmware/hal -Ifirmware/src/ble tests/ble_mailbox_backpressure_test.c \
 *   firmware/src/ble/ble_midi_host.c -o /tmp/ble_mailbox_backpressure_test
 */
#include <stdio.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "ble_midi_host.h"

static int failed, checks;
#define CHECK(x) do { checks++; if (!(x)) { failed++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); } } while (0)

static xb_ring_t mailbox;
static int mailbox_send(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    return xb_frame_put((xb_ring_t *)ctx, type, p, n);
}
static void fill_mailbox(void)
{
    const uint8_t d = 0;
    unsigned i;
    xb_ring_init(&mailbox);
    for (i = 0; i < 255; i++) CHECK(xb_frame_put(&mailbox, 0x7F, &d, 1) == 0);
    CHECK(xb_ring_free(&mailbox) == 3);
}
static void drain_mailbox(void)
{
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    do {
        n = xb_frame_get(&mailbox, &type, d, sizeof d);
    } while (n > 0);
    CHECK(n == 0);
}
static int pop(uint8_t *type, uint8_t *data, unsigned cap)
{
    return xb_frame_get(&mailbox, type, data, cap);
}
static void connected(bleh_t *h)
{
    bleh_init(h, mailbox_send, &mailbox, "X0X37", 0);
    h->state = BLEH_CONNECTED;
    h->conn = 0x0040;
    h->cccd[0] = 1;
    h->acl_total = h->credits = 4;
}

static void test_noteoff_retained(void)
{
    bleh_t h;
    const uint8_t off[] = {0x80, 60, 0};
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    connected(&h);
    CHECK(bleh_midi_send(&h, off, sizeof off, 0x0456) == 0);
    bleh_poll(&h);
    CHECK(h.q_r == 0 && h.q_w == 1);            /* mailbox rejected Note Off: retry later */
    CHECK(h.credits == 4);                      /* rejected packet consumes no HCI ACL credit */
    drain_mailbox();
    bleh_poll(&h);
    CHECK(h.q_r == 1 && h.credits == 3);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_ACL && n == 16);
    if (n == 16) CHECK(d[8] == 27 && d[13] == 0x80 && d[14] == 60 && d[15] == 0);
    bleh_poll(&h);
    CHECK(pop(&type, d, sizeof d) == 0);       /* accepted Note Off must not duplicate */
}

static void test_sysex_retained(void)
{
    bleh_t h;
    const uint8_t sx[] = {0xF0, 0x7D, 0x22, 0xF7};
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    connected(&h);
    CHECK(bleh_sysex_send(&h, sx, sizeof sx, 0x0102) == 0);
    bleh_poll(&h);
    CHECK(h.sx_on == 1 && h.sx_pos == 0 && h.credits == 4);
    drain_mailbox();
    bleh_poll(&h);
    CHECK(h.sx_on == 0 && h.credits == 3);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_ACL && n == 18);
    if (n == 18) CHECK(d[13] == 0xF0 && d[14] == 0x7D && d[15] == 0x22 && d[17] == 0xF7);
    bleh_poll(&h);
    CHECK(pop(&type, d, sizeof d) == 0);
}

static void test_startup_retry(void)
{
    bleh_t h;
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    bleh_init(&h, mailbox_send, &mailbox, "X0X37", 0);
    bleh_start(&h);
    CHECK(h.state == BLEH_STARTING && h.cmd_wait == 0);
    drain_mailbox();
    bleh_poll(&h);
    CHECK(h.cmd_wait == 1 && h.cmd_op == 0x0C03);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_CMD && n == 3);
    if (n == 3) CHECK(d[0] == 3 && d[1] == 12 && d[2] == 0);
}

static void test_advertising_retry(void)
{
    bleh_t h;
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    bleh_init(&h, mailbox_send, &mailbox, "X0X37", 0);
    h.state = BLEH_ADVERTISING;
    bleh_poll(&h);
    CHECK(h.want_adv == 0 && h.cmd_wait == 0);
    drain_mailbox();
    bleh_poll(&h);
    CHECK(h.want_adv == 1 && h.cmd_wait == 1);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_CMD && n == 4);
    if (n == 4) CHECK(d[0] == 0x0A && d[1] == 0x20 && d[3] == 1);
}

static void test_conn_update_retry(void)
{
    bleh_t h;
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    connected(&h);
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0);
    CHECK(h.cpu_state == 0 && h.credits == 4);
    drain_mailbox();
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 1);
    CHECK(h.cpu_state == 1 && h.credits == 3);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_ACL && n == 20);
    if (n == 20) CHECK(d[8] == 0x12 && d[9] == 0x31);
}


static void test_disconnection_drops_old_midi(void)
{
    bleh_t h;
    const uint8_t off[] = {0x80, 60, 0};
    const uint8_t disc_evt[] = {0x05, 0x04, 0x00, 0x40, 0x00, 0x13};
    uint8_t type, d[XB_FRAME_MAX];
    fill_mailbox();
    connected(&h);
    CHECK(bleh_midi_send(&h, off, sizeof off, 0x0123) == 0);
    bleh_poll(&h);
    CHECK(h.q_w != h.q_r);                  /* still queued after transport refusal */
    bleh_rx(&h, BLEH_H4_EVT, disc_evt, sizeof disc_evt);
    CHECK(h.state == BLEH_ADVERTISING && h.q_w == h.q_r);
    drain_mailbox();
    /* Advertising restart is allowed, but stale Note Off must not be replayed. */
    bleh_poll(&h);
    CHECK(pop(&type, d, sizeof d) == 0 || type == BLEH_H4_CMD);
}

static void test_disconnect_command_retries(void)
{
    bleh_t h;
    uint8_t type, d[XB_FRAME_MAX];
    int n;
    fill_mailbox();
    connected(&h);
    bleh_radio(&h, 0);
    bleh_poll(&h);
    CHECK(h.cmd_wait == 0);
    drain_mailbox();
    bleh_poll(&h);
    CHECK(h.cmd_wait == 1 && h.cmd_op == 0x0406);
    n = pop(&type, d, sizeof d);
    CHECK(type == BLEH_H4_CMD && n == 6);
    if (n == 6) CHECK(d[0] == 0x06 && d[1] == 0x04 && d[2] == 3 && d[5] == 0x13);
}

int main(void)
{
    test_noteoff_retained();
    test_sysex_retained();
    test_startup_retry();
    test_advertising_retry();
    test_conn_update_retry();
    test_disconnection_drops_old_midi();
    test_disconnect_command_retries();
    printf("ble mailbox backpressure: %d checks, %d failures\n", checks, failed);
    return failed ? 1 : 0;
}
