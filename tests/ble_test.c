/* SPDX-License-Identifier: GPL-3.0-only */
/* Host tests for Bluetooth MIDI (SMK-37): the HCI mailbox rings (x0x_ble_abi.h), the BLE-MIDI host
 * (src/ble/ble_midi_host.c: start sequence, GATT, MIDI in and out with timestamps, running status,
 * SysEx, pairing refused, radio off), the glue (src/ble/ble_glue.h: image check, USB-MIDI packets,
 * the address) and the park handshake (the protocol of fm1_cpu1.h, with two threads).
 *   cc -O2 -Wall -Ifirmware/hal -Ifirmware/src/ble tests/ble_test.c -lpthread
 *   a.out [ctl.img]   (with a file: only the app's image check on it) */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "ble_midi_host.c"
#include "ble_glue.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---------------------------------------------------------------- mailbox */
static void t_ring(void)
{
    static xb_ring_t r;
    uint8_t p[400], q[400], t = 0;
    uint32_t i, n = 0;
    int k;
    xb_ring_init(&r);
    CHECK(xb_ring_used(&r) == 0 && xb_ring_free(&r) == XB_RING_SIZE - 1u);
    CHECK(xb_frame_get(&r, &t, q, sizeof q) == 0);
    for (i = 0; i < sizeof p; i++) p[i] = (uint8_t)(i * 7u + 1u);
    CHECK(xb_frame_put(&r, 4, p, XB_FRAME_MAX) == -2);          /* type + payload > XB_FRAME_MAX */
    CHECK(xb_frame_put(&r, 4, p, XB_FRAME_MAX - 1u) == 0);
    CHECK(xb_frame_get(&r, &t, q, sizeof q) == (int)(XB_FRAME_MAX - 1u) && t == 4 && !memcmp(p, q, XB_FRAME_MAX - 1u));
    /* wrap many times, sizes 0..250, the ring always in step */
    for (k = 0; k < 5000; k++) {
        uint32_t len = (uint32_t)(k * 37) % 251u;
        int rc = xb_frame_put(&r, (uint8_t)(k & 7), p + (k % 50), len);
        CHECK(rc == 0);
        rc = xb_frame_get(&r, &t, q, sizeof q);
        if (rc != (int)len || t != (k & 7) || memcmp(q, p + (k % 50), len)) { n++; }
    }
    CHECK(n == 0);
    /* full: a put that does not fit writes nothing */
    xb_ring_init(&r);
    for (k = 0; xb_frame_put(&r, 2, p, 100) == 0; k++) ;
    CHECK(k == (int)((XB_RING_SIZE - 1u) / 103u));
    CHECK(xb_frame_put(&r, 2, p, 100) == -1);
    for (i = 0; i < (uint32_t)k; i++) CHECK(xb_frame_get(&r, &t, q, sizeof q) == 100);
    CHECK(xb_ring_used(&r) == 0);
    /* too long for the reader: dropped, the ring stays in step */
    CHECK(xb_frame_put(&r, 2, p, 50) == 0 && xb_frame_put(&r, 4, p, 3) == 0);
    CHECK(xb_frame_get(&r, &t, q, 10) == -1);
    CHECK(xb_frame_get(&r, &t, q, 10) == 3 && t == 4);
    /* a bad length: everything flushed */
    xb_ring_init(&r);
    r.buf[0] = 0xFF; r.buf[1] = 0xFF; r.buf[2] = 4; r.head = 10;
    CHECK(xb_frame_get(&r, &t, q, sizeof q) == -1 && r.tail == r.head);
    /* half a frame: nothing yet */
    xb_ring_init(&r);
    r.buf[0] = 5; r.buf[1] = 0; r.buf[2] = 4; r.head = 4;
    CHECK(xb_frame_get(&r, &t, q, sizeof q) == 0 && r.tail == 0);
    /* CRC-32 as zlib */
    CHECK(xb_crc32(0, (const uint8_t *)"123456789", 9) == 0xCBF43926u);
}

/* two threads, one writer and one reader: every frame arrives once, in order, intact */
static xb_ring_t sp_r;
#define SP_N 200000
static void *sp_writer(void *a)
{
    uint8_t p[64];
    int k = 0;
    (void)a;
    while (k < SP_N) {
        uint32_t len = 1u + (uint32_t)k % 60u, i;
        for (i = 0; i < len; i++) p[i] = (uint8_t)(k + (int)i);
        p[0] = (uint8_t)k;
        if (xb_frame_put(&sp_r, (uint8_t)(k & 3), p, len) == 0) k++;
    }
    return 0;
}
static void t_ring_threads(void)
{
    pthread_t th;
    uint8_t q[64], t;
    int k = 0, bad = 0;
    xb_ring_init(&sp_r);
    pthread_create(&th, 0, sp_writer, 0);
    while (k < SP_N) {
        int n = xb_frame_get(&sp_r, &t, q, sizeof q), i;
        if (n == 0) continue;
        if (n != 1 + k % 60 || t != (k & 3) || q[0] != (uint8_t)k) bad++;
        for (i = 1; i < n; i++) if (q[i] != (uint8_t)(k + i)) bad++;
        k++;
    }
    pthread_join(th, 0);
    CHECK(bad == 0);
}

/* ---------------------------------------------------------------- the host against a fake controller */
typedef struct { uint8_t t; uint8_t b[64]; uint16_t n; } pkt_t;
static pkt_t sent[64];
static int nsent;
static int fake_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (nsent < 64 && n <= 64) { sent[nsent].t = t; memcpy(sent[nsent].b, p, n); sent[nsent].n = n; nsent++; }
    return 0;
}
static unsigned op_of(int i) { return sent[i].b[0] | sent[i].b[1] << 8; }
static void cc(bleh_t *h, unsigned op, uint8_t status, uint8_t extra)   /* command complete */
{
    uint8_t e[10] = {0x0E, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), status, 0, 0, 0, 0};
    if (op == 0x2002) { e[1] = 7; e[6] = 27; e[7] = 0; e[8] = extra; }
    bleh_rx(h, BLEH_H4_EVT, e, (uint16_t)(2 + e[1]));
}
static void acl(bleh_t *h, unsigned cid, const uint8_t *d, unsigned n)
{
    uint8_t b[64];
    b[0] = 0x40; b[1] = 0x20;                  /* handle 0x040, PB 2 (start) */
    b[2] = (uint8_t)(4 + n); b[3] = 0; b[4] = (uint8_t)n; b[5] = 0; b[6] = (uint8_t)cid; b[7] = 0;
    memcpy(b + 8, d, n);
    bleh_rx(h, BLEH_H4_ACL, b, (uint16_t)(8 + n));
}
static void midi_w(bleh_t *h, const uint8_t *d, unsigned n)   /* the central writes the MIDI characteristic */
{
    uint8_t b[40] = {0x52, 6, 0};
    memcpy(b + 3, d, n);
    acl(h, 4, b, 3 + n);
}
static int sx_got[3], sx_len;
static uint8_t sx_buf[128];
static void sx_cb(const uint8_t *p, uint32_t n, int end, void *ctx)
{
    (void)ctx;
    if (end > 0) sx_got[0]++;
    else if (end < 0) sx_got[1]++;
    else { memcpy(sx_buf + sx_len, p, n); sx_len += (int)n; sx_got[2]++; }
}
static void connect(bleh_t *h)
{
    static const uint8_t uaddr[6] = {1, 2, 3, 4, 5, 6};
    static const uint8_t cmplt[21] = {0x3E, 19, 0x01, 0x00, 0x40, 0x00, 0x01};
    unsigned ops[] = {0x0C03, 0x0C01, 0x2001, 0x2002, 0x2005, 0x2006, 0x2008, 0x2009};
    int i;
    nsent = 0;
    bleh_init(h, fake_send, 0, "X0X-ABCD", uaddr);
    bleh_set_sysex(h, sx_cb);
    bleh_start(h);
    for (i = 0; i < 8; i++) {
        CHECK(nsent == i + 1 && sent[i].t == BLEH_H4_CMD && op_of(i) == ops[i]);
        if (i == 1) CHECK(sent[i].b[10] & 0x20);                             /* LE meta (bit 61) */
        if (i == 2) CHECK(sent[i].b[3] & 1);                                 /* LE connection complete */
        if (i == 4) CHECK(sent[i].b[3] == 1 && sent[i].b[8] == (6 | 0xC0));   /* the random static address */
        if (i == 5) CHECK(sent[i].b[3 + 5] == 1);                            /* own address type: random */
        cc(h, ops[i], 0, 4);
    }
    CHECK(h->state == BLEH_ADVERTISING);
    bleh_poll(h);                                  /* advertising on */
    CHECK(nsent == 9 && op_of(8) == 0x200A && sent[8].b[3] == 1);
    cc(h, 0x200A, 0, 0);
    bleh_poll(h);
    CHECK(nsent == 9);
    bleh_rx(h, BLEH_H4_EVT, cmplt, 21);
    CHECK(bleh_connected(h) && h->conn == 0x40);
}
static void t_host(void)
{
    static bleh_t h;
    bleh_msg_t m;
    uint8_t w[2] = {1, 0};
    connect(&h);
    /* ATT: MTU, the name, the CCCD */
    nsent = 0;
    { uint8_t r[3] = {2, 0x17, 0}; acl(&h, 4, r, 3); }
    CHECK(nsent == 1 && sent[0].t == BLEH_H4_ACL && sent[0].b[8] == 3 && sent[0].b[9] == 23);
    { uint8_t r[3] = {10, 3, 0}; acl(&h, 4, r, 3); }
    CHECK(nsent == 2 && sent[1].b[8] == 11 && !memcmp(sent[1].b + 9, "X0X-ABCD", 8));
    CHECK(!bleh_notifying(&h));
    { uint8_t r[5] = {18, 7, 0, w[0], w[1]}; acl(&h, 4, r, 5); }
    CHECK(bleh_notifying(&h) && nsent == 3 && sent[2].b[8] == 19);
    { uint8_t e[] = {0x13, 5, 1, 0x40, 0, 3, 0}; bleh_rx(&h, BLEH_H4_EVT, e, sizeof e); }   /* the 3 buffers back */
    CHECK(h.credits == 4);
    /* MIDI in: note on with a timestamp; running status with and without timestamp; real time */
    { const uint8_t d[] = {0x81, 0x82, 0x90, 60, 100, 0x83, 62, 101, 64, 102, 0x84, 0xF8};
      midi_w(&h, d, sizeof d); }
    CHECK(bleh_midi_next(&h, &m) && m.len == 3 && m.b[0] == 0x90 && m.b[1] == 60 && m.b[2] == 100 && m.ts == (1 << 7 | 2));
    CHECK(bleh_midi_next(&h, &m) && m.b[0] == 0x90 && m.b[1] == 62 && m.ts == (1 << 7 | 3));
    CHECK(bleh_midi_next(&h, &m) && m.b[0] == 0x90 && m.b[1] == 64 && m.b[2] == 102 && m.ts == (1 << 7 | 3));
    CHECK(bleh_midi_next(&h, &m) && m.len == 1 && m.b[0] == 0xF8 && m.ts == (1 << 7 | 4));
    CHECK(!bleh_midi_next(&h, &m));
    /* a72: real time as Loopy Pro sends it: Stop, Start, clocks and Continue in one event, a clock between
     * running status notes; each one is a USB-MIDI packet CIN 0xF for the sequencer (engine.c midi_in) */
    { const uint8_t d[] = {0x80, 0x81, 0xFC, 0x81, 0xFA, 0x81, 0xF8, 0x82, 0x90, 60, 100, 0x83, 0xF8, 61, 100,
                           0x84, 0xF8, 0x85, 0xFB};
      static const uint32_t want[] = {0xFu | 0xFCu << 8, 0xFu | 0xFAu << 8, 0xFu | 0xF8u << 8,
                                      0x9u | 0x90u << 8 | 60u << 16 | 100u << 24, 0xFu | 0xF8u << 8,
                                      0x9u | 0x90u << 8 | 61u << 16 | 100u << 24, 0xFu | 0xF8u << 8, 0xFu | 0xFBu << 8};
      unsigned i;
      midi_w(&h, d, sizeof d);
      for (i = 0; i < sizeof want / sizeof want[0]; i++)
          CHECK(bleh_midi_next(&h, &m) && ble_usb_of_msg(&m) == want[i]);
      CHECK(!bleh_midi_next(&h, &m)); }
    /* program change (2 bytes), song position (system common, ends running status) */
    { const uint8_t d[] = {0x80, 0x80, 0xC3, 5, 0x81, 0xF2, 0x10, 0x20, 0x82, 7};
      midi_w(&h, d, sizeof d); }
    CHECK(bleh_midi_next(&h, &m) && m.len == 2 && m.b[0] == 0xC3 && m.b[1] == 5);
    CHECK(bleh_midi_next(&h, &m) && m.len == 3 && m.b[0] == 0xF2 && m.b[1] == 0x10 && m.b[2] == 0x20);
    CHECK(!bleh_midi_next(&h, &m));               /* the 7 after F2: no running status */
    /* SysEx over two packets, a clock inside it */
    sx_len = 0; memset(sx_got, 0, sizeof sx_got);
    { const uint8_t d[] = {0x80, 0x81, 0xF0, 0x7E, 0x7F, 0x82, 0xF8, 0x06}; midi_w(&h, d, sizeof d); }
    { const uint8_t d[] = {0x80, 0x01, 0x02, 0x83, 0xF7}; midi_w(&h, d, sizeof d); }
    CHECK(sx_got[0] == 1 && sx_got[1] == 0 && sx_len == 5 && !memcmp(sx_buf, "\x7E\x7F\x06\x01\x02", 5));
    CHECK(bleh_midi_next(&h, &m) && m.b[0] == 0xF8 && !bleh_midi_next(&h, &m));
    CHECK(h.sysex_in == 1);
    /* a SysEx broken off by a note */
    { const uint8_t d[] = {0x80, 0x81, 0xF0, 0x01, 0x82, 0x90, 1, 2}; midi_w(&h, d, sizeof d); }
    CHECK(sx_got[1] == 1 && bleh_midi_next(&h, &m) && m.b[0] == 0x90);
    /* a cut message: the rest of the packet is dropped */
    { const uint8_t d[] = {0x80, 0x81, 0x90, 1}; midi_w(&h, d, sizeof d); }
    CHECK(!bleh_midi_next(&h, &m));
    /* attempt 32: a burst of 60 notes (12 packets of 5) all arrive */
    { int k, j, got = 0;
      for (k = 0; k < 12; k++) {
          uint8_t d[17] = {0x80, 0x81, 0x90, 0, 1};
          for (j = 0; j < 4; j++) { d[5 + 3 * j] = 0x81; d[6 + 3 * j] = (uint8_t)(k * 5 + j + 1); d[7 + 3 * j] = 1; }
          d[3] = (uint8_t)(k * 5); midi_w(&h, d, sizeof d);
      }
      while (bleh_midi_next(&h, &m)) { if (m.b[0] == 0x90 && m.b[1] == got) got++; }
      CHECK(got == 60); }
    /* a bad header: ignored */
    { const uint8_t d[] = {0x00, 0x81, 0x90, 1, 2}; midi_w(&h, d, sizeof d); }
    CHECK(!bleh_midi_next(&h, &m));
    /* attempt 34: the parameter request on demand, once; its answer */
    nsent = 0;
    CHECK(bleh_conn_update(&h, 12, 24, 0, 600) == 1 && nsent == 1 && sent[0].b[8] == 0x12 &&
          sent[0].b[12] == 12 && sent[0].b[14] == 24 && sent[0].b[18] == 0x58 && sent[0].b[19] == 2);
    CHECK(bleh_conn_update(&h, 12, 24, 0, 600) == 0);
    { uint8_t e[] = {0x13, 5, 1, 0x40, 0, 1, 0}; bleh_rx(&h, BLEH_H4_EVT, e, sizeof e); }
    { uint8_t r[] = {0x40, 0x20, 10, 0, 6, 0, 5, 0, 0x13, 0x31, 2, 0, 1, 0};
      bleh_rx(&h, BLEH_H4_ACL, r, sizeof r); CHECK(h.cpu_result == 2); }
    { uint8_t u[] = {0x3E, 10, 0x03, 0, 0x40, 0, 24, 0, 0, 0, 0x58, 2}; bleh_rx(&h, BLEH_H4_EVT, u, sizeof u);
      CHECK(h.conn_int == 24 && h.conn_lat == 0); }
    /* MIDI out: one notification, one header, running status off by default */
    nsent = 0;
    { const uint8_t a[3] = {0x90, 60, 100}, b[3] = {0x90, 61, 100}, c[1] = {0xF8};
      CHECK(bleh_midi_send(&h, a, 3, 0x0105) == 0);
      CHECK(bleh_midi_send(&h, b, 3, 0x0106) == 0);
      CHECK(bleh_midi_send(&h, c, 1, 0x0106) == 0); }
    { const uint8_t bad[3] = {0x90, 0x80, 1}; CHECK(bleh_midi_send(&h, bad, 3, 0) == -1); }
    { const uint8_t sx[3] = {0xF0, 1, 0xF7}; CHECK(bleh_midi_send(&h, sx, 3, 0) == -1); }
    bleh_poll(&h);
    CHECK(nsent == 1);
    { const uint8_t want[] = {0x1B, 6, 0, 0x82, 0x85, 0x90, 60, 100, 0x86, 0x90, 61, 100, 0x86, 0xF8};
      CHECK(sent[0].b[4] == sizeof want && !memcmp(sent[0].b + 8, want, sizeof want)); }
    /* running status on */
    h.tx_running = 1; nsent = 0;
    { const uint8_t a[3] = {0x80, 60, 0}, b[3] = {0x80, 61, 0};
      bleh_midi_send(&h, a, 3, 5); bleh_midi_send(&h, b, 3, 6); }
    bleh_poll(&h);
    { const uint8_t want[] = {0x1B, 6, 0, 0x80, 0x85, 0x80, 60, 0, 0x86, 61, 0};
      CHECK(nsent == 1 && !memcmp(sent[0].b + 8, want, sizeof want)); }
    h.tx_running = 0;
    /* timestamps in two headers: two notifications */
    nsent = 0;
    { const uint8_t a[3] = {0x90, 1, 1}; bleh_midi_send(&h, a, 3, 0x7F); bleh_midi_send(&h, a, 3, 0x80); }
    bleh_poll(&h);
    CHECK(nsent == 1 && sent[0].b[11] == 0x80 && sent[0].b[12] == 0xFF);
    { uint8_t e[] = {0x13, 5, 1, 0x40, 0, 2, 0}; bleh_rx(&h, BLEH_H4_EVT, e, sizeof e); }   /* credits back */
    bleh_poll(&h);
    CHECK(nsent == 2 && sent[1].b[11] == 0x81 && sent[1].b[12] == 0x80);
    /* SysEx out: 40 data bytes over notifications, F0 first, F7 last */
    {
        uint8_t sx[42], got[64];
        int i, gl = 0, j;
        sx[0] = 0xF0; for (i = 1; i <= 40; i++) sx[i] = (uint8_t)i; sx[41] = 0xF7;
        CHECK(bleh_sysex_send(&h, sx, 42, 3) == 0);
        CHECK(bleh_sysex_send(&h, sx, 42, 3) == -2);
        nsent = 0;
        for (i = 0; i < 6 && h.sx_on; i++) {
            uint8_t e[] = {0x13, 5, 1, 0x40, 0, 4, 0};
            bleh_rx(&h, BLEH_H4_EVT, e, sizeof e);
            bleh_poll(&h);
        }
        CHECK(!h.sx_on && nsent == 3);
        CHECK(sent[0].b[12] == 0x83 && sent[0].b[13] == 0xF0);
        for (i = 0; i < nsent; i++)
            for (j = 12; j < 8 + sent[i].b[4]; j++) got[gl++] = sent[i].b[j];
        CHECK(gl == 2 + 40 + 2 && got[gl - 2] == 0x83 && got[gl - 1] == 0xF7 && !memcmp(got + 2, sx + 1, 40));
        for (i = 0; i < nsent; i++) CHECK(sent[i].b[4] <= 23);
    }
    /* pairing refused, L2CAP signalling rejected */
    nsent = 0;
    { uint8_t r[7] = {0x01, 3, 0, 1, 16, 7, 7}; acl(&h, 6, r, 7); }
    CHECK(nsent == 1 && sent[0].b[6] == 6 && sent[0].b[8] == 5 && sent[0].b[9] == 5 && h.smp_rejects == 1);
    { uint8_t r[8] = {0x12, 9, 4, 0, 1, 2, 3, 4}; acl(&h, 5, r, 8); }
    CHECK(nsent == 2 && sent[1].b[6] == 5 && sent[1].b[8] == 1 && sent[1].b[9] == 9);
    /* radio off: disconnect, then advertising off; on again: advertising on */
    nsent = 0;
    bleh_radio(&h, 0);
    bleh_poll(&h);
    CHECK(nsent == 1 && op_of(0) == 0x0406 && sent[0].b[5] == 0x13);
    bleh_poll(&h);
    CHECK(nsent == 1);
    { uint8_t e[] = {0x0F, 4, 0, 1, 0x06, 0x04}; bleh_rx(&h, BLEH_H4_EVT, e, sizeof e); }
    { uint8_t e[] = {0x05, 4, 0, 0x40, 0, 0x16}; bleh_rx(&h, BLEH_H4_EVT, e, sizeof e); }
    CHECK(h.state == BLEH_ADVERTISING && !bleh_notifying(&h));
    bleh_poll(&h);                                 /* the link ended advertising already: nothing to send */
    bleh_poll(&h);
    CHECK(nsent == 1);
    bleh_radio(&h, 1);
    bleh_poll(&h);
    CHECK(nsent == 2 && op_of(1) == 0x200A && sent[1].b[3] == 1);
    cc(&h, 0x200A, 0, 0);
    bleh_radio(&h, 0);                             /* off while advertising: advertising off */
    bleh_poll(&h);
    CHECK(nsent == 3 && op_of(2) == 0x200A && sent[2].b[3] == 0);
    cc(&h, 0x200A, 0, 0);
    bleh_poll(&h);
    CHECK(nsent == 3);
    /* a failed command during start: error */
    {
        static bleh_t g;
        nsent = 0;
        bleh_init(&g, fake_send, 0, "X", 0);
        bleh_start(&g);
        cc(&g, 0x0C03, 0x0C, 0);
        CHECK(g.state == BLEH_ERROR);
    }
}

/* ---------------------------------------------------------------- the glue */
static uint8_t img[XB_FLASH_SIZE];
static uint32_t make_img(uint32_t size)
{
    xb_header_t *h = (xb_header_t *)(void *)img;
    uint32_t i;
    memset(img, 0xFF, sizeof img);
    for (i = sizeof *h; i < size; i++) img[i] = (uint8_t)(i * 13u);
    memset(h, 0, sizeof *h);
    h->magic = XB_HDR_MAGIC; h->hdr_version = XB_HDR_VERSION; h->hdr_size = sizeof *h;
    h->fw_version = 0x20000; h->image_size = size; h->abi = XB_ABI;
    h->init = XB_FLASH_XIP + 0xD8; h->poll = XB_FLASH_XIP + 0xF0;
    h->isr_stack_lo = 0x01C06E80; h->isr_stack_hi = 0x01C07680;
    h->ram_lo = XB_RAM_LO; h->ram_hi = 0x01C07680;
    h->irq_lo = (1u << 29) | (1u << 31); h->irq_hi = (1u << 8) | (1u << 9) | (1u << 10) | (1u << 13);
    h->crc32 = 0;
    h->crc32 = xb_crc32(0, img, size);
    return h->crc32;
}
static void t_glue(void)
{
    xb_header_t *h = (xb_header_t *)(void *)img;
    bleh_msg_t m;
    uint8_t b[3], a1[6], a2[6];
    char n1[9], n2[9];
    uint8_t rec[0x22C];
    memset(img, 0xFF, sizeof img);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_NONE);   /* an empty block: as before */
    memset(img, 0, sizeof img);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_NONE);
    make_img(64504);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_OK);
    img[1000] ^= 1;
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);   /* one bit: the CRC */
    make_img(64504); h->image_size = 0x10001;
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    make_img(64504); h->abi = 1; h->crc32 = 0; h->crc32 = xb_crc32(0, img, 64504);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
#define REMAKE(stmt) do { make_img(64504); stmt; h->crc32 = 0; h->crc32 = xb_crc32(0, img, 64504); } while (0)
    REMAKE(h->init = XB_FLASH_XIP + 64504);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->poll = 0x01C01800);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->ram_hi = XB_RAM_HI + 4);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->ram_lo = XB_RAM_LO - 4);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->isr_stack_hi = 0x01C08000);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->irq_hi |= 1u << 31);                   /* TIMER5 */
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->irq_lo |= 1u << 11);                   /* ALNK0 */
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    REMAKE(h->hdr_size = 64);
    CHECK(ble_img_check(img, sizeof img, XB_FLASH_XIP) == BLE_IMG_BAD);
    /* USB-MIDI packets */
    m.len = 3; m.b[0] = 0x92; m.b[1] = 60; m.b[2] = 7;
    CHECK(ble_usb_of_msg(&m) == (0x9u | 0x92u << 8 | 60u << 16 | 7u << 24));
    m.b[0] = 0xF8; m.b[1] = m.b[2] = 0;
    CHECK(ble_usb_of_msg(&m) == (0xFu | 0xF8u << 8));
    m.b[0] = 0xF2; m.b[1] = 1; m.b[2] = 2;
    CHECK((ble_usb_of_msg(&m) & 0xF) == 3);
    m.b[0] = 0xF1; CHECK((ble_usb_of_msg(&m) & 0xF) == 2);
    m.b[0] = 0xF6; CHECK((ble_usb_of_msg(&m) & 0xF) == 5);
    m.b[0] = 0xF0; CHECK(ble_usb_of_msg(&m) == 0);
    m.b[0] = 0x40; CHECK(ble_usb_of_msg(&m) == 0);
    CHECK(ble_msg_of_usb(0x9u | 0x90u << 8 | 60u << 16 | 100u << 24, b) == 3 && b[0] == 0x90 && b[1] == 60 && b[2] == 100);
    CHECK(ble_msg_of_usb(0xCu | 0xC1u << 8 | 5u << 16, b) == 2);
    CHECK(ble_msg_of_usb(0xFu | 0xF8u << 8, b) == 1 && b[0] == 0xF8);
    CHECK(ble_msg_of_usb(0xFu | 0xFAu << 8, b) == 1);
    CHECK(ble_msg_of_usb(0x4u | 0xF0u << 8, b) == 0);              /* SysEx: not mirrored */
    CHECK(ble_msg_of_usb(0x9u | 0x80u << 8, b) == 0);              /* CIN and status disagree */
    CHECK(ble_msg_of_usb(0x3u | 0xF2u << 8 | 1u << 16 | 2u << 24, b) == 3);
    /* every message both ways: the same bytes */
    {
        int st, bad = 0;
        for (st = 0x80; st < 0x100; st++) {
            bleh_msg_t mm = {(uint8_t)msg_need((uint8_t)st), {(uint8_t)st, 0x12, 0x34}, 0};
            uint32_t p = ble_usb_of_msg(&mm), n;
            if (!p) continue;
            if (mm.len < 3) mm.b[2] = 0;
            if (mm.len < 2) mm.b[1] = 0;
            p = ble_usb_of_msg(&mm);
            n = ble_msg_of_usb(p, b);
            if (n != mm.len || memcmp(b, mm.b, n)) bad++;
        }
        CHECK(bad == 0);
    }
    /* the address: random static, the same for the same record, the name from it */
    memset(rec, 0x33, sizeof rec);
    ble_addr_of(rec, sizeof rec, a1, n1);
    ble_addr_of(rec, sizeof rec, a2, n2);
    CHECK(!memcmp(a1, a2, 6) && !strcmp(n1, n2) && (a1[5] & 0xC0) == 0xC0 && !strncmp(n1, "X0X-", 4) && strlen(n1) == 8);
    rec[100] ^= 1;
    ble_addr_of(rec, sizeof rec, a2, n2);
    CHECK(memcmp(a1, a2, 6) != 0);
}

/* ---------------------------------------------------------------- park handshake (fm1_cpu1.h's protocol) */
/* Core 1 here is a thread running the idle loop's park branch as fm1_c1_main does: between jobs it
 * calls poll (the "flash" must be on), parks when asked (no poll, no interrupts), and still runs
 * jobs. Core 0 asks to park, waits for parked, turns the "flash" off, back on, unparks. Any poll
 * while the flash is off is a failure. */
static volatile int pk_flash_on = 1, pk_park, pk_parked, pk_stop, pk_bad_poll, pk_job, pk_done, pk_seq;
static volatile long pk_polls;
static void *pk_core1(void *a)
{
    int parked = 0, done = 0;
    (void)a;
    while (!pk_stop) {
        int j = __atomic_load_n(&pk_job, __ATOMIC_SEQ_CST);
        if (j == done) {
            int pk = __atomic_load_n(&pk_park, __ATOMIC_SEQ_CST);
            if (pk) {
                parked = 1;
                if (pk_parked != pk) __atomic_store_n(&pk_parked, pk, __ATOMIC_SEQ_CST);
                continue;
            }
            if (parked) { parked = 0; __atomic_store_n(&pk_parked, 0, __ATOMIC_SEQ_CST); }
            if (!__atomic_load_n(&pk_flash_on, __ATOMIC_SEQ_CST)) pk_bad_poll++;
            pk_polls++;
            continue;
        }
        done = j;                                  /* a job (runs whether parked or not) */
        __atomic_store_n(&pk_done, j, __ATOMIC_SEQ_CST);
    }
    return 0;
}
static void t_park(void)
{
    pthread_t th;
    int k, timeouts = 0;
    pthread_create(&th, 0, pk_core1, 0);
    for (k = 0; k < 20000; k++) {
        long spin = 0;
        int seq = ++pk_seq;
        __atomic_store_n(&pk_park, seq, __ATOMIC_SEQ_CST);
        while (__atomic_load_n(&pk_parked, __ATOMIC_SEQ_CST) != seq)
            if (++spin > 100000000L) { timeouts++; break; }
        __atomic_store_n(&pk_flash_on, 0, __ATOMIC_SEQ_CST);
        if (k % 3 == 0) {                          /* an audio job while parked (between read windows) */
            int j = pk_job + 1;
            __atomic_store_n(&pk_job, j, __ATOMIC_SEQ_CST);
            while (__atomic_load_n(&pk_done, __ATOMIC_SEQ_CST) != j) ;
        }
        __atomic_store_n(&pk_flash_on, 1, __ATOMIC_SEQ_CST);
        __atomic_store_n(&pk_park, 0, __ATOMIC_SEQ_CST);
    }
    {                                              /* unparked: it polls again */
        long p0 = pk_polls, spin = 0;
        while (pk_polls == p0 && ++spin < 2000000000L) ;
        CHECK(pk_polls != p0);
    }
    pk_stop = 1;
    pthread_join(th, 0);
    CHECK(timeouts == 0);
    CHECK(pk_bad_poll == 0);
}

/* the start guard (BEHAVIOR.md BLE-12) */
static void t_guard(void)
{
    ble_guard_t g;
    memset(&g, 0xA5, sizeof g);                      /* power-on RAM: no magic */
    CHECK(ble_guard_boot(&g, 1, 1) == 1 && g.magic == BLE_GUARD_MAGIC && g.trying && !g.blocked && !g.fails);
    ble_guard_started(&g, 0, 1);                     /* READY: still trying until ok */
    CHECK(g.trying && !g.blocked);
    ble_guard_ok(&g);
    CHECK(!g.trying && !g.fails);
    CHECK(ble_guard_boot(&g, 1, 1) == 1);            /* a good start does not block the next one */
    /* a crash or a hang in the start: the unit resets with trying set */
    CHECK(ble_guard_boot(&g, 1, 1) == 0 && g.blocked && g.why == BLE_WHY_RESET && g.fails == 1 && !g.trying);
    CHECK(ble_guard_boot(&g, 1, 1) == 0 && g.fails == 1);  /* stays blocked, no new count */
    ble_guard_user_off(&g);                          /* BLUETOOTH OFF */
    CHECK(!g.blocked && ble_guard_boot(&g, 1, 0) == 0 && !g.trying);
    CHECK(ble_guard_boot(&g, 1, 1) == 1 && g.trying);  /* ON again: one new try */
    ble_guard_started(&g, -1, 0);
    CHECK(g.blocked && g.why == BLE_WHY_TIMEOUT && !g.trying && g.fails == 2);
    g.blocked = 0;
    CHECK(ble_guard_boot(&g, 1, 1) == 1);
    ble_guard_started(&g, -2, 0);
    CHECK(g.why == BLE_WHY_WRONG_JOB && g.blocked);
    g.blocked = 0;
    CHECK(ble_guard_boot(&g, 1, 1) == 1);
    ble_guard_started(&g, 0, 0);
    CHECK(g.why == BLE_WHY_NOT_READY && g.blocked);
    g.blocked = 0;
    CHECK(ble_guard_boot(&g, 1, 1) == 1);
    ble_guard_started(&g, -3, 0);
    CHECK(g.why == BLE_WHY_NO_TICK && g.blocked && !g.trying);
    g.blocked = 0;
    CHECK(ble_guard_boot(&g, 0, 1) == 0 && !g.trying);  /* no image: no try */
    g.magic = 0;                                     /* bad magic: a clean record */
    g.trying = 1;
    CHECK(ble_guard_boot(&g, 1, 1) == 1 && !g.blocked && !g.fails);
}

int main(int argc, char **argv)
{
    if (argc > 1) {                                /* ble_test ctl.img: the app's check on a real image */
        FILE *f = fopen(argv[1], "rb");
        size_t n;
        memset(img, 0xFF, sizeof img);
        if (!f) { printf("ble_test: cannot open %s\n", argv[1]); return 1; }
        n = fread(img, 1, sizeof img, f);
        fclose(f);
        printf("%s: %zu B, ble_img_check %d (0 = ok)\n", argv[1], n, ble_img_check(img, sizeof img, XB_FLASH_XIP));
        return ble_img_check(img, sizeof img, XB_FLASH_XIP) != BLE_IMG_OK;
    }
    t_ring();
    t_ring_threads();
    t_host();
    t_glue();
    t_park();
    t_guard();
    printf("ble_test: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
