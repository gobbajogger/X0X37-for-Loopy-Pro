/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent simulated HCI checks. No target or device operations occur. */
#define main candidate_signal_main
#include "../../tests/host/ble_midi_signal_test.c"
#undef main
int main(void)
{
    blec_t c;
    const uint8_t p[] = {0x12, 0x6d, 8, 0, 6, 0, 12, 0, 0, 0, 0x90, 1};
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 0x13};
    const uint8_t failed[] = {5, 4, 0x0c, 0x50, 0, 0};
    /* Both PB=0 and PB=2 initial fragments accept the same complete command. */
    ready(&c); acl(&c, 0, 5, p, sizeof p, sizeof p);
    assert(sent == 1); reply(0x13, 0x6d, 1, 0);
    /* Each lifetime boundary removes a partial CID 5 frame and queued reply. */
    for (unsigned how = 0; how != 3; how++) {
        ready(&c); c.credits = 0; request(&c, 1);
        acl(&c, 2, 5, p, 5, sizeof p);
        assert(c.rx_have == 5 && c.rx_need == sizeof p && c.sig_count == 1);
        if (how == 0) blec_start(&c, now);
        else if (how == 1) blec_stop(&c);
        else blec_rx(&c, 4, down, sizeof down, now);
        assert(!c.rx_have && !c.rx_need && !c.sig_count && !c.sig_read);
        c.state = BLEC_CONNECTED; c.step = 14; c.conn = 0x50;
        c.waiting = c.att_waiting = 0; c.credits = 5;
        unsigned before = sent;
        acl(&c, 1, 0, p + 5, sizeof p - 5, 0);
        assert(sent == before && c.rx_reason == BLEC_RX_FRAGMENT);
        request(&c, 0x6d); assert(sent == before + 1); reply(0x13, 0x6d, 1, 0);
    }
    /* A full mailbox retains the reply, with credits unchanged until acceptance. */
    ready(&c); busy = 1; request(&c, 0x6d);
    assert(!sent && c.credits == 5 && c.sig_count == 1);
    blec_poll(&c, now + 2999); assert(blec_connected(&c) && !sent);
    busy = 0; blec_poll(&c, now + 3000);
    assert(c.state == BLEC_DISCONNECTING && c.errors == 1 && !c.sig_count);
    assert(last_type == 1 && last[0] == 6);
    /* Shutdown cannot accept a failed disconnect as credit reclamation. */
    ready(&c); c.credits = 0; blec_stop(&c);
    assert(c.state == BLEC_STOPPING && c.conn == 0x50);
    blec_rx(&c, 4, failed, sizeof failed, now);
    assert(c.state == BLEC_STOPPING && c.conn == 0x50 && !c.credits);
    blec_rx(&c, 4, down, sizeof down, now);
    assert(c.state == BLEC_OFF && !c.conn && c.credits == 5);
    /* A 16-bit completion count cannot wrap before saturation. */
    ready(&c); c.credits = 1; done(&c, 0x50, 65535);
    assert(c.credits == 5);
    puts("PASS: PB0, signaling lifecycle reset, mailbox deadline, shutdown credits and 16-bit saturation");
    return 0;
}
