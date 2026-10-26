/* Public HCI probe. Transport, clock and peer events are simulated. */
#include "abort-suite.inc"
static void intent_after_failure(unsigned accepted, unsigned complete, unsigned wrap)
{
    blec_t c;
    uint32_t at = abort_begin(&c, wrap, wrap ? 0xfffffff0u : 100u);
    uint32_t cutoff = c.deadline;
    if (accepted) {
        busy = 0; blec_poll(&c, cutoff - 1u);
        command_event(&c, complete, 0x0c);
        assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
    } else blec_poll(&c, cutoff);
    unsigned tries = attempts;
    blec_stop(&c);
    for (unsigned i = 0; i < 5; i++) {
        command_event(&c, complete, 0);
        command_event(&c, !complete, 0x0c);
        blec_stop(&c); blec_poll(&c, cutoff + 1u + i);
        assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
        assert(c.deadline == cutoff && attempts == tries);
    }
    down_event(&c, 1, 0x50); down_event(&c, 0, 0x51);
    assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
    down_event(&c, 0, 0x50);
    assert(c.state == BLEC_OFF && !c.conn && c.credits == c.acl_total);
    assert(!c.waiting && !c.att_waiting && !c.sig_count && !c.rx_need && !c.rx_have);
    /* Old command results cannot restart scanning after the stopped link ends. */
    command_event(&c, 0, 0); command_event(&c, 1, 0);
    blec_poll(&c, cutoff + 10000u);
    assert(c.state == BLEC_OFF && attempts == tries);
    busy = 0; /* The new Reset command uses an available transport. */
    blec_start(&c, cutoff + 10001u);
    assert(c.state == BLEC_STARTING && c.cmd_op == 0x0c03 && !c.conn && !c.credits);
    (void)at;
}
static void status_and_stop(unsigned complete, unsigned wrap)
{
    blec_t c;
    uint32_t at = abort_begin(&c, 1, wrap ? 0xfffffff0u : 100u);
    uint32_t cutoff = c.deadline;
    busy = 0; blec_poll(&c, at + 1u);
    assert(c.waiting && sent == 1);
    unsigned tries = attempts;
    blec_stop(&c); command_event(&c, complete, 0);
    for (unsigned i = 0; i < 4; i++) {
        blec_stop(&c); blec_poll(&c, cutoff - 1u);
        assert(c.state == BLEC_STOPPING && !c.waiting && c.conn == 0x50 && !c.credits);
        assert(c.deadline == cutoff && attempts == tries);
    }
    blec_poll(&c, cutoff);
    assert(c.state == BLEC_FAILED && c.conn == 0x50 && !c.credits);
    down_event(&c, 0, 0x50);
    assert(c.state == BLEC_OFF && c.credits == 5 && !c.conn && attempts == tries);
}
int main(void)
{
    for (unsigned wrap = 0; wrap < 2; wrap++)
        for (unsigned complete = 0; complete < 2; complete++) {
            for (unsigned accepted = 0; accepted < 2; accepted++)
                intent_after_failure(accepted, complete, wrap);
            status_and_stop(complete, wrap);
        }
    puts("12 independent event-order and stop-intent cases: PASS");
    return 0;
}
