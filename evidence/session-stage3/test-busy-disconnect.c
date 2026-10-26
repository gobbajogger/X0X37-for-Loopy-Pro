/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent public HCI probe for a required disconnect after mailbox pressure. */
#define main candidate_signal_main
#include "../../tests/host/ble_midi_signal_test.c"
#undef main
static int probe(unsigned overflow)
{
    blec_t c;
    ready(&c); busy = 1;
    if (overflow) {
        c.credits = 0;
        for (unsigned id = 1; id <= 5; id++) request(&c, (uint8_t)id);
    } else {
        request(&c, 1);
        blec_poll(&c, now + 3000);
    }
    assert(c.state == BLEC_DISCONNECTING && c.errors == 1 && !c.sig_count && !sent);
    unsigned failed_at = c.now_ms;
    busy = 0;
    blec_poll(&c, failed_at + 1);
    blec_poll(&c, failed_at + 2999);
    unsigned retried = sent && last_type == 1 && last[0] == 6;
    blec_poll(&c, failed_at + 3000);
    printf("%s: disconnect retry=%u sent=%u state=%u waiting=%u conn=%u errors=%u\n",
           overflow ? "queue overflow" : "queue timeout", retried, sent,
           c.state, c.waiting, c.conn, c.errors);
    if (!retried) puts("FAIL: mailbox accepted later polls, but no HCI Disconnect was submitted");
    return !retried;
}
int main(void)
{
    unsigned failures = (unsigned)probe(0) + (unsigned)probe(1);
    return failures ? 1 : 0;
}
