/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
/* The interface between the X0X app (GPL-3.0-only) and a separate Bluetooth controller image
 * (ble/ctl, MPL-2.0 shim plus JieLi's own files, built by the user). This header is the only code
 * that both sides compile. It is MPL-2.0 so that the controller image can use it.
 *
 * The two sides are like two chips on one board:
 *   - the image sits in its own flash block (64 KiB, XIP 0x02087000) and owns its own RAM window;
 *   - the app starts it once on core 1 (init), then calls its idle entry (poll) from core 1's idle
 *     loop, and talks to it only through frames in two byte rings.
 * ABI 3: the image holds the controller AND JieLi's BLE host (btstack.a). The frames are BLE-MIDI
 * packets and the link state (XB_HX_*), no HCI. The image has two flash pieces: piece 1 in the
 * Bluetooth block (0x8B000, in the app area, written by every install) and piece 2 at 0xAB000..0xB6FFF
 * without 0xAF000..0xAFFFF (plain flash, written by the app on the installer's request: editor command
 * CTL2). Its CRC (p2_crc) covers it without the hole. It has a
 * second RAM window at the top of the app's BLERAM (XB_RAM2_LO..HI) and a small store for its bonds
 * (XB_NV_SIZE bytes): the app keeps it in its own flash and hands it over at each start.
 * Nothing else crosses: no app function is called by the image, no image function is called by
 * the app except init and poll.
 *
 * Ring frame: [u16 len, little endian][u8 H4 type][len - 1 bytes]. One writer and one reader per
 * ring. The writer fills the bytes, then XB_SYNC(), then moves head. The reader reads, then
 * XB_SYNC(), then moves tail. Define XB_SYNC before this header (the default is a compiler
 * barrier plus the host's full fence). */
#pragma once
#include <stdint.h>

#ifndef XB_SYNC
#define XB_SYNC() __sync_synchronize()
#endif

#define XB_HDR_MAGIC   0x43423058u        /* "X0BC": the image header */
#define XB_BOOT_MAGIC  0x50424C43u        /* "CLBP": the boot parameters */
#define XB_MBOX_MAGIC  0x584D4258u        /* "XBMX": the mailbox */
#define XB_ABI         3u                 /* this layout (header, boot parameters, mailbox) */
#define XB_HDR_VERSION 3u

/* where the image lives (SMK-37 flash layout 2) and the RAM it may own */
#define XB_FLASH_OFF   0x0008B000u        /* flash offset of the Bluetooth block */
#define XB_FLASH_XIP   0x02087000u        /* its XIP address (the image is linked here) */
#define XB_FLASH_SIZE  0x00010000u        /* 64 KiB */
#define XB_RAM_LO      0x01C01480u        /* the image's RAM window (after the app's RAM code; ABI 3: from 0x01C01480) */
#define XB_RAM_HI      0x01C07F00u        /* (0x01C07F00.. is crt0's status words) */
#define XB_FLASH2_OFF  0x000AB000u        /* piece 2: flash offset (plain, outside the app area) */
#define XB_FLASH2_XIP  0x020A7000u        /* its XIP address (plain window from 0x9D000: XIP = off - 0x4000) */
#define XB_FLASH2_SIZE 0x0000C000u        /* 0xAB000..0xB6FFF, with a hole: */
#define XB_FLASH2_HOLE 0x00004000u        /* 0xAF000..0xAFFFF (the store owner tag) is not the image's */
#define XB_FLASH2_HOLE_SIZE 0x1000u
#define XB_RAM2_LO     0x01C7CDC0u        /* the image's second RAM window: the top of the app's BLERAM */
#define XB_RAM2_HI     0x01C7D400u
#define XB_NV_SIZE     256u               /* the image's bond store (the app keeps it in flash) */
#define XB_NV_ADDR     (XB_RAM2_HI - XB_NV_SIZE)   /* in RAM 2, not cleared by the image: the app fills it */

/* the image header: the first bytes of the image. mkimage.py fills image_size and crc32. */
typedef struct {
    uint32_t magic;                       /* XB_HDR_MAGIC */
    uint16_t hdr_version, hdr_size;       /* XB_HDR_VERSION, sizeof(xb_header_t) */
    uint32_t fw_version;                  /* the image's own version (major << 16 | minor) */
    uint32_t image_size;                  /* bytes from the header to the end of the image */
    uint32_t crc32;                       /* CRC-32 (IEEE) of the image with this field 0 */
    uint32_t abi;                         /* XB_ABI */
    uint32_t init;                        /* int init(const xb_boot_t *): core 1, once; 0 = ready */
    uint32_t poll;                        /* void poll(void): core 1 idle loop, interrupts on */
    uint32_t isr_stack_lo, isr_stack_hi;  /* core 1's interrupt stack (its ssp) */
    uint32_t ram_lo, ram_hi;              /* all the RAM the image uses: inside XB_RAM_LO..HI */
    uint32_t irq_lo, irq_hi;              /* the interrupt numbers it may route to core 1 (bits) */
    uint32_t p2_size, p2_crc;             /* piece 2 at XB_FLASH2_XIP: bytes from there, the hole included (0 =
                                           * none); CRC-32 of those bytes without the hole */
    uint32_t ram2_lo, ram2_hi;            /* the RAM it uses in XB_RAM2_LO..HI (lo == hi: none) */
} xb_header_t;

/* the boot parameters: the app fills them, the image reads them in init */
typedef struct {
    uint32_t magic;                       /* XB_BOOT_MAGIC */
    uint32_t abi;                         /* XB_ABI */
    uint32_t sys_clk_hz, lsb_clk_hz, hsb_clk_hz;
    uint32_t tick_addr;                   /* a free-running up-counter the image may read */
    uint32_t ticks_per_ms;
    uint32_t mbox;                        /* address of the xb_mbox_t */
    uint32_t settle_ms;                   /* how long init may run the controller before it returns */
    uint32_t vec_base;                    /* the interrupt vector table (128 words) */
    uint32_t icfg_base;                   /* core 1's interrupt configuration words (4 bits each) */
    uint8_t xosc[2];                      /* RF calibration: crystal trim left, right */
    uint8_t pa_trim[7];
    uint8_t osc_type;
    uint8_t mcs_dgain[20];
    uint8_t pwr_max, pwr_ble;             /* bt_max_pwr_set: the overall and the BLE level */
    uint8_t addr[6];                      /* its random static address, little endian */
    char name[10];                        /* the name it advertises (0-terminated) */
    uint32_t nv;                          /* address of the bond store (XB_NV_ADDR; the app filled it) */
    uint32_t heap2, heap2_size;           /* more heap for the image in the app's RAM (0 = none); 8-aligned */
} xb_boot_t;

/* controller states (mbox.state, written by the image) */
enum { XB_ST_OFF = 0, XB_ST_INIT = 1, XB_ST_READY = 2, XB_ST_FAILED = 3 };
/* mbox.dbg: [XB_DBG_STAGE] the last init step it reached (XB_STG_*), [XB_DBG_MS] its own ms
 * then, [XB_DBG_DELAYS] delay_us calls (the RF trim makes most of them), [XB_DBG_RA] the caller of
 * the last delay_us (bit 0 set while that delay runs), [XB_DBG_INIT_MS] how long init ran,
 * [XB_DBG_STALL] bits 0-15: delays that saw the tick counter stop, bits 16-31: the caller's caller
 * of the last delay (low 16 bits of the address) */
enum { XB_DBG_STAGE = 0, XB_DBG_MS = 1, XB_DBG_DELAYS = 2, XB_DBG_RA = 3, XB_DBG_INIT_MS = 4, XB_DBG_STALL = 5 };
enum { XB_STG_ENTRY = 1, XB_STG_OS = 2, XB_STG_H4 = 3, XB_STG_TASK = 4, XB_STG_SETTLE = 5, XB_STG_SETTLED = 6,
       XB_STG_READY = 7 };
/* image errors (mbox.err) */
enum { XB_E_NONE = 0, XB_E_BOOT = 1, XB_E_HEAP = 2, XB_E_TASK = 3, XB_E_ASSERT = 4, XB_E_RESET = 5 };

/* ABI 3 frames (the H4 type byte is the frame type) */
enum { XB_HX_MIDI = 0x20,                 /* both ways: one BLE-MIDI packet (header byte first) */
       XB_HX_STATE = 0x21,                /* image -> app: a link change (payload: xb_hx_t.link, 8 bytes) */
       XB_HX_CMD = 0x22 };                /* app -> image: [0] 1 radio on, 0 off; [0] 2: forget the bonds */
enum { XB_LK_IDLE = 0, XB_LK_ADV = 1, XB_LK_CONN = 2 };
/* the link and the image's health, written by the image (DIAG reads it) */
typedef struct {
    volatile uint8_t state, ccc, bonded, reason;   /* XB_LK_*, notify on, encrypted with a bond, last end reason */
    volatile uint16_t interval, mtu;      /* 1.25 ms units; the ATT MTU */
    volatile uint32_t links, pairs;       /* connections; pairings (bits 0-15 done, 16-31 failed) */
    volatile uint32_t heap;               /* heap peak bytes (0-15), failed allocations (16-23), size of the last
                                           * failed one in 16-byte units (24-31) */
    volatile uint32_t stacks;             /* the least free bytes ever / 16: tasks (0-7), run (8-15), isr (16-23) */
    volatile uint32_t nv_writes;          /* bond store changes (the app saves when it changes) */
    volatile uint32_t rx, tx, tx_drops;   /* BLE-MIDI packets in, notified, not sent (no room) */
    volatile uint16_t latency, timeout;   /* a61: conn latency (events), supervision timeout (10 ms units) */
    volatile uint32_t disc[4];            /* a61: recent disconnects: reason(0-7)|evsize(8-15)|status(16-23)|ccc(24) */
    volatile uint32_t evlog, smlog;       /* a61: rolling last-4 LE subevent / SM event codes (newest in low byte) */
    volatile uint32_t updates;            /* a61: conn-update-complete events seen on the current link */
    volatile uint16_t poll_gap_max, pad_a62;  /* a62: max ms between core-1 idle passes while connected */
} xb_hx_t;

#define XB_RING_SIZE 512u                /* a power of two */
typedef struct {
    volatile uint32_t head;               /* the writer's (own cache line) */
    uint32_t pad0[7];
    volatile uint32_t tail;               /* the reader's (own cache line) */
    uint32_t pad1[7];
    uint8_t buf[XB_RING_SIZE];
} xb_ring_t;

typedef struct {
    uint32_t magic, abi;                  /* the app: XB_MBOX_MAGIC, XB_ABI */
    volatile uint32_t dbg[6];             /* the image: its start record (XB_DBG_*), for diagnosis only.
                                           * The app sets them to 0 before init; the layout is ABI 2. */
    volatile uint32_t state, err;         /* the image: XB_ST_*, XB_E_* */
    volatile uint32_t c2h_drops;          /* the image: frames it could not put (ring full) */
    volatile uint32_t irqs_routed[4];     /* the image: the interrupt numbers it took (bits 0-127) */
    volatile uint32_t polls;              /* the image: poll calls (a heartbeat) */
    xb_hx_t hx;                           /* the image: link and health */
    xb_ring_t h2c;                        /* app -> image: BLE-MIDI packets to notify, commands */
    xb_ring_t c2h;                        /* image -> app: BLE-MIDI packets written, link changes */
} xb_mbox_t;

#define XB_FRAME_MAX 160u                 /* the largest frame either side writes (type + payload) */

static inline uint32_t xb_ring_used(const xb_ring_t *r)
{
    return (r->head - r->tail) & (XB_RING_SIZE - 1u);
}
static inline uint32_t xb_ring_free(const xb_ring_t *r)
{
    return XB_RING_SIZE - 1u - xb_ring_used(r);
}

/* one frame in: 0 = written, -1 = no room (nothing written), -2 = bad length */
static inline int xb_frame_put(xb_ring_t *r, uint8_t type, const uint8_t *p, uint32_t n)
{
    uint32_t len = n + 1u, h, i;
    if (len > XB_FRAME_MAX)
        return -2;
    if (xb_ring_free(r) < len + 2u)
        return -1;
    h = r->head;
    r->buf[h] = (uint8_t)len;
    h = (h + 1u) & (XB_RING_SIZE - 1u);
    r->buf[h] = (uint8_t)(len >> 8);
    h = (h + 1u) & (XB_RING_SIZE - 1u);
    r->buf[h] = type;
    h = (h + 1u) & (XB_RING_SIZE - 1u);
    for (i = 0; i < n; i++) {
        r->buf[h] = p[i];
        h = (h + 1u) & (XB_RING_SIZE - 1u);
    }
    XB_SYNC();
    r->head = h;
    XB_SYNC();
    return 0;
}

/* the type of the next frame and its payload length: 1 = a whole frame waits, 0 = none,
 * -1 = the ring holds a bad length (the reader drops everything: xb_ring_flush) */
static inline int xb_frame_peek(const xb_ring_t *r, uint8_t *type, uint32_t *n)
{
    uint32_t used, t, len;
    XB_SYNC();
    used = xb_ring_used(r);
    if (used < 3u)
        return 0;
    t = r->tail;
    len = (uint32_t)r->buf[t] | (uint32_t)r->buf[(t + 1u) & (XB_RING_SIZE - 1u)] << 8;
    if (len == 0 || len > XB_FRAME_MAX)
        return -1;
    if (used < len + 2u)
        return 0;
    *type = r->buf[(t + 2u) & (XB_RING_SIZE - 1u)];
    *n = len - 1u;
    return 1;
}

/* one frame out into p (max bytes): the payload length, 0 = none, -1 = bad length or too long for
 * p (the frame is dropped, the ring stays in step) */
static inline int xb_frame_get(xb_ring_t *r, uint8_t *type, uint8_t *p, uint32_t max)
{
    uint32_t n = 0, t, i;
    int rc = xb_frame_peek(r, type, &n);
    if (rc <= 0) {
        if (rc < 0) {
            r->tail = r->head;            /* a bad length: nothing after it can be trusted */
            XB_SYNC();
        }
        return rc;
    }
    t = (r->tail + 3u) & (XB_RING_SIZE - 1u);
    for (i = 0; i < n; i++) {
        if (i < max)
            p[i] = r->buf[t];
        t = (t + 1u) & (XB_RING_SIZE - 1u);
    }
    XB_SYNC();
    r->tail = t;
    XB_SYNC();
    return n <= max ? (int)n : -1;
}

static inline void xb_ring_init(xb_ring_t *r)
{
    r->head = 0;
    r->tail = 0;
}

/* CRC-32 (IEEE, reflected 0xEDB88320), as zlib.crc32: the image check */
static inline uint32_t xb_crc32(uint32_t crc, const uint8_t *p, uint32_t n)
{
    uint32_t i, b;
    crc = ~crc;
    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (b = 0; b < 8u; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
