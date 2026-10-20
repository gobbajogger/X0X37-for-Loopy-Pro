/* Independent public API check. All HCI and transport inputs are simulated. */
#define main signal_test_main
#include "../../../tests/host/ble_midi_signal_test.c"
#undef main
int main(void)
{
    blec_t c;
    ready(&c); c.credits = 0; busy = 1;
    for (unsigned id = 1; id <= 5; id++) request(&c, (uint8_t)id);
    assert(c.state == BLEC_DISCONNECTING && !sent);
    unsigned deadline = c.deadline;
    const uint8_t unrelated[] = {0xff, 0};
    blec_rx(&c, 4, unrelated, sizeof unrelated, deadline);
    busy = 0;
    blec_stop(&c);
    printf("stop at deadline: now=%u deadline=%u accepted=%u state=%u conn=%u\n",
        c.now_ms, c.deadline, sent, c.state, c.conn);
    fflush(stdout);
    assert(!sent);
    return 0;
}
