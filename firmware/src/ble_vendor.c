/* SPDX-License-Identifier: GPL-3.0-only */
/* Opt-in SMC vendor-host TEST application adapter. No HCI/GATT protocol engine.
 * Guard/start/attach code retained from baseline ble.c (816e0d3). USB Loopy and
 * all musical controls remain; this profile deliberately has no BLE peripheral.
 * Requires independently admitted immutable UI data and complete ABI4 manifest.
 * No runtime or hardware admission is implied by compiling this adapter. */
#include "fm1_ble.h"
#include "fm1_supply.h"
#include "x0x_ble_segment.h"
#include "x0x_smc_host_messages.h"
#include "ble/ble_glue.h"
#include "ble/smc_host_ingress.h"
#include "fm1_smc_vendor.h"
extern int smc_app_ui_admitted(void);
extern int smc_ble_midi_accept(const uint8_t *, uint8_t);
extern void smc_ble_link(int);
extern void smk_inlog(uint32_t, uint32_t);
static void ble_record(void);
typedef struct {
    xb_mbox_t mb;
    xsm_boot_t boot;
    smc_host_ingress_t ingress;
    uint8_t fr[XSM_NOTIFY_MAX];
} ble_vendor_ram_t;
static ble_vendor_ram_t ble_r __attribute__((section(".ble_ram"), aligned(64)));
static uint32_t ble_alive[2] __attribute__((section(".ble_ram")));
static uint32_t ble_rails[2] __attribute__((section(".ble_ram")));
static uint8_t ble_img, ble_boot_on, ble_up, ble_want, ble_host_on, ble_dead, ble_blocked;
static uint8_t ble_c0_ready, ble_central_link_seen;
static uint32_t ble_ok_at;
static volatile uint32_t ble_feedback_epoch;
ble_guard_t ble_guard __attribute__((section(".noinit")));
extern uint32_t _ram_sink[], _ram_sink_end[];
#define BLE_SINK_FILL(i) (0x5A17C0DEu ^ ((i) * 0x01010101u))
static void ble_sink_fill(void)
{
    uint32_t i, n = (uint32_t)(_ram_sink_end - _ram_sink);
    for (i = 0; i < n; i++)
        _ram_sink[i] = BLE_SINK_FILL(i);
}
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
static void ble_prepare(int on)
{
    const xb4_manifest_t *h = FM1_SMC_HDR;
    fm1_t5w_boot();                             /* before TIMER5 starts: keep the last boot's frame watch */
    ble_img = !smc_app_ui_admitted() ? BLE_IMG_BAD : fm1_smc_image_check();
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
    fm1_guard_xip_hi = XB4_XIP(XB4_PRIMARY_OFF) + XB4_PRIMARY_SIZE - 1u;
    fm1_guard_ram_hi = h->ram_hi;
}
static void ble_start(void)
{
    const xb4_manifest_t *h = FM1_SMC_HDR;
    xb_mbox_t *mb = &ble_r.mb;
    int rc;
    uint32_t bus, stk, top;
    uint8_t addr[6]; char name[9];
    {
        static const uint8_t none[1] = {0};
        if (flash_ok)                           /* the factory record (USRTRIM): one address per unit */
            ble_addr_of(fm1_xip_ptr(SMK_TRIM_FLASH), 0x22Cu, addr, name);
        else
            ble_addr_of(none, 1u, addr, name);
    }
    memset(&ble_r.ingress, 0, sizeof ble_r.ingress);
    ble_c0_ready = 0;
    if (!ble_boot_on)
        return;
    /* Irreversible for this boot, including failed initialization attempts. */
    bp_update_attempt(&smc_update_gate);
    {
        uint8_t *z = (uint8_t *)mb;
        uint32_t i;
        for (i = 0; i < sizeof *mb; i++)
            z[i] = 0;
    }
    mb->magic = XB_MBOX_MAGIC;
    mb->abi = XSM_ABI;
    xb_ring_init(&mb->h2c);
    xb_ring_init(&mb->c2h);
    fm1_ble_boot_fill(&ble_r.boot.base, mb);
    ble_r.boot.base.abi = XSM_ABI;
    ble_r.boot.ext_magic = XSM_BOOT_EXT_MAGIC;
    ble_r.boot.ext_size = sizeof ble_r.boot;
    memcpy(ble_r.boot.address, addr, sizeof addr);
    ble_r.boot.reserved[0] = ble_r.boot.reserved[1] = 0;
    ble_r.boot.radio_desired = 0;
    ble_r.boot.feedback_epoch = 0; ble_feedback_epoch = 0;
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
    {   /* attempt 27: the stock supply setup (DCDC13, VDC14 1.60 V, SYSVDD 1.38 V) before the RF init */
        uint32_t f = irq_save();
        ble_rails[0] = fm1_rails_read() | (uint32_t)fm1_p33_read(0x00u) << 24;
        (void)fm1_supply_stock();               /* its read-back: rails[1] (DIAG) */
        ble_rails[1] = fm1_rails_read() | (uint32_t)fm1_p33_read(0x00u) << 24;
        irq_restore(f);
    }
    {
        uint32_t f = irq_save(), t0 = fm1_ticks();
        stk = fm1_guard_stack_pause();          /* the image's run stack */
        rc = fm1_ble_init_here(h->init, &ble_r.boot.base);
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

#ifndef X0X_RAM_CANARY
#define X0X_RAM_CANARY 0
#endif
static void ble_attach(void)
{
    const xb4_manifest_t *h = FM1_SMC_HDR;
    if (!ble_c0_ready)
        return;
    if (!fm1_c1_on) {                           /* one core (fm1-x0x#10): no Bluetooth */
        ble_guard.trying = 0;                   /* not the controller's fault: no block */
        fm1_guard_bus_resume(FM1_GUARD_BUS_BITS);
        ble_dead = 1;
        return;
    }
    fm1_guard_watch_c0_stacks();               /* test 21: who changes core 0's interrupt stack */
    fm1_t5w_arm = 1;                            /* test 23: the timer 5 frame closed to all writers */
#if X0X_RAM_CANARY
    /* Test 26: the RAM canary, core 1 watches it. Test builds only: the area
     * overlaps the end of .pool when less than 8 KiB of the pool is free. */
    for (uint32_t *cw = (uint32_t *)(uintptr_t)FM1_CAN_LO; cw < (uint32_t *)(uintptr_t)FM1_CAN_HI; cw++)
        *cw = 0xFFFFFFFFu;
    fm1_can.n = fm1_can.scans = 0;
    fm1_can.armed = 0xCA9A11E5u;
#else
    fm1_can.armed = 0;                          /* .ble_ram survives a reset: clear an old arm */
#endif
    fm1_c0_dead.magic = 0;                      /* test 24: core 1 watches core 0's clock */
    fm1_c0_dead.n = 0;
    fm1_ble_sync();
    fm1_c1_c0_clock = &fm1_ms;
    fm1_c1_mb.poll = (void (*)(void))(uintptr_t)h->poll;
    fm1_ble_sync();
    ble_ok_at = fm1_ms;
    ble_up = 1;
}
static void ble_record(void)
{
    uint32_t i;
    fm1_ble_sync();
    ble_guard.mb_state = ble_r.mb.state;
    ble_guard.mb_err = ble_r.mb.err;
    for (i = 0; i < 6u; i++)
        ble_guard.dbg[i] = ble_r.mb.dbg[i];
}
static void ble_out(uint32_t pkt) { (void)pkt; } /* USB musical output remains in usb.c. */
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
static void ble_central_link_notify(int up)
{
    uint8_t next = (uint8_t)(up != 0);
    if (next != ble_central_link_seen) {
        ble_central_link_seen = next;
        smc_ble_link(next);
    }
}

static int ble_vendor_emit(void *ctx, const uint8_t *p, uint8_t n)
{
    (void)ctx;
    if (n == 3u) smk_inlog(4u, (uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 24);
    else if (n == 6u) smk_inlog(4u, (uint32_t)p[0] << 8 | (uint32_t)p[4] << 16);
    return smc_ble_midi_accept(p, n);
}
/* TIMER5 owns the copied ingress and dedicated SMC queue; never APC/Loopy. */
static void ble_service(void)
{
    uint32_t k;
    uint8_t type;
    if (!ble_up) { ble_feedback_epoch = 0; smc_hi_link(&ble_r.ingress, 0); ble_central_link_notify(0); return; }
    if (ble_r.mb.state != XB_ST_READY || !fm1_c1_on) {
        ble_record(); ble_fail(); ble_feedback_epoch = 0; smc_hi_link(&ble_r.ingress, 0); ble_central_link_notify(0); return;
    }
    ble_alive[0] = fm1_ms; ble_alive[1] = ble_r.mb.polls;
    if (ble_guard.trying && fm1_ms - ble_ok_at > BLE_GUARD_OK_MS) ble_guard_ok(&ble_guard);
    ble_r.boot.radio_desired = ble_want; fm1_ble_sync();
    ble_host_on = ble_want;
    if (!ble_want || ble_feedback_epoch != ble_r.boot.feedback_epoch) {
        ble_feedback_epoch = 0; smc_hi_link(&ble_r.ingress, 0); ble_central_link_notify(0); }
    for (k = 0; k < 8u; ++k) {
        int n = xb_frame_get(&ble_r.mb.c2h, &type, ble_r.fr, sizeof ble_r.fr);
        if (!n) break;
        if (n < 0) { ble_r.ingress.rejected++; continue; }
        if (type == XSM_LINK_SESSION && n == 6 && ble_r.fr[4] <= 1u) {
            uint32_t epoch = xsm_epoch_read(ble_r.fr);
            if (!epoch || epoch != ble_r.boot.feedback_epoch) continue;
            int up = ble_want && ble_r.fr[4];
            /* Duplicate up publications can occur when the following notify is
             * full. Do not reset the decoder or replace the producer snapshot. */
            if (up && ble_feedback_epoch == epoch && ble_r.ingress.connected) continue;
            ble_feedback_epoch = up ? epoch : 0;
            smc_hi_link(&ble_r.ingress, up);
            ble_central_link_notify(up);
        } else if (type == XSM_NOTIFY && ble_want && ble_r.ingress.connected &&
                   ble_feedback_epoch == ble_r.boot.feedback_epoch) {
            smc_hi_notification(&ble_r.ingress, ble_r.fr, (uint32_t)n, ble_vendor_emit, 0);
        } else if (type != XSM_EVENT) {
            ble_r.ingress.rejected++;
        }
    }
}
int plat_ble_state(void)
{
    if (ble_img == BLE_IMG_NONE) return PLAT_BLE_NOIMG;
    if (ble_img != BLE_IMG_OK) return PLAT_BLE_BADIMG;
    if (ble_dead) return PLAT_BLE_FAILED;
    if (!ble_boot_on) return ble_want ? PLAT_BLE_RESTART : PLAT_BLE_OFF;
    if (!ble_want) return PLAT_BLE_OFF;
    if (!ble_up) return PLAT_BLE_STARTING;
    return ble_r.ingress.connected ? PLAT_BLE_CONNECTED : PLAT_BLE_READY;
}
const char *plat_ble_name(void) { return "SMC-Mixer TEST"; }
uint32_t plat_ble_smc_session(void)
{
    uint32_t epoch = ble_feedback_epoch;
    fm1_ble_sync();
    return epoch;
}
int plat_ble_smc_send_session(uint32_t epoch, const uint8_t *msg, uint8_t len)
{
    fm1_ble_sync();
    return epoch && epoch == ble_feedback_epoch && ble_up && ble_host_on && ble_want && ble_r.ingress.connected &&
           xsm_feedback_put(&ble_r.mb.h2c, epoch, msg, len) == 0;
}
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    (void)msg; (void)len;
    return 0; /* This profile requires identity captured before producing MIDI. */
}
void plat_ble_set(int on)
{
    ble_want = (uint8_t)(on != 0);
    bp_update_request(&smc_update_gate, on);
    if (!on && ble_guard.blocked) {             /* BLUETOOTH OFF: the next power-on tries again */
        ble_guard_user_off(&ble_guard);
        if (ble_blocked) {
            ble_blocked = 0;
            ble_dead = 0;                       /* nothing started this boot: OFF, then RESTART */
        }
    }
}
static uint32_t ble_diag(uint32_t *w, uint32_t max)
{
    const uint32_t *g = (const uint32_t *)&ble_guard;
    uint32_t i, n = 0;
    for (i = 0; i < sizeof ble_guard / 4u && n < max; ++i) w[n++] = g[i];
    if (max - n < 15u) return n;
    w[n++] = XSM_ABI;
    w[n++] = ble_img | (uint32_t)ble_up << 8 | (uint32_t)ble_dead << 16;
    w[n++] = ble_r.mb.state; w[n++] = ble_r.mb.err; w[n++] = ble_r.mb.polls;
    w[n++] = ble_r.mb.c2h_drops;
    w[n++] = ble_r.ingress.notifications; w[n++] = ble_r.ingress.messages;
    w[n++] = ble_r.ingress.rejected; w[n++] = ble_r.ingress.dropped;
    w[n++] = ble_alive[0]; w[n++] = ble_alive[1];
    w[n++] = 0x534D4347u; /* versioned never-started update admission */
    w[n++] = (uint32_t)bp_update_allowed(&smc_update_gate) |
        (uint32_t)smc_update_gate.attempted << 1 | (uint32_t)smc_update_gate.requested_on << 2 | (uint32_t)ble_boot_on << 3;
    w[n++] = fm1_boot.p3_rst | (fm1_boot.rst_src & 0xFFFFFFu) << 8;
    return n;
}
