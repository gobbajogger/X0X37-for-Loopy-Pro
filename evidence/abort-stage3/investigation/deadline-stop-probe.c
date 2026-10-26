/* Public API probe. The service case uses ble.c's stop-before-poll order.
 * HCI events, time and transport are simulated. No device access occurs. */
#include "abort-suite.inc"
int main(int argc, char **argv)
{
    blec_t c;
    unsigned offset = argc > 2 && !strcmp(argv[2], "after") ? 1u : 0u;
    uint32_t start = argc > 3 ? 0xfffffff0u : 100u;
    if (argc > 1 && !strcmp(argv[1], "cancel")) {
        abort_ready(&c); c.conn = 0; c.state = BLEC_CONNECTING; c.step = 8;
        c.waiting = 0; c.now_ms = start;
        blec_stop(&c); assert(c.state == BLEC_STOPPING && c.waiting && c.cmd_op == 0x200e);
        uint32_t deadline = c.deadline;
        const uint8_t conn[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        blec_rx(&c, 4, conn, sizeof conn, deadline + offset);
        printf("Cancel race at cutoff: state=%u now=%u deadline=%u attempts=%u accepted_total=%u conn=%u\n",
            c.state,c.now_ms,c.deadline,attempts,sent,c.conn);
        fflush(stdout);
        assert(attempts == 0); /* The original stop interval has expired. */
    } else {
        abort_begin(&c, 1, start);
        uint32_t deadline = c.deadline;
        unsigned service = argc > 1 && !strcmp(argv[1], "service");
        if (service) blec_poll(&c, deadline - 1);
        else {
            const uint8_t unrelated[] = {0xff, 0};
            blec_rx(&c, 4, unrelated, sizeof unrelated, deadline + offset);
        }
        assert(c.state == BLEC_DISCONNECTING && !sent);
        busy = 0;
        /* ble_central_service calls stop before it reads HCI and calls poll. */
        blec_stop(&c);
        printf("Stop at cutoff: state=%u stored_now=%u actual_tick=%u deadline=%u attempts=%u accepted=%u conn=%u\n",
            c.state,c.now_ms,deadline+offset,c.deadline,attempts,sent,c.conn);
        if (service) blec_poll(&c, deadline + offset);
        fflush(stdout);
        assert(sent == 0); /* No submission is permitted at or after the deadline. */
    }
    return 0;
}
