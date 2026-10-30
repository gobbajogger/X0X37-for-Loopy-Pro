/* SPDX-License-Identifier: GPL-3.0-only */
/* Public HCI regressions. Transport pressure and events are simulated. */
#define main signal_test_main
#include "ble_midi_signal_test.c"
#undef main

static unsigned attempts;
static int abort_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{
    if (t == 1 && n == 6 && p[0] == 6 && p[1] == 4) {
        assert(p[2] == 3 && p[3] == 0x50 && !p[4] && p[5] == 0x13);
        attempts++;
    }
    return send_h4(t, p, n, ctx);
}
static void abort_ready(blec_t *c)
{
    ready(c); c->send = abort_send; c->credits = 0; attempts = 0;
}
static uint32_t abort_begin(blec_t *c, unsigned overflow, uint32_t start)
{
    abort_ready(c); now = start; busy = 1;
    /* An earlier command must not count as acceptance of this Disconnect. */
    c->waiting = 1; c->cmd_op = 0x200d;
    request(c, 1);
    if (overflow) for (unsigned id = 2; id <= 5; id++) request(c, (uint8_t)id);
    else blec_poll(c, now + 3000u);
    assert(c->state == BLEC_DISCONNECTING && c->errors == 1);
    assert(!c->sig_count && !c->att_waiting && !c->waiting);
    assert(attempts == 1 && !sent && c->conn == 0x50 && !c->credits);
    return c->now_ms;
}
static void command_event(blec_t *c, unsigned complete, unsigned status)
{
    const uint8_t done_event[] = {0x0e, 4, 1, 6, 4, (uint8_t)status};
    const uint8_t status_event[] = {0x0f, 4, (uint8_t)status, 1, 6, 4};
    blec_rx(c, 4, complete ? done_event : status_event, 6, c->now_ms);
}
static void down_event(blec_t *c, unsigned status, unsigned handle)
{
    const uint8_t p[] = {5, 4, (uint8_t)status, (uint8_t)handle, 0, 0x13};
    blec_rx(c, 4, p, sizeof p, c->now_ms);
}
static void recovered(unsigned overflow, uint32_t start, unsigned delay, unsigned complete)
{
    blec_t c;
    uint32_t at = abort_begin(&c, overflow, start), deadline = c.deadline;
    if (delay > 1) {
        blec_poll(&c, at + 1); blec_poll(&c, at + delay - 1);
        assert(!sent && c.deadline == deadline && c.state == BLEC_DISCONNECTING);
    }
    busy = 0; blec_poll(&c, at + delay);
    assert(sent == 1 && c.waiting && last_type == 1);
    assert(c.conn == 0x50 && !c.credits && c.deadline == deadline);
    command_event(&c, complete, 0);
    assert(!c.waiting && c.state == BLEC_DISCONNECTING && c.conn == 0x50 && !c.credits);
    unsigned accepted_attempts = attempts;
    blec_poll(&c, deadline - 1);
    assert(sent == 1 && attempts == accepted_attempts && c.deadline == deadline);
    down_event(&c, 0x0c, 0x50); down_event(&c, 0, 0x51);
    assert(c.state == BLEC_DISCONNECTING && c.conn == 0x50 && !c.credits);
    down_event(&c, 0, 0x50);
    assert(c.state == BLEC_SCANNING && !c.conn && c.credits == 5);
    /* Two-peer contract: the rescan's Set Scan Enable holds the HCI command
     * credit until its Command Complete returns it. */
    { const uint8_t scan_done[] = {0x0e, 4, 1, 0x0c, 0x20, 0}; blec_rx(&c, 4, scan_done, sizeof scan_done, c.now_ms); }
    assert(!c.sig_count && !c.rx_need && !c.rx_have);
    /* Reconnect without completion credits from the old link. */
    const uint8_t conn[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    c.state = BLEC_CONNECTING; c.step = 8; c.waiting = 0;
    blec_rx(&c, 4, conn, sizeof conn, c.now_ms);
    assert(c.state == BLEC_DISCOVERING && c.att_waiting && c.credits == 4);
    assert(last_type == 2 && last[6] == 4 && last[8] == 0x10);
    blec_stop(&c); assert(attempts == accepted_attempts + 1);
}
static void failed(unsigned overflow, unsigned accepted, unsigned complete)
{
    blec_t c;
    uint32_t at = abort_begin(&c, overflow, 0xfffffff0u), deadline = c.deadline;
    if (accepted) {
        busy = 0; blec_poll(&c, at + 1); assert(sent == 1);
        command_event(&c, complete, 0x0c);
        assert(c.state == BLEC_FAILED && !c.waiting && c.conn == 0x50 && !c.credits);
    } else {
        blec_poll(&c, at + 1); blec_poll(&c, deadline - 1);
        assert(!sent && c.deadline == deadline && c.state == BLEC_DISCONNECTING);
    }
    unsigned before = attempts;
    busy = 0; blec_poll(&c, deadline); blec_poll(&c, deadline + 10000u);
    assert(c.state == BLEC_FAILED && !c.waiting && c.conn == 0x50 && !c.credits);
    assert(attempts == before); /* No late submission or renewed deadline. */
    down_event(&c, 0, 0x50);
    assert(c.state == BLEC_SCANNING && !c.conn && c.credits == 5);
    /* Two-peer contract: the rescan's Set Scan Enable holds the HCI command
     * credit until its Command Complete returns it. */
    { const uint8_t scan_done[] = {0x0e, 4, 1, 0x0c, 0x20, 0}; blec_rx(&c, 4, scan_done, sizeof scan_done, c.now_ms); }
}
static void accepted_timeout(void)
{
    blec_t c;
    uint32_t at = abort_begin(&c, 1, 100), deadline = c.deadline;
    busy = 0; blec_poll(&c, at + 1); command_event(&c, 0, 0);
    blec_poll(&c, deadline - 1); assert(sent == 1 && c.conn == 0x50 && !c.credits);
    blec_poll(&c, deadline); blec_poll(&c, deadline + 1);
    assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits && sent == 1);
    blec_stop(&c); assert(c.deadline == deadline);
    down_event(&c, 0, 0x50); assert(c.state == BLEC_OFF && c.credits == 5);
    for (unsigned complete = 0; complete <= 1; complete++) {
        abort_ready(&c); blec_stop(&c); assert(sent == 1);
        command_event(&c, complete, 0x0c);
        assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
        blec_stop(&c); blec_poll(&c, c.deadline + 1); assert(sent == 1);
        down_event(&c, 0, 0x50); assert(c.state == BLEC_OFF && !c.conn && c.credits == 5);
    }
}
static void lifecycle_abort(void)
{
    blec_t c;
    for (unsigned accepted = 0; accepted <= 1; accepted++) {
        uint32_t at = abort_begin(&c, 1, 100);
        if (accepted) { busy = 0; blec_poll(&c, at + 1); command_event(&c, 0, 0); }
        unsigned before = attempts;
        uint32_t deadline = c.deadline;
        blec_stop(&c); assert(c.state == BLEC_STOPPING && c.conn == 0x50 && c.deadline == deadline);
        busy = 0; blec_poll(&c, c.now_ms + 1);
        assert(sent == 1 && attempts == before + (accepted ? 0u : 2u));
        command_event(&c, 0, 0); blec_poll(&c, c.deadline - 1);
        assert(sent == 1 && c.conn == 0x50 && !c.credits);
        down_event(&c, 0, 0x50);
        assert(c.state == BLEC_OFF && !c.conn && c.credits == 5);
    }
    abort_ready(&c); busy = 1; blec_stop(&c);
    uint32_t deadline = c.deadline;
    blec_poll(&c, deadline - 1); assert(c.state == BLEC_STOPPING && !sent);
    busy = 0; blec_poll(&c, deadline);
    assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits && !sent);
    blec_stop(&c); blec_poll(&c, deadline + 10000u);
    assert(c.state == BLEC_FAILED && c.deadline == deadline && !sent && c.conn == 0x50);
    down_event(&c, 0, 0x50); assert(c.state == BLEC_OFF && c.credits == 5 && !c.conn);
    blec_start(&c, deadline + 10001u);
    /* The forced connected state includes the returned HCI command credit. */
    c.state = BLEC_CONNECTED; c.step = 14; c.conn = 0x50;c.linked=1; c.waiting = 0; c.command_credit = 1;
    unsigned before = attempts; blec_stop(&c); assert(attempts == before + 1);
    /* A connection can win the cancellation race during stop. */
    abort_ready(&c); c.conn = 0; c.linked = 0; c.state = BLEC_STOPPING; c.deadline = now + 3000; busy = 1;
    const uint8_t conn[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    blec_rx(&c, 4, conn, sizeof conn, now);
    assert(c.conn == 0x50 && !sent && attempts == 1);
    busy = 0; blec_poll(&c, now + 1); assert(sent == 1 && attempts == 2);
    down_event(&c, 0, 0x50); assert(c.state == BLEC_OFF && !c.conn);
}
int main(int argc, char **argv)
{
    for (unsigned overflow = 0; overflow <= 1; overflow++) {
        if (argc > 1 && strcmp(argv[1], overflow ? "overflow" : "timeout")) continue;
        for (unsigned wrap = 0; wrap <= 1; wrap++)
            for (unsigned complete = 0; complete <= 1; complete++) {
                recovered(overflow, wrap ? 0xfffffff0u : 100u, 1, complete);
                recovered(overflow, wrap ? 0xfffffff0u : 100u, 2999, complete);
            }
        failed(overflow, 0, 0);
        failed(overflow, 1, 0); failed(overflow, 1, 1);
    }
    if (argc == 1) { accepted_timeout(); lifecycle_abort(); }
    puts("Disconnect retry, acceptance, deadline, events and lifecycle: PASS");
    return 0;
}
