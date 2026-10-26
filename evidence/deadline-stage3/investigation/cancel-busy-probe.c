/* Public API probe. Mailbox refusal, clock and HCI events are simulated. */
#define main signal_test_main
#include "../../../tests/host/ble_midi_signal_test.c"
#undef main
int main(void)
{
    blec_t c;
    ready(&c); c.conn = 0; c.state = BLEC_CONNECTING; c.step = 8;
    c.waiting = 0; c.now_ms = 100; busy = 1;
    blec_stop(&c);
    printf("After refused Cancel: state=%u waiting=%u conn=%u accepted=%u deadline=%u\n",
        c.state, c.waiting, c.conn, sent, c.deadline);
    const uint8_t connected[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    busy = 0;
    blec_rx(&c, 4, connected, sizeof connected, 101);
    blec_poll(&c, 101);
    printf("After successful connection: state=%u waiting=%u conn=%u accepted=%u\n",
        c.state, c.waiting, c.conn, sent);
    fflush(stdout);
    assert(c.conn == 0x50); /* The successful connection must remain available for Disconnect. */
    return 0;
}
