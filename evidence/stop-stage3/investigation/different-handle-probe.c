/* Simulated public HCI and extracted production service. No device access. */
#define main retained_public_suite_main
#include "../../../tests/host/ble_midi_stop_test.c"
#undef main

static void different_connection(unsigned enhanced)
{
    uint8_t p[33] = {0x3e, enhanced ? 31 : 19, enhanced ? 0x0a : 1,
        0, 0x51, 0, 0};
    unsigned n = enhanced ? 33 : 21;
    memcpy(pending, p, n); pending_n = n;
    ble_central_service();
}
int main(void)
{
    unsigned cases = 0;
    for (unsigned wrap = 0; wrap < 2; wrap++)
        for (unsigned enhanced = 0; enhanced < 2; enhanced++)
            for (unsigned expired = 0; expired < 2; expired++)
                for (unsigned refused = 0; refused < 2; refused++) {
                    blec_t *c = &ble_r.host.c;
                    setup(1, wrap ? 0xfffffff0u : 100u);
                    uint32_t cutoff = stop(0);
                    response(0x200e, 0, 1);
                    busy = refused;
                    connection(enhanced, 0, 1);
                    assert(c->conn == 0x50 && c->state == BLEC_STOPPING);
                    assert(accepted[2] == !refused);
                    if (expired) {
                        fm1_ms = cutoff;
                        ble_central_service();
                        assert(c->state == BLEC_FAILED && !c->waiting);
                    }
                    unsigned previous = attempts[2];
                    unsigned state = c->state;
                    /* The other successful event must not replace the retained handle. */
                    different_connection(!enhanced);
                    assert(c->conn == 0x50 && c->state == state);
                    assert(c->deadline == cutoff && accepted[2] == !refused);
                    assert(attempts[2] == previous + (!expired && refused));
                    down(0, 0x51);
                    assert(c->conn == 0x50 && c->state == state);
                    /* Turning Bluetooth on cannot erase the unresolved stop. */
                    ble_want = 1;
                    ble_central_service();
                    blec_start(c, fm1_ms);
                    assert(c->conn == 0x50 && c->state == state);
                    assert(c->deadline == cutoff && !linked);
                    down(0, 0x50);
                    assert(c->state == BLEC_OFF && !c->conn && c->credits == 5);
                    busy = 0;
                    ble_central_service();
                    assert(c->state == BLEC_STARTING && c->cmd_op == 0x0c03);
                    cases++;
                }
    printf("%u different-handle, stop-intent and restart cases: PASS\n", cases);
    return 0;
}
