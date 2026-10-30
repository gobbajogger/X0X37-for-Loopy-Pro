/* Public HCI and extracted production service probe. No device access occurs. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.h"
static struct {
    struct { blec_t c; } host;
    struct { unsigned state, polls, c2h; } mb;
    uint8_t fr[128];
} ble_r;
enum { XB_ST_READY = 1, BLE_GUARD_OK_MS = 1000 };
static unsigned ble_up, fm1_c1_on, ble_host_on, ble_want, busy;
static uint32_t fm1_ms, ble_alive[2], ble_ok_at;
static struct { unsigned trying; } ble_guard;
static unsigned attempts, accepted, disconnects, pending_n, linked;
static uint8_t pending[32];
static void ble_record(void) { assert(0); }
static void ble_fail(void) { assert(0); }
static void ble_guard_ok(void *p) { (void)p; }
static void ble_central_link_notify(int up) { linked = (unsigned)up; }
static int xb_frame_get(void *q, uint8_t *t, uint8_t *p, unsigned n)
{
    (void)q;
    if (!pending_n) return 0;
    assert(pending_n <= n); unsigned have = pending_n;
    memcpy(p, pending, have); pending_n = 0; *t = 4;
    return (int)have;
}
static unsigned xb_ring_used(void *q) { (void)q; return pending_n; }
static int transport(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx; assert(h4 == 1 && n >= 3);
    attempts++;
    if (busy) return -1;
    accepted++;
    if (p[0] == 6 && p[1] == 4) disconnects++;
    return 0;
}
#include "ble_central_service.inc"
int main(int argc, char **argv)
{
    blec_t *c = &ble_r.host.c;
    const uint8_t addr[6] = {0};
    unsigned scanning = argc > 1 && !strcmp(argv[1], "scan");
    unsigned refused = argc < 3 || !strcmp(argv[2], "refused");
    unsigned failed_status = argc > 2 && !strcmp(argv[2], "failed-status");
    blec_init(c, transport, 0, 0, addr);
    c->state = scanning ? BLEC_SCANNING : BLEC_CONNECTING;
    c->step = scanning ? 6 : 8;
    c->cmd_op = scanning ? 0x200c : 0x200d; c->waiting = 1;
    c->acl_total = c->credits = 5;
    c->deadline = 7000;
    /* Successful status for the active Create or Command Complete for Scan Enable. */
    const uint8_t status[] = {0x0f, 4, 0, 1, 0x0d, 0x20};
    const uint8_t scan_on[] = {0x0e, 4, 1, 0x0c, 0x20, 0};
    blec_rx(c, 4, scanning ? scan_on : status, 6, 100);
    assert(!c->waiting && c->state == (scanning ? BLEC_SCANNING : BLEC_CONNECTING));
    ble_up = fm1_c1_on = ble_host_on = 1; ble_want = 0;
    ble_r.mb.state = XB_ST_READY; fm1_ms = 100; busy = refused;
    ble_central_service();
    printf("After production stop: path=%s refused=%u state=%u waiting=%u handle=%u attempts=%u accepted=%u deadline=%u\n",
        scanning ? "scan-disable" : "cancel", refused, c->state, c->waiting,
        c->conn, attempts, accepted, c->deadline);
    fflush(stdout);
    if (scanning) {
        assert(c->state == BLEC_STOPPING); /* A refused disable must remain pending. */
        if (!refused) {
            uint8_t disabled[] = {0x0e, 4, 1, 0x0c, 0x20, failed_status ? 0x0c : 0};
            memcpy(pending, disabled, sizeof disabled); pending_n = sizeof disabled;
            fm1_ms = 101; ble_central_service();
            printf("After Scan Disable Command Complete: status=%u state=%u waiting=%u\n",
                failed_status ? 0x0c : 0, c->state, c->waiting);
            fflush(stdout);
            assert(c->state == (failed_status ? BLEC_FAILED : BLEC_OFF) && !linked);
        }
    } else {
        const uint8_t connected[] = {0x3e, 0x13, 1, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        memcpy(pending, connected, sizeof connected); pending_n = sizeof connected;
        busy = 0; fm1_ms = 101; ble_central_service();
        printf("After successful connection: state=%u handle=%u attempts=%u accepted=%u Disconnects=%u\n",
            c->state, c->conn, attempts, accepted, disconnects);
        fflush(stdout);
        assert(c->conn == 0x50 && disconnects == 1 && c->state == BLEC_STOPPING && !linked);
    }
    return 0;
}
