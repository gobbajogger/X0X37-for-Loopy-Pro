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

static void ble_record(void);
static uint8_t ble_img = BLE_IMG_NONE, ble_boot_on, ble_up, ble_want, ble_host_on, ble_dead, ble_blocked;
static uint32_t ble_ok_at;                      /* fm1_ms when the controller became READY */
/* the start guard (ble_glue.h): survives a reset, so a start that crashes or freezes the unit is not
 * tried again at the next power-on */
ble_guard_t ble_guard __attribute__((section(".noinit")));

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
static void ble_start(void)
{
    const xb_header_t *h = FM1_BLE_HDR;
    xb_mbox_t *mb = &ble_r.mb;
    int rc;
    uint32_t bus;
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
    if (!ble_boot_on)
        return;
    if (!fm1_c1_on) {                           /* one core (fm1-x0x#10): no Bluetooth */
        ble_guard.trying = 0;                   /* not the controller's fault: no block */
        ble_dead = 1;
        return;
    }
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
    bus = fm1_guard_bus_pause();                /* JieLi's RF code: not for the bus-invalid checks */
    fm1_wdt_feed();                             /* init may take FM1_BLE_INIT_MS of the 8 s */
    {
        uint32_t f = irq_save(), t0 = fm1_ticks();
        rc = FL_FAR(fm1_cpu1_run_wait_ram)((void (*)(uint32_t))(uintptr_t)h->init,
                                           (uint32_t)(uintptr_t)&ble_r.boot, 24000u * FM1_BLE_INIT_MS);
        ble_guard.init_ms = (fm1_ticks() - t0) / (FM1_TICKS_PER_US * 1000u);
        irq_restore(f);
    }
    fm1_wdt_feed();
    ble_record();
    ble_guard_started(&ble_guard, rc, rc == 0 && mb->state == XB_ST_READY);
    if (rc != 0) {                              /* it did not come back: one core from now on */
        fm1_cpu1_hold();
        fm1_c1_gave_up = 4;
        fm1_guard_bus_resume(bus);
        ble_dead = 1;
        return;
    }
    if (mb->state != XB_ST_READY) {             /* its interrupts off on core 1, no poll */
        fm1_cpu1_irqs_off();
        fm1_guard_bus_resume(bus);
        ble_dead = 1;
        return;
    }
    fm1_c1_mb.poll = (void (*)(void))(uintptr_t)h->poll;
    fm1_ble_sync();
    ble_ok_at = fm1_ms;
    ble_up = 1;                                 /* the bus-invalid checks stay off while it runs */
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
        if (n > 0)
            bleh_rx(h, t, ble_r.fr, (uint16_t)n);
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
    if (n + 10u > max)
        return n;
    w[n++] = ble_img | (uint32_t)ble_boot_on << 8 | (uint32_t)ble_up << 16 | (uint32_t)ble_dead << 24;
    w[n++] = ble_want | (uint32_t)ble_host_on << 8 | (uint32_t)ble_blocked << 16 | (uint32_t)fm1_c1_on << 24;
    w[n++] = fm1_c1_gave_up;
    w[n++] = (uint32_t)plat_ble_state();
    w[n++] = ble_up ? ble_r.mb.state : 0xFFFFFFFFu;
    w[n++] = ble_up ? ble_r.mb.err : 0xFFFFFFFFu;
    w[n++] = ble_up ? ble_r.mb.polls : 0;
    w[n++] = fm1_c1_mb.poll_max;
    w[n++] = ble_up ? ble_r.h.state : 0xFFFFFFFFu;
    w[n++] = fm1_c1_trace;
    return n;
}
