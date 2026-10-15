/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 Bluetooth MIDI (BLE only, no audio): the app side.
 *
 * A separate controller image (ble/ctl: the MPL shim and JieLi's own files, built on the user's
 * computer, never part of this program) sits in the flash block at 0x8B000. This file:
 *   - checks it (header, CRC, entries, RAM window, interrupts) before anything starts it;
 *   - starts it on core 1 at power-on (its init entry, core 0 in RAM meanwhile, fm1_cpu1.h) and
 *     hands core 1's idle loop its poll entry;
 *   - talks to it only through standard HCI packets in the mailbox rings (x0x_ble_abi.h), with
 *     the BLE-MIDI host (ble/ble_midi_host.c, MPL-2.0, from felucca-bsp);
 *   - puts BLE-MIDI in into the USB MIDI in queue and mirrors the MIDI out (clock, notes) to BLE.
 * Without an image, or with BLUETOOTH OFF at power-on, nothing of this runs and core 1 starts as
 * before. Service: the TIMER5 interrupt, next to usb_poll (2 kHz, never inside the render). */
#include "fm1_ble.h"
#include "ble/ble_midi_host.c"
#include "ble/ble_glue.h"

/* All of it in BLERAM (main RAM is full): not cleared at boot, so ble_start sets every field it
 * reads, and nothing reads them before (ble_host_on, in .bss, stays 0 until then). */
#define BLE_OQ 64u                              /* MIDI out to mirror: USB-MIDI packets */
typedef struct {
    xb_mbox_t mb;                               /* first: 64-byte aligned (the rings' cache lines) */
    bleh_t h;
    xb_boot_t boot;
    uint8_t fr[XB_FRAME_MAX];                   /* one frame from the controller */
    uint32_t oq[BLE_OQ];
    volatile uint32_t ow, orr;
    uint32_t sysex_drops;
    char nm[9];                                 /* the name it advertises */
} ble_ram_t;
static ble_ram_t ble_r __attribute__((section(".ble_ram"), aligned(64)));
static uint32_t ble_rx_last[2];             /* the last frame from the controller (DIAG) */
/* core 0's last Bluetooth tick (ms since boot) and core 1's poll count then. In .ble_ram (not
 * cleared): after a watchdog reset, DIAG shows when core 0 stopped and if core 1 still ran. */
static uint32_t ble_alive[2] __attribute__((section(".ble_ram")));

static void ble_record(void);
static uint8_t ble_img = BLE_IMG_NONE, ble_boot_on, ble_up, ble_want, ble_host_on, ble_dead, ble_blocked;
static uint32_t ble_ok_at;                      /* fm1_ms when the controller became READY */
/* the start guard (ble_glue.h): survives a reset, so a start that crashes or freezes the unit is not
 * tried again at the next power-on */
ble_guard_t ble_guard __attribute__((section(".noinit")));
extern uint32_t _ram_sink[], _ram_sink_end[];   /* app.ld: 1 KiB at 0x01C00000 that nothing uses */
#define BLE_SINK_FILL(i) (0x5A17C0DEu ^ ((i) * 0x01010101u))
static void ble_sink_fill(void)
{
    uint32_t i, n = (uint32_t)(_ram_sink_end - _ram_sink);
    for (i = 0; i < n; i++)
        _ram_sink[i] = BLE_SINK_FILL(i);
}
/* how many words of the sink changed since ble_sink_fill: a write by the radio (or its DMA) */
/* core 0's record of its start wait (fm1_cpu1.h), kept in the guard */
static void ble_wait_record(void)
{
    uint32_t i;
    ble_guard.wait_ms = fm1_c1_wait_ms;
    for (i = 0; i < 4u; i++)
        ble_guard.seen[i] = fm1_c1_wait_seen[i];
    ble_guard.code_bad = fm1_c1_sum_bad;
    ble_guard.code_sum = fm1_c1_sum_got;
}
static void ble_sink_check(void)
{
    uint32_t i, n = (uint32_t)(_ram_sink_end - _ram_sink), bad = 0;
    for (i = 0; i < n; i++)
        if (_ram_sink[i] != BLE_SINK_FILL(i)) {
            if (!bad++) {
                ble_guard.sink_at = (uint32_t)(uintptr_t)&_ram_sink[i];
                ble_guard.sink_val = _ram_sink[i];
            }
        }
    ble_guard.sink_n = bad;
}

static int ble_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    return xb_frame_put(&ble_r.mb.h2c, t, p, n);
}
static void ble_sysex_in(const uint8_t *p, uint32_t n, int end, void *ctx)
{
    (void)p; (void)n; (void)ctx;
    if (end)
        ble_r.sysex_drops++;                      /* SysEx from Bluetooth: not taken (counted) */
}

/* power-on, before core 1 starts (plat_cpu2_set): the image check, and with BLUETOOTH ON core 1's
 * stacks and the PC limits for it */
static void ble_prepare(int on)
{
    const xb_header_t *h = FM1_BLE_HDR;
    ble_img = (uint8_t)ble_img_check(FM1_BLE_IMG, XB_FLASH_SIZE, XB_FLASH_XIP);
    ble_want = (uint8_t)(on != 0);
    if (ble_guard.magic == BLE_GUARD_MAGIC && ble_guard.trying) {
        if (ble_r.mb.magic == XB_MBOX_MAGIC)
            ble_record();                       /* the start that reset the unit: its mailbox (BLERAM) */
        ble_sink_check();                       /* (the boot may have used that RAM too) */
        ble_guard.sink_n |= 0x80000000u;        /* bit 31: checked at the next boot, not after the init */
        ble_guard.reset = fm1_boot.p3_rst | (fm1_boot.rst_src & 0xFFFFFFu) << 8;   /* what reset it */
        ble_guard.wait_end = fm1_c1_wait_end;   /* how core 0's wait ended (RAM code) */
        ble_wait_record();
    }
    if (!ble_guard_boot(&ble_guard, ble_img == BLE_IMG_OK, on)) {
        if (ble_img == BLE_IMG_OK && on && ble_guard.blocked) {
            ble_blocked = 1;                    /* the last start failed: not now (STATUS FAILED) */
            ble_dead = 1;
        }
        return;
    }
    ble_boot_on = 1;
    fm1_cpu1_ble_stacks(h->isr_stack_hi, h->ram_lo);
    fm1_guard_xip_hi = XB_FLASH_XIP + XB_FLASH_SIZE - 1u;
    fm1_guard_ram_hi = h->ram_hi;
}

/* power-on, after core 1 started and before the audio and TIMER5: run the image's init on core 1
 * with core 0 in RAM (the controller's RF trim), then hand core 1 its poll entry */
/* The controller's init runs on core 0 at power-on, before core 1 starts (one core runs, as in
 * fm1-ctl). Tests 1-11 ran it on core 1 while core 0 waited in RAM: core 0 stopped about 48 ms
 * into every start (in the TX IQ trim), with its RAM code unchanged; core 1 then went on to READY.
 * ble_attach hands the controller to core 1 after plat_cpu2_set. */
static uint8_t ble_c0_ready;
static void ble_start(void)
{
    const xb_header_t *h = FM1_BLE_HDR;
    xb_mbox_t *mb = &ble_r.mb;
    int rc;
    uint32_t bus, stk, top;
    uint8_t addr[6];
    {
        static const uint8_t none[1] = {0};
        if (flash_ok)                           /* the factory record (USRTRIM): one address per unit */
            ble_addr_of(fm1_xip_ptr(SMK_TRIM_FLASH), 0x22Cu, addr, ble_r.nm);
        else
            ble_addr_of(none, 1u, addr, ble_r.nm);
    }
    ble_r.ow = ble_r.orr = 0;
    ble_r.sysex_drops = 0;
    ble_c0_ready = 0;
    if (!ble_boot_on)
        return;
    {
        uint8_t *z = (uint8_t *)mb;
        uint32_t i;
        for (i = 0; i < sizeof *mb; i++)
            z[i] = 0;
    }
    mb->magic = XB_MBOX_MAGIC;
    mb->abi = XB_ABI;
    xb_ring_init(&mb->h2c);
    xb_ring_init(&mb->c2h);
    fm1_ble_boot_fill(&ble_r.boot, mb);
    bleh_init(&ble_r.h, ble_send, 0, ble_r.nm, addr);
    bleh_set_sysex(&ble_r.h, ble_sysex_in);
    fm1_ble_sync();
    ble_guard.start_ms = fm1_ms;
    ble_guard.sink_n = ble_guard.sink_at = ble_guard.sink_val = ble_guard.wait_end = 0;
    ble_guard.xosc_pre = fm1_ble_xosc();
    ble_guard.xosc_post = 0;
    ble_sink_fill();
    fm1_guard_enable(FM1_GUARD_PC);             /* the PC windows with the image's flash and RAM (ble_prepare) */
    bus = fm1_guard_bus_pause();                /* JieLi's RF code: not for the bus-invalid checks */
    top = (uint32_t)fm1_guard_top_locked();
    if (top)
        fm1_guard_unlock_top();                 /* request_irq writes the vectors */
    fm1_wdt_feed();                             /* init takes about 0.6 s of the 8 s */
    {
        uint32_t f = irq_save(), t0 = fm1_ticks();
        stk = fm1_guard_stack_pause();          /* the image's run stack */
        rc = fm1_ble_init_here(h->init, &ble_r.boot);
        fm1_guard_stack_resume(stk);
        ble_guard.init_ms = (fm1_ticks() - t0) / (FM1_TICKS_PER_US * 1000u);
        irq_restore(f);
    }
    if (top)
        fm1_guard_lock_top();
    fm1_wdt_feed();
    ble_guard.xosc_post = fm1_ble_xosc();
    ble_record();
    ble_sink_check();
    ble_guard_started(&ble_guard, 0, rc == 0 && mb->state == XB_ST_READY);
    if (rc != 0 || mb->state != XB_ST_READY) {
        fm1_guard_bus_resume(bus);
        ble_dead = 1;
        return;
    }
    ble_c0_ready = 1;                           /* the bus-invalid checks stay off while it runs */
}

/* after plat_cpu2_set: core 1 runs the controller from now on (its interrupts, its poll) */
static void ble_attach(void)
{
    const xb_header_t *h = FM1_BLE_HDR;
    if (!ble_c0_ready)
        return;
    if (!fm1_c1_on) {                           /* one core (fm1-x0x#10): no Bluetooth */
        ble_guard.trying = 0;                   /* not the controller's fault: no block */
        fm1_guard_bus_resume(FM1_GUARD_BUS_BITS);
        ble_dead = 1;
        return;
    }
    fm1_guard_watch_c0_stacks();               /* test 20: who changes core 0's interrupt stack */
    fm1_c1_mb.poll = (void (*)(void))(uintptr_t)h->poll;
    fm1_ble_sync();
    ble_ok_at = fm1_ms;
    ble_up = 1;
}

/* the image's start record and the mailbox, kept in the guard (ED_DIAG reads it) */
static void ble_record(void)
{
    uint32_t i;
    fm1_ble_sync();
    ble_guard.mb_state = ble_r.mb.state;
    ble_guard.mb_err = ble_r.mb.err;
    for (i = 0; i < 6u; i++)
        ble_guard.dbg[i] = ble_r.mb.dbg[i];
}

/* the audio ISR (single producer): mirror one MIDI out packet */
static void ble_out(uint32_t pkt)
{
    if (ble_host_on && bleh_notifying(&ble_r.h) && ble_r.ow - ble_r.orr < BLE_OQ) {
        ble_r.oq[ble_r.ow % BLE_OQ] = pkt;
        RING_PUBLISH();
        ble_r.ow++;
    }
}

static void ble_fail(void)
{
    fm1_guard_bus_resume(FM1_GUARD_BUS_BITS);   /* the controller stops: the app's checks again */
    fm1_c1_mb.poll = 0;                         /* core 1 stops calling it */
    fm1_ble_sync();
    fm1_cpu1_irqs_off();
    ble_up = 0;
    ble_host_on = 0;
    ble_dead = 1;
}

/* TIMER5, next to usb_poll (same context: the MIDI in queue has one writer) */
static void ble_service(void)
{
    bleh_t *h = &ble_r.h;
    uint32_t k;
    uint8_t t;
    bleh_msg_t m;
    if (!ble_up)
        return;
    if (ble_r.mb.state != XB_ST_READY || !fm1_c1_on) {
        ble_record();
        ble_fail();
        return;
    }
    ble_alive[0] = fm1_ms;
    ble_alive[1] = ble_r.mb.polls;
    if (ble_guard.trying && fm1_ms - ble_ok_at > BLE_GUARD_OK_MS)
        ble_guard_ok(&ble_guard);               /* it ran: the next power-on starts it again */
    if (!ble_host_on) {
        if (!ble_want)
            return;                             /* OFF since power-on: the host never starts */
        ble_host_on = 1;
        bleh_start(h);
    }
    bleh_radio(h, ble_want);
    for (k = 0; k < 8u; k++) {                  /* events and data from the controller */
        int n = xb_frame_get(&ble_r.mb.c2h, &t, ble_r.fr, sizeof ble_r.fr);
        if (n == 0)
            break;
        if (n > 0) {
            ble_rx_last[0] = t | (uint32_t)n << 8 | (uint32_t)ble_r.fr[0] << 16 | (uint32_t)ble_r.fr[1] << 24;
            ble_rx_last[1] = ble_r.fr[2] | (uint32_t)ble_r.fr[3] << 8 | (uint32_t)ble_r.fr[4] << 16 |
                             (uint32_t)ble_r.fr[5] << 24;
            bleh_rx(h, t, ble_r.fr, (uint16_t)n);
        }
    }
    while (ble_r.orr != ble_r.ow) {             /* MIDI out, mirrored */
        uint8_t b[3];
        uint32_t len = ble_msg_of_usb(ble_r.oq[ble_r.orr % BLE_OQ], b);
        if (len && bleh_notifying(h) && bleh_midi_send(h, b, (uint8_t)len, (uint16_t)(fm1_ms & 0x1FFFu)) == -2)
            break;                              /* the host's queue is full: next tick */
        ble_r.orr++;
    }
    bleh_poll(h);
    while (mi_w - mi_r < MQ && bleh_midi_next(h, &m)) {   /* MIDI in: the USB queue */
        uint32_t pkt = ble_usb_of_msg(&m);
        if (pkt) {
            midi_in_q[mi_w % MQ] = pkt;
            RING_PUBLISH();
            mi_w++;
        }
    }
}

int plat_ble_state(void)
{
    if (ble_img == BLE_IMG_NONE)
        return PLAT_BLE_NOIMG;
    if (ble_img == BLE_IMG_BAD)
        return PLAT_BLE_BADIMG;
    if (ble_dead)
        return PLAT_BLE_FAILED;
    if (!ble_boot_on)
        return ble_want ? PLAT_BLE_RESTART : PLAT_BLE_OFF;
    if (!ble_want)
        return PLAT_BLE_OFF;
    if (!ble_up || ble_r.h.state == BLEH_STARTING || ble_r.h.state == BLEH_IDLE)
        return PLAT_BLE_STARTING;
    if (ble_r.h.state == BLEH_ERROR)
        return PLAT_BLE_FAILED;
    return bleh_connected(&ble_r.h) ? PLAT_BLE_CONNECTED : PLAT_BLE_READY;
}
const char *plat_ble_name(void) { return ble_r.nm; }
void plat_ble_set(int on)
{
    ble_want = (uint8_t)(on != 0);
    if (!on && ble_guard.blocked) {             /* BLUETOOTH OFF: the next power-on tries again */
        ble_guard_user_off(&ble_guard);
        if (ble_blocked) {
            ble_blocked = 0;
            ble_dead = 0;                       /* nothing started this boot: OFF, then RESTART */
        }
    }
}

/* ED_DIAG (editor.c): the start guard and the controller's state, as 32-bit words */
static uint32_t ble_diag(uint32_t *w, uint32_t max)
{
    uint32_t n = 0, i;
    const uint32_t *g = (const uint32_t *)&ble_guard;
    for (i = 0; i < sizeof ble_guard / 4u && n < max; i++)
        w[n++] = g[i];
    if (n + 17u > max)
        return n;
    w[n++] = ble_img | (uint32_t)ble_boot_on << 8 | (uint32_t)ble_up << 16 | (uint32_t)ble_dead << 24;
    w[n++] = ble_want | (uint32_t)ble_host_on << 8 | (uint32_t)ble_blocked << 16 | (uint32_t)fm1_c1_on << 24;
    w[n++] = fm1_c1_gave_up;
    w[n++] = (uint32_t)plat_ble_state();
    w[n++] = ble_r.mb.state;                    /* raw: after a reset, the last start's */
    w[n++] = ble_r.mb.err;
    w[n++] = ble_r.mb.polls;
    w[n++] = fm1_c1_mb.poll_max;
    w[n++] = ble_r.h.state;
    w[n++] = fm1_c1_trace;
    w[n++] = fm1_boot.p3_rst | (fm1_boot.rst_src & 0xFFFFFFu) << 8;   /* this boot's reset */
    if (n + 6u > max)
        return n;
    w[n++] = ble_r.mb.h2c.head | ble_r.mb.h2c.tail << 16;   /* the HCI rings: written, read */
    w[n++] = ble_r.mb.c2h.head | ble_r.mb.c2h.tail << 16;
    w[n++] = ble_r.mb.c2h_drops;
    w[n++] = ble_r.h.step | (uint32_t)ble_r.h.cmd_wait << 8 | (uint32_t)ble_r.h.credits << 16;
    w[n++] = ble_r.mb.dbg[XB_DBG_INIT_MS];     /* frames the controller took */
    w[n++] = ble_r.mb.dbg[XB_DBG_MS];          /* frames it gave to the app */
    if (n + 2u > max)
        return n;
    w[n++] = ble_rx_last[0];                                /* the last frame from it: type, length, bytes */
    w[n++] = ble_rx_last[1];
    if (n + 2u > max)
        return n;
    w[n++] = ble_alive[0];
    w[n++] = ble_alive[1];
    return n;
}
