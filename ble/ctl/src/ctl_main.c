// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * The image header, init() and poll() (the two entries the app calls on core 1), and the H4
 * transport between the controller and the app's mailbox (x0x_ble_abi.h). */
#include "ctl_port.h"

typedef struct {
    const char *name;
    void (*init)(const void *cfg);
    int  (*open)(void);
    int  (*close)(void);
    void (*register_packet_handler)(void (*h)(int type, const u8 *p, int size));
    int  (*can_send_packet_now)(u8 type);
    int  (*send_packet)(int type, const u8 *p, int size);
    int  (*set_baudrate)(u32 baud);
    void (*reset_link)(void);
    void (*set_sco_config)(u16 voice, int n);
} hci_transport_t;   /* layout from include_lib/btctrler/hci_transport.h */

#ifndef CTL_HOST
void hci_packet_handler(int type, const u8 *packet, int size);

/* The H4 transport the controller gets (btctrler_task_init). In btctrler.a,
 * hci_controller_init calls register_packet_handler(hci_h4_download_data): that is the
 * host -> controller entry. The controller sends its events and data with send_packet.
 * The library's own hci_transport_h4_controller_instance() is this, but its
 * can_send_packet_now always gives 0, and its send_packet is the controller -> host way. */
static void (*ctl_h2c)(int type, const u8 *p, int size);
static void ctl_tr_init(const void *cfg) { (void)cfg; }
static int ctl_tr_open(void) { return 0; }
static int ctl_tr_close(void) { return 0; }
static void ctl_tr_register(void (*hd)(int type, const u8 *p, int size)) { ctl_h2c = hd; }
static int ctl_tr_can_send(u8 type) { (void)type; return 1; }
static int ctl_tr_send(int type, const u8 *p, int size)
{
    hci_packet_handler(type, p, size);
    return 0;
}
static const hci_transport_t ctl_transport = {
    .name = "x0x", .init = ctl_tr_init, .open = ctl_tr_open, .close = ctl_tr_close,
    .register_packet_handler = ctl_tr_register, .can_send_packet_now = ctl_tr_can_send,
    .send_packet = ctl_tr_send,
};
extern int btctrler_task_init(const void *transport, const void *config);
#else
#include "../src_host/host_abi.h"
extern void btstack_init(void);
#endif
extern void bt_max_pwr_set(u8 pwr, u8 pg, u8 iq, u8 ble);
void ctl_delay_init(void);
void ctl_port_irq_enable_core(void);

/* Linker symbols (ctl.ld) */
extern u8 __ctl_data_lma[], __ctl_data_start[], __ctl_data_end[];
extern u8 __ctl_ramtext_lma[], __ctl_ramtext_start[], __ctl_ramtext_end[];
extern u8 __ctl_bss_start[], __ctl_bss_end[];
extern u8 __ctl_ram_lo[], __ctl_ram_hi[];
extern u8 __ctl_isr_stack_lo[], __ctl_isr_stack_hi[];
extern u8 __ctl_run_stack_hi[];
extern u8 __ctl_run_stack_lo[];
#ifdef CTL_HOST
extern u8 __ctl_ram2_lo[], __ctl_ram2_hi[];
#endif

const xb_boot_t *ctl_bp;
xb_mbox_t *ctl_mb;
static volatile u32 failed;

static u32 ctl_init(u32 bp);
static u32 ctl_poll(u32 unused);
int ctl_entry_init(const xb_boot_t *bp);
void ctl_entry_poll(void);

/* ---- the image header: the first bytes of the image (mkimage.py fills image_size and crc32) ---- */
#define BIT32(n) (1u << ((n) & 31u))
__attribute__((section(".ctl_header"), used))
const xb_header_t ctl_image_header = {
    .magic = XB_HDR_MAGIC, .hdr_version = XB_HDR_VERSION, .hdr_size = sizeof(xb_header_t),
    .fw_version = 0x00020000u,
    .abi = XB_ABI,
    .init = (u32)(uintptr_t)ctl_entry_init,
    .poll = (u32)(uintptr_t)ctl_entry_poll,
    .isr_stack_lo = (u32)(uintptr_t)__ctl_isr_stack_lo, .isr_stack_hi = (u32)(uintptr_t)__ctl_isr_stack_hi,
    .ram_lo = (u32)(uintptr_t)__ctl_ram_lo, .ram_hi = (u32)(uintptr_t)__ctl_ram_hi,
#ifdef CTL_HOST
    .ram2_lo = (u32)(uintptr_t)__ctl_ram2_lo, .ram2_hi = (u32)(uintptr_t)XB_RAM2_HI,   /* the bond store too */
    /* p2_size, p2_crc: mkimage.py */
#endif
    /* BLE_RX 29, AES 31; BREDR 40, BT_CLKN 41, BT_DBG 42, BLE_EVENT 45 (hwi.h). No timer: the
     * clock is the app's free-running counter (ctl_timer.c). */
    .irq_lo = BIT32(29) | BIT32(31),
    .irq_hi = BIT32(40) | BIT32(41) | BIT32(42) | BIT32(45),
};

#ifdef CTL_HOST
/* attempt 45 DIAG, after READY, in the two live words (DIAG "took" and "gave"):
 *   took: bits 0-7 frames from the app, 8-15 host events, 16-23 the last event code, 24-31 timer runs
 *   gave: bits 0-7 frames to the app, 8-11 / 12-15 posts / pend returns of the controller task,
 *         16-19 / 20-23 the same for the host task, 24-25 the tasks that wait, 26-27 task_create
 *         failures (attempt 46), 28-29 the tasks that ran */
static u8 d_took, d_gave;
extern u8 ctl_d_tc_fail, ctl_d_post[2], ctl_d_pend[2], ctl_d_ran, ctl_d_timers, ctl_d_ev_n, ctl_d_ev_last;
u32 ctl_os_d_waiting(void);
__attribute__((noinline, section(".bt_stack_code.d45"))) static void d_live(void)
{
    ctl_mb->dbg[XB_DBG_INIT_MS] = d_took | (u32)ctl_d_ev_n << 8 | (u32)ctl_d_ev_last << 16 |
                                  (u32)ctl_d_timers << 24;
    ctl_mb->dbg[XB_DBG_MS] = d_gave | (u32)ctl_d_post[0] << 8 | (u32)ctl_d_pend[0] << 12 |
                             (u32)ctl_d_post[1] << 16 | (u32)ctl_d_pend[1] << 20 |
                             ctl_os_d_waiting() << 24 | (u32)(ctl_d_tc_fail > 3u ? 3u : ctl_d_tc_fail) << 26 |
                             (u32)ctl_d_ran << 28;
}
/* ---- image -> app: BLE-MIDI packets and the link state (host_abi.h) ---- */
void hx_to_app(unsigned char type, const unsigned char *p, unsigned n)
{
    u32 f = ctl_port_irq_save();
    if (xb_frame_put(&ctl_mb->c2h, type, p, n) != 0)
        ctl_mb->c2h_drops++;
    else if (d_gave < 255u)
        d_gave++;
    ctl_port_irq_restore(f);
}
static u8 rx_pkt[XB_FRAME_MAX] __attribute__((section(".ctl_ram2")));
/* app -> image: a MIDI packet stays in the ring while the host's send buffer has no room for it
 * (back-pressure: the app's packer waits, nothing is dropped) */
void ctl_host_poll(void)
{
    u8 type;
    u32 n;
    int k;
    while ((k = xb_frame_peek(&ctl_mb->h2c, &type, &n)) != 0) {
        if (k > 0 && type == XB_HX_MIDI && !hx_can_send(n))
            break;
        k = xb_frame_get(&ctl_mb->h2c, &type, rx_pkt, sizeof rx_pkt);
        if (k > 0) {
            if (d_took < 255u) d_took++;
            hx_from_app(type, rx_pkt, (unsigned)k);
        }
    }
}
#else
/* ---- controller -> app ---- */
void hci_packet_handler(int type, const u8 *packet, int size)
{
    u32 f = ctl_port_irq_save();
    if (size < 0 || xb_frame_put(&ctl_mb->c2h, (u8)type, packet, (u32)size) != 0)
        ctl_mb->c2h_drops++;              /* the app's flow control (credits) keeps this rare */
    else if (ctl_mb->state == XB_ST_READY)
        ctl_mb->dbg[XB_DBG_MS]++;         /* after READY: frames the controller gave to the app */
    ctl_port_irq_restore(f);
}

/* ---- app -> controller ---- */
static u8 rx_pkt[XB_FRAME_MAX];
void ctl_host_poll(void)
{
    xb_ring_t *r = &ctl_mb->h2c;
    u8 type;
    u32 n;
    int k;
    while ((k = xb_frame_peek(r, &type, &n)) != 0) {
        if (k < 0) {                      /* a bad length: drop all (xb_frame_get) */
            xb_frame_get(r, &type, rx_pkt, sizeof rx_pkt);
            return;
        }
        /* The HCI flow control is the command and ACL credits in the host. */
        if (!ctl_h2c)
            return;                       /* the controller did not register its entry (yet) */
        k = xb_frame_get(r, &type, rx_pkt, sizeof rx_pkt);
        if (k > 0) {
            ctl_mb->dbg[XB_DBG_INIT_MS]++; /* after READY: frames given to the controller */
            ctl_h2c(type, rx_pkt, k);
        }
    }
}

#endif

/* the controller stops for good: its interrupts off on core 1, state FAILED. The app sees it
 * and stops calling poll(). */
#ifdef CTL_HOST
/* health for DIAG: heap, the least free stack bytes / 16 (attempt 48: in FAILED too) */
__attribute__((noinline, section(".bt_stack_code.d48"))) static void d_health(void)
{
    u32 t = ctl_os_stack_free() >> 4, r = ctl_stack_free(__ctl_run_stack_lo, __ctl_run_stack_hi) >> 4,
        i = ctl_stack_free(__ctl_isr_stack_lo, __ctl_isr_stack_hi) >> 4;
    ctl_mb->hx.heap = ctl_heap_stat();
    ctl_mb->hx.stacks = (t > 255u ? 255u : t) | (r > 255u ? 255u : r) << 8 | (i > 255u ? 255u : i) << 16 |
                        (ctl_q_full > 255u ? 255u : ctl_q_full) << 24;
}
#endif

void ctl_fail(u32 err)
{
    volatile u32 *cfg = (volatile u32 *)(uintptr_t)(ctl_bp ? ctl_bp->icfg_base : 0u);
    u32 n;
    ctl_port_irq_save();
    failed = 1;
    if (ctl_mb) {
        for (n = 0; n < 128u && cfg; n++)
            if (ctl_mb->irqs_routed[n >> 5] & (1u << (n & 31u)))
                cfg[n >> 3] &= ~(1u << ((n & 7u) * 4u));
        ctl_mb->err = err;
#ifdef CTL_HOST
        /* attempt 47: DIAG keeps took/gave (INIT_MS, MS) in FAILED too; the heap use goes to hx.heap,
         * the failing caller is in RA (P33_SYSTEM_RESET, assert) */
        d_health();
        d_live();
#else
        ctl_mb->dbg[XB_DBG_MS] = ctl_heap_stat();
#endif
        XB_SYNC();
        ctl_mb->state = XB_ST_FAILED;
        XB_SYNC();
    }
    for (;;)                              /* the app holds core 1 when its job does not come back */
        ctl_port_cpu_relax();
}

static int boot_ok(const xb_boot_t *bp)
{
    return bp && bp->magic == XB_BOOT_MAGIC && bp->abi == XB_ABI && bp->mbox &&
           bp->tick_addr && bp->vec_base && bp->icfg_base;
}

/* the start record for the app (xb_mbox_t dbg): the step and the time */
/* attempt 41: a step inside a stage (0x300 + k in stage 3), with the heap use in place of the time */
void ctl_mark(u32 n)
{
    ctl_mb->dbg[XB_DBG_STAGE] = n;
    ctl_mb->dbg[XB_DBG_MS] = ctl_heap_stat();
    ctl_mb->dbg[XB_DBG_RA] = ctl_heap_main_size();
    ctl_mb->dbg[XB_DBG_STALL] = ctl_heap_free2();
    XB_SYNC();
}
void ctl_mark_v(u32 n, u32 v)
{
    ctl_mb->dbg[XB_DBG_DELAYS] = v;
    ctl_mark(n);
}
static void stage(u32 n)
{
    ctl_mb->dbg[XB_DBG_STAGE] = n;
    ctl_mb->dbg[XB_DBG_MS] = ctl_ms();
    XB_SYNC();
}

/* core 1, once, as an app job, with the app's core 0 waiting in RAM: set up the RAM, the
 * controller and its interrupts, then run it until it settles (the RF trim). 0 = ready. */
static u32 ctl_init(u32 arg)
{
    const xb_boot_t *bp = (const xb_boot_t *)(uintptr_t)arg;
    u32 t0, t_init, idle = 0;
    memcpy(__ctl_ramtext_start, __ctl_ramtext_lma, (size_t)(__ctl_ramtext_end - __ctl_ramtext_start));
    memcpy(__ctl_data_start, __ctl_data_lma, (size_t)(__ctl_data_end - __ctl_data_start));
    memset(__ctl_bss_start, 0, (size_t)(__ctl_bss_end - __ctl_bss_start));
#ifdef CTL_HOST
    memset(__ctl_ram2_lo, 0, (size_t)(__ctl_ram2_hi - __ctl_ram2_lo));   /* not the bond store above it */
    memset(__ctl_isr_stack_lo, 0xA5, (size_t)(__ctl_isr_stack_hi - __ctl_isr_stack_lo));
    {   /* the run stack below this frame */
        volatile u8 here;
        u8 *top = (u8 *)((uintptr_t)&here & ~3u) - 256;
        if (top > __ctl_run_stack_lo)
            memset(__ctl_run_stack_lo, 0xA5, (size_t)(top - __ctl_run_stack_lo));
    }
#endif
    if (!boot_ok(bp))
        return XB_E_BOOT;
    (void)ctl_port_irq_save();            /* off for the whole init (it runs on core 0, before core 1 starts) */
    ctl_bp = bp;
    ctl_mb = (xb_mbox_t *)(uintptr_t)bp->mbox;
    ctl_mb->err = XB_E_NONE;
    ctl_mb->state = XB_ST_INIT;
    XB_SYNC();
    ctl_heap_init();
    ctl_delay_init();
    ctl_os_init();
    ctl_port_tick_start(1000);
    t_init = ctl_ms();
    stage(XB_STG_ENTRY);
    stage(XB_STG_OS);
    stage(XB_STG_H4);
    ctl_mark(0x301);
    bt_max_pwr_set(bp->pwr_max, 6, 6, bp->pwr_ble);
    ctl_mark(0x302);
#ifdef CTL_HOST
    /* attempt 59: the host profiles/handlers are set up AFTER the stack init (hx_start, once the
     * host task has run btstack_task_init), as the stock app does. Running hx_init here (before
     * btstack_init) let btstack_task_init's l2cap/hci init wipe le_hci_stack and drop our
     * hci_event_callback(hx_events): only l2cap_hci_event_handler survived, so no HCI event ever
     * reached hx_events (ctl_d_ev_n 0, no GATT). */
    ctl_mark(0x320);
    btstack_init();                       /* its task starts the controller (btctrler_task_init) */
    ctl_mark(0x330);
#else
    btctrler_task_init(&ctl_transport, NULL);
#endif
    stage(XB_STG_TASK);
    /* The RF trim runs in the controller task during the settle loop. Its inner loops are in RAM, but
     * JieLi's interrupt handlers are in the flash, and flash reads can come back wrong under the trim's
     * high current (felucca-bsp#2). Core 1 died twice at the same place in the trim (rd_adc_peak).
     * So the interrupts stay off until the trim has settled: no flash reads from interrupts. */
    (void)ctl_port_irq_save();            /* off, and the tasks' critical sections keep them off */
    t0 = ctl_ms();
    stage(XB_STG_SETTLE);
    while ((s32)(ctl_ms() - t0) < (s32)bp->settle_ms) {   /* the RF trim runs in the controller task */
        if (ctl_os_run_once())
            idle = 0;
        else if (++idle > 2000u && (s32)(ctl_ms() - t0) > 50)
            break;                        /* every task waits: settled */
    }
    stage(XB_STG_SETTLED);
#ifdef CTL_HOST
    ctl_mb->hx.heap = ctl_heap_stat();
#endif
    /* The interrupts stay off until the first poll on core 1. */
    ctl_mb->dbg[XB_DBG_INIT_MS] = ctl_ms() - t_init;
    if (ctl_mb->err != XB_E_NONE) {
        ctl_mb->state = XB_ST_FAILED;
        return ctl_mb->err;
    }
    stage(XB_STG_READY);
    ctl_mb->dbg[XB_DBG_MS] = 0;           /* from now on: HCI counters (ctl_host_poll, c2h) */
    ctl_mb->dbg[XB_DBG_INIT_MS] = 0;
    ctl_mb->state = XB_ST_READY;
    XB_SYNC();
    return 0;
}

/* core 1, the app's idle loop: one pass on the image's own stack */
static u32 ctl_poll(u32 unused)
{
    (void)unused;
    if (failed)
        return 0;
    if (!ctl_mb->polls) {                 /* the first poll, on core 1: its interrupts (core 1 started after init) */
        ctl_port_irq_reroute(CTL_BLE_IRQ_PRIO);
        ctl_port_irq_enable_core();
        ctl_port_irq_restore(0);
    }
    ctl_mb->polls++;
    ctl_os_run_once();
#ifdef CTL_HOST
    hx_start();                           /* attempt 50: once the host task waits */
    d_live();
#endif
#ifdef CTL_HOST
    if (ctl_mb->polls < 64u || (ctl_mb->polls & 1023u) == 0) {   /* health for DIAG (attempt 51: the first 64 polls too, so a reset leaves fresh values) */
        d_health();
    }
#endif
    return 0;
}

/* the two entries in the header: they move to the image's own stack first */
int ctl_entry_init(const xb_boot_t *bp)
{
    return (int)ctl_asm_stack_call(ctl_init, (u32)(uintptr_t)bp, __ctl_run_stack_hi);
}
void ctl_entry_poll(void)
{
    ctl_asm_stack_call(ctl_poll, 0, __ctl_run_stack_hi);
}
