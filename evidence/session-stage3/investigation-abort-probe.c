/* Public HCI probe. Synthetic transport backpressure; no physical-device claim. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"
static unsigned busy, disconnects, attempts;
static int send_h4(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)n; (void)ctx;
    if (h4 == 1 && p[0] == 6 && p[1] == 4) attempts++;
    if (busy) return -1;
    if (h4 == 1 && p[0] == 6 && p[1] == 4) disconnects++;
    return 0;
}
static void request(blec_t *c, unsigned id, unsigned now)
{
    const uint8_t p[] = {0x50, 0x20, 16, 0, 12, 0, 5, 0,
        0x12, (uint8_t)id, 8, 0, 6, 0, 12, 0, 0, 0, 0x90, 1};
    blec_rx(c, 2, p, sizeof p, now);
}
int main(int argc, char **argv)
{
    const uint8_t addr[6] = {0};
    blec_t c;
    blec_init(&c, send_h4, 0, 0, addr);
    c.state = BLEC_CONNECTED; c.step = 14; c.conn = 0x50;
    c.acl_total = 5; c.credits = 0;
    busy = 1;
    if (argc > 1 && !strcmp(argv[1], "timeout")) {
        request(&c, 1, 100);
        blec_poll(&c, 3100);
    } else {
        for (unsigned id = 1; id <= 5; id++) request(&c, id, 100);
    }
    assert(c.state == BLEC_DISCONNECTING && c.errors == 1 && attempts == 1 && !disconnects);
    unsigned recover = c.now_ms + 1;
    busy = 0;
    blec_poll(&c, recover);
    printf("Recovered transport before deadline: state=%u conn=%u attempts=%u accepted_disconnects=%u\n",
        c.state, c.conn, attempts, disconnects);
    blec_poll(&c, c.deadline - 1);
    printf("Before deadline: state=%u conn=%u attempts=%u accepted_disconnects=%u\n",
        c.state, c.conn, attempts, disconnects);
    blec_poll(&c, c.deadline);
    printf("At deadline: state=%u conn=%u attempts=%u accepted_disconnects=%u\n",
        c.state, c.conn, attempts, disconnects);
    fflush(stdout);
    /* Required abort must reach the recovered transport before it reports failure. */
    assert(disconnects == 1);
    return 0;
}
