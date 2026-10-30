/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"

static uint8_t tx_type, tx[40], tx_n, midi[8];
static unsigned midi_n, midi_count;
static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    assert(n <= sizeof tx);
    tx_type = type; tx_n = (uint8_t)n; memcpy(tx, p, n); return 0;
}
static void got_midi(const uint8_t *p, uint8_t n, void *ctx)
{
    (void)ctx;
    assert(n <= sizeof midi); memcpy(midi, p, n); midi_n = n; midi_count++;
}
static void cc(blec_t *c, unsigned op, const uint8_t *extra, unsigned n)
{
    uint8_t b[16] = {0x0e, 0, 1, (uint8_t)op, (uint8_t)(op >> 8), 0};
    b[1] = (uint8_t)(4u + n);
    if (n) memcpy(b + 6, extra, n);
    blec_rx(c, 4, b, (uint16_t)(6u + n), 10);
}
static void att(blec_t *c, const uint8_t *p, unsigned n)
{
    uint8_t b[64] = {0};
    b[0] = 1; b[2] = (uint8_t)(n + 4u); b[4] = (uint8_t)n; b[6] = 4;
    memcpy(b + 8, p, n); blec_rx(c, 2, b, (uint16_t)(n + 8u), 100);
}
int main(void)
{
    blec_t c;
    const uint8_t addr[6] = {1,2,3,4,5,0xc0};
    const uint8_t buf[3] = {27,0,5};
    const uint8_t adv[] = {0x3e,0x17,0x02,1,0,1,1,2,3,4,5,6,11,10,9,'S','M','C','-','M','i','x','e','r',0xc0};
    const uint8_t conn[] = {0x3e,0x13,1,0,1,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};
    const uint8_t gap[] = {0x11,6,1,0,3,0,0,0x18};
    const uint8_t svc[] = {0x11,20,4,0,8,0,0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,
                           0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
    const uint8_t chr[] = {0x09,21,5,0,0x16,6,0,0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,
                           0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77};
    const uint8_t info[] = {0x05,1,7,0,2,0x29};
    const uint8_t wr[] = {0x13};
    const uint8_t note[] = {0x1b,6,0,0x80,0x81,0x90,0x18,0x7f};
    const uint8_t sx1[] = {0x1b,6,0,0x80,0x81,0xf0,0x7f,0x7f};
    const uint8_t sx2[] = {0x1b,6,0,0x80,0x06,0x21,0x81,0xf7};
    blec_init(&c, send_h4, got_midi, 0, addr);
    blec_start(&c, 0);
    cc(&c, 0x0c03, 0, 0); cc(&c, 0x0c01, 0, 0);
    cc(&c, 0x2001, 0, 0); cc(&c, 0x2002, buf, 3);
    cc(&c, 0x2005, 0, 0); cc(&c, 0x200b, 0, 0);
    cc(&c, 0x200c, 0, 0);
    assert(c.state == BLEC_SCANNING);
    blec_rx(&c, 4, adv, sizeof adv, 20);
    assert(c.step == 7 && tx_type == 1 && tx[0] == 0x0c && tx[1] == 0x20);
    cc(&c, 0x200c, 0, 0);
    assert(c.state == BLEC_CONNECTING && tx[0] == 0x0d);
    { const uint8_t status[] = {0x0f,4,0,1,0x0d,0x20}; blec_rx(&c,4,status,sizeof status,30); }
    blec_rx(&c, 4, conn, sizeof conn, 40);
    assert(c.state == BLEC_DISCOVERING && tx_type == 2 && tx[8] == 0x10);
    att(&c, gap, sizeof gap); assert(tx[8] == 0x10 && tx[9] == 4);
    att(&c, svc, sizeof svc); assert(tx[8] == 0x08);
    att(&c, chr, sizeof chr); assert(tx[8] == 0x04);
    att(&c, info, sizeof info);
    /* Synthetic discovery assigns CCCD 7; capture metadata does not supply it. */
    assert(tx_type == 2 && tx_n == 13 && tx[8] == 0x12 &&
           tx[9] == 7 && tx[10] == 0 && tx[11] == 1 && tx[12] == 0);
    att(&c, wr, sizeof wr); assert(blec_connected(&c));
    att(&c, note, sizeof note); assert(midi_count == 1 && midi_n == 3 && midi[1] == 0x18);
    att(&c, sx1, sizeof sx1); att(&c, sx2, sizeof sx2);
    assert(midi_count == 2 && midi_n == 6 && midi[0] == 0xf0 && midi[4] == 0x21 && midi[5] == 0xf7);
    /* Timestamp-low 0xF7 in an open SysEx precedes ordinary 7-bit data. */
    { const uint8_t sx3[] = {0x1b,6,0,0x80,0x81,0xf0,0x7f,0x7f};
      const uint8_t sx4[] = {0x1b,6,0,0x80,0xf7,0x06,0x21,0x81,0xf7};
      att(&c, sx3, sizeof sx3); att(&c, sx4, sizeof sx4); }
    assert(midi_count == 3 && midi_n == 6 && midi[4] == 0x21 && midi[5] == 0xf7);
    { const uint8_t out[] = {0x90,0x5e,0x7f}; assert(blec_midi_send(&c,out,3)); }
    { const uint8_t done[] = {0x13,5,1,1,0,5,0}; blec_rx(&c,4,done,sizeof done,110); }
    blec_poll(&c, 111); assert(tx_type == 2 && tx[8] == 0x52);
    /* A timed-out create waits for Cancel Complete before scanning again. */
    blec_init(&c, send_h4, got_midi, 0, addr);
    c.state = BLEC_CONNECTING; c.step = 8; c.deadline = 1;
    blec_poll(&c, 10); assert(c.state == BLEC_CONNECTING && tx_type == 1 && tx[0] == 0x0e);
    cc(&c, 0x200e, 0, 0); assert(c.state == BLEC_SCANNING && tx[0] == 0x0c);
    /* Discovery failure disconnects before restarting the scan. */
    blec_init(&c, send_h4, got_midi, 0, addr);
    c.state = BLEC_DISCOVERING; c.step = 10; c.conn = 1;c.linked=1;
    { const uint8_t error[] = {1,0x10,1,0,0x0a}; att(&c, error, sizeof error); }
    assert(c.state == BLEC_DISCONNECTING && tx_type == 1 && tx[0] == 0x06);
    /* The controller answers Disconnect with Command Status; it returns the command credit. */
    { const uint8_t st[] = {0x0f,4,0,1,0x06,0x04}; blec_rx(&c,4,st,sizeof st,115); }
    { const uint8_t down[] = {5,4,0,1,0,0x13}; blec_rx(&c,4,down,sizeof down,120); }
    assert(c.state == BLEC_SCANNING && tx[0] == 0x0c);
    /* OFF→ON waits for scan disable completion, then starts with a fresh Reset. */
    blec_init(&c, send_h4, got_midi, 0, addr);
    c.state = BLEC_SCANNING; c.step = 6; c.now_ms = 200;
    blec_stop(&c); assert(c.state == BLEC_STOPPING && tx[0] == 0x0c);
    cc(&c, 0x200c, 0, 0); assert(c.state == BLEC_OFF);
    blec_start(&c, 220); assert(c.state == BLEC_STARTING && tx[0] == 0x03);
    cc(&c, 0x200c, 0, 0); assert(c.state == BLEC_STARTING && tx[0] == 0x03);
    /* A cancelled create waits for LE Connection Complete as well as Cancel Complete. */
    blec_init(&c, send_h4, got_midi, 0, addr);
    c.state = BLEC_CONNECTING; c.step = 8; c.now_ms = 240;
    blec_stop(&c); assert(c.state == BLEC_STOPPING && tx[0] == 0x0e);
    cc(&c, 0x200e, 0, 0); assert(c.state == BLEC_STOPPING);
    { uint8_t failed_conn[sizeof conn]; memcpy(failed_conn, conn, sizeof conn);
      failed_conn[3] = 2; blec_rx(&c,4,failed_conn,sizeof failed_conn,250); }
    assert(c.state == BLEC_OFF);
    return 0;
}
