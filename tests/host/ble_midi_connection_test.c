/* SPDX-License-Identifier: GPL-3.0-only */
/* Public-API HCI regression. All controller events and time are simulated. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ble_midi_central.h"

typedef struct { unsigned commands, scans, creates, cancels, acl, blocked; } wire_t;
static wire_t wire;
static uint32_t now;
static const char *name;
static unsigned failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL %s line=%d: %s state=%u step=%u waiting=%u handle=%04x scans=%u creates=%u cancels=%u\n", name, __LINE__, #x, c.state, c.step, c.waiting, c.conn, wire.scans, wire.creates, wire.cancels); failures++; return; } } while (0)
static int send_h4(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (wire.blocked) return -1;
    if (h4 == 1 && n >= 3) {
        unsigned op = p[0] | (unsigned)p[1] << 8;
        wire.commands++;
        if (op == 0x200c && n >= 5 && p[3]) wire.scans++;
        if (op == 0x200d) wire.creates++;
        if (op == 0x200e) wire.cancels++;
    } else if (h4 == 2) wire.acl++;
    return 0;
}
static void event(blec_t *c, const uint8_t *p, unsigned n)
{ blec_rx(c, 4, p, (uint16_t)n, ++now); }
static void cc(blec_t *c, unsigned op, unsigned status)
{
    uint8_t p[] = {0x0e, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), (uint8_t)status, 27, 0, 5};
    if (op == 0x2002) p[1] = 7;
    event(c, p, p[1] + 2u);
}
static void cs(blec_t *c, unsigned op, unsigned status)
{
    const uint8_t p[] = {0x0f, 4, (uint8_t)status, 1, (uint8_t)op, (uint8_t)(op >> 8)};
    event(c, p, sizeof p);
}
static void connection(blec_t *c, unsigned enhanced, unsigned status)
{
    uint8_t p[33] = {0x3e, 19, 1, (uint8_t)status, 0x50, 0, 0, 0, 1, 2, 3, 4, 5, 6};
    if (enhanced) { p[1] = 31; p[2] = 0x0a; }
    unsigned off = enhanced ? 26 : 14;
    p[off] = 24; p[off + 4] = 0x90; p[off + 5] = 1;
    event(c, p, p[1] + 2u);
}
static void advertisement(blec_t *c)
{
    const uint8_t p[] = {0x3e, 17, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 9, 'S', 'M', 'C', 0xd8};
    event(c, p, sizeof p);
}
static void begin(blec_t *c, uint32_t start)
{
    static const uint8_t addr[6] = {1, 2, 3, 4, 5, 0xc0};
    static const unsigned setup[] = {0x0c03, 0x0c01, 0x2001, 0x2002, 0x2005, 0x200b, 0x200c};
    memset(&wire, 0, sizeof wire); now = start;
    blec_init(c, send_h4, 0, 0, addr); blec_start(c, now);
    for (unsigned i = 0; i < sizeof setup / sizeof *setup; i++) cc(c, setup[i], 0);
    advertisement(c); cc(c, 0x200c, 0); cs(c, 0x200d, 0);
}
static void timeout(blec_t *c)
{ now = c->deadline; blec_poll(c, now); }
static void acl(blec_t *c, const uint8_t *p, unsigned n)
{
    uint8_t b[40] = {0x50, 0x20, (uint8_t)(n + 4), 0, (uint8_t)n, 0, 4, 0};
    memcpy(b + 8, p, n); blec_rx(c, 2, b, (uint16_t)(n + 8), ++now);
}
static void discover(blec_t *c)
{
    const uint8_t svc[] = {0x11, 20, 0x70, 0, 0x7f, 0, 0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
    const uint8_t chr[] = {0x09, 21, 0x71, 0, 0x16, 0x72, 0, 0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77};
    const uint8_t desc[] = {0x05, 1, 0x73, 0, 0x02, 0x29};
    const uint8_t write[] = {0x13};
    acl(c, svc, sizeof svc); acl(c, chr, sizeof chr); acl(c, desc, sizeof desc); acl(c, write, sizeof write);
}
static void success_cancel(unsigned enhanced, uint32_t start)
{
    blec_t c; name = "successful Cancel waits for required terminal event"; begin(&c, start); timeout(&c);
    CHECK(wire.cancels == 1 && c.state == BLEC_CONNECTING);
    cc(&c, 0x200e, 0);
    CHECK(c.state == BLEC_CONNECTING && wire.scans == 1);
    connection(&c, enhanced, 2);
    CHECK(c.state == BLEC_SCANNING && wire.scans == 2 && wire.cancels == 1);
    cc(&c, 0x200c, 0); advertisement(&c); cc(&c, 0x200c, 0); cs(&c, 0x200d, 0);
    connection(&c, enhanced, 0); discover(&c);
    CHECK(blec_connected(&c) && wire.creates == 2);
    printf("PASS %s enhanced=%u wrap=%u\n", name, enhanced, start > 0xffff0000u);
}
static void refused_cancel(unsigned enhanced, unsigned connection_first, unsigned failure)
{
    blec_t c; name = failure ? "terminal failure before Cancel refusal" : "connection wins automatic Cancel";
    begin(&c, 100); timeout(&c);
    if (connection_first) connection(&c, enhanced, failure ? 0x3e : 0);
    cc(&c, 0x200e, 0x0c);
    if (!connection_first) {
        connection(&c, enhanced, failure ? 0x3e : 0);
    }
    if (failure) CHECK(c.state == BLEC_SCANNING && wire.scans == 2);
    else {
        CHECK(c.state == BLEC_DISCOVERING && c.conn == 0x50 && wire.scans == 1 && c.att_waiting);
        discover(&c); CHECK(blec_connected(&c));
    }
    printf("PASS %s enhanced=%u connection_first=%u\n", name, enhanced, connection_first);
}
static void late_terminal(unsigned enhanced, unsigned success, unsigned complete_first)
{
    blec_t c; name = "late original Create result remains owned"; begin(&c, 0xfffff000u); timeout(&c);
    uint32_t cutoff = c.deadline;
    if (complete_first) cc(&c, 0x200e, success ? 0x0c : 0);
    now = cutoff; blec_poll(&c, now);
    connection(&c, enhanced, success ? 0 : complete_first ? 2 : 0x3e);
    if (!complete_first) cc(&c, 0x200e, 0x0c);
    if (success) { CHECK(c.state == BLEC_DISCOVERING && c.conn == 0x50); discover(&c); CHECK(blec_connected(&c)); }
    else CHECK(c.state == BLEC_SCANNING && wire.scans == 2);
    CHECK(wire.cancels == 1);
    printf("PASS %s enhanced=%u success=%u complete_first=%u\n", name, enhanced, success, complete_first);
}
static void stale_cancel_discovery(void)
{
    blec_t c; name = "late Cancel refusal does not disturb discovery"; begin(&c, 100); timeout(&c);
    connection(&c, 0, 0);
    CHECK(c.state == BLEC_DISCOVERING && c.att_waiting);
    unsigned before = wire.commands;
    cc(&c, 0x200e, 0x0c);
    CHECK(c.state == BLEC_DISCOVERING && c.att_waiting && wire.commands == before);
    discover(&c); CHECK(blec_connected(&c));
    printf("PASS %s\n", name);
}
static void stopped_cancel(unsigned terminal_first)
{
    blec_t c; name = terminal_first ? "stop after original Create failed" : "stop after automatic Cancel transport refusal";
    begin(&c, 100);
    if (!terminal_first) wire.blocked = 1;
    timeout(&c);
    if (terminal_first) connection(&c, 0, 0x3e);
    else { CHECK(wire.cancels == 0); wire.blocked = 0; }
    blec_stop(&c);
    CHECK(c.state == BLEC_STOPPING && wire.cancels == 1);
    cc(&c, 0x200e, terminal_first ? 0x0c : 0);
    if (!terminal_first) connection(&c, 0, 2);
    CHECK(c.state == BLEC_OFF && wire.cancels == 1);
    printf("PASS %s\n", name);
}
int main(void)
{
    for (unsigned e = 0; e < 2; e++) {
        success_cancel(e, 100); success_cancel(e, 0xfffff000u);
        refused_cancel(e, 0, 0); refused_cancel(e, 1, 0); refused_cancel(e, 1, 1);
        for (unsigned s = 0; s < 2; s++) for (unsigned f = 0; f < 2; f++) late_terminal(e, s, f);
    }
    stale_cancel_discovery();
    stopped_cancel(0); stopped_cancel(1);
    printf("%u failing scenarios\n", failures);
    return failures ? 1 : 0;
}
