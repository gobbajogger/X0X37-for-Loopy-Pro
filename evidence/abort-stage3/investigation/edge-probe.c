/* Public HCI investigation probe. Events and transport are simulated. */
#include "abort-suite.inc"

static void status_order(void)
{
    for (unsigned complete = 0; complete < 2; complete++) {
        blec_t c;
        uint32_t at = abort_begin(&c, 1, 200);
        busy = 0; blec_poll(&c, at + 1);
        unsigned accepted = attempts;
        down_event(&c, 0, 0x50);
        assert(c.state == BLEC_SCANNING && c.conn == 0 && c.credits == 5);
        command_event(&c, complete, 0);
        assert(c.state == BLEC_SCANNING && c.conn == 0 && c.credits == 5);
        assert(attempts == accepted);
        /* A late old command result cannot acknowledge the scan command. */
        assert(c.waiting && c.cmd_op == 0x200c);
    }
}
static void service_stop(void)
{
    for (unsigned complete = 0; complete < 2; complete++) {
        blec_t c;
        uint32_t at = abort_begin(&c, 0, 0xfffffff0u);
        uint32_t deadline = c.deadline;
        busy = 0; blec_poll(&c, at + 1);
        command_event(&c, complete, 0x0c);
        unsigned accepted = attempts;
        /* ble.c calls stop on each failed tick while radio intent is OFF. */
        for (unsigned i = 0; i < 10; i++) {
            blec_stop(&c); blec_poll(&c, at + 2 + i);
            assert(c.state == BLEC_FAILED && c.conn == 0x50);
            assert(c.deadline == deadline && attempts == accepted);
        }
        down_event(&c, 1, 0x50); down_event(&c, 0, 0x51);
        assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
        down_event(&c, 0, 0x50);
        assert(c.state == BLEC_OFF && !c.conn && c.credits == 5);
    }
}
static void cancel_race_deadline(void)
{
    blec_t c;
    abort_ready(&c); c.conn = 0; c.state = BLEC_CONNECTING; c.step = 8;
    c.waiting = 0; c.now_ms = 500; busy = 0;
    blec_stop(&c); assert(c.state == BLEC_STOPPING && c.waiting && c.cmd_op == 0x200e);
    uint32_t deadline = c.deadline;
    const uint8_t conn[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    busy = 1; blec_rx(&c, 4, conn, sizeof conn, deadline - 2);
    assert(c.state == BLEC_STOPPING && c.conn == 0x50 && !c.waiting && c.deadline == deadline);
    busy = 0; blec_poll(&c, deadline - 1);
    assert(c.waiting && c.cmd_op == 0x0406 && attempts == 2 && c.deadline == deadline);
    blec_poll(&c, deadline);
    assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.waiting);
    down_event(&c, 0, 0x50); assert(c.state == BLEC_OFF && !c.conn);
}
int main(void)
{
    status_order(); service_stop(); cancel_race_deadline();
    puts("Event order, repeated service stop, and late cancel race: PASS");
    return 0;
}
