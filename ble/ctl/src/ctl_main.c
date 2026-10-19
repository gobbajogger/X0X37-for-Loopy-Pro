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
    /* BLE_RX 29, AES 31; BREDR 40, BT_CLKN 41, BT_DBG 42, BLE_EVENT 45 (hwi.h). No timer: the
     * clock is the app's free-running counter (ctl_timer.c). */
    .irq_lo = BIT32(29) | BIT32(31),
    .irq_hi = BIT32(40) | BIT32(41) | BIT32(42) | BIT32(45),
};

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

/* the controller stops for good: its interrupts off on core 1, state FAILED. The app sees it
 * and stops calling poll(). */
void ctl_fail(u32 err)
{
    volatile u32 *cfg = (volatile u32 *)(uintptr_t)(ctl_bp ? ctl_bp->icfg_base : 0u);
    u32 n;
    ctl_port_irq_save();
    failed = 1;
    if (ctl_mb) {
        for (n = 0; n < 64u && cfg; n++)
            if (ctl_mb->irqs_routed[n >> 5] & (1u << (n & 31u)))
                cfg[n >> 3] &= ~(1u << ((n & 7u) * 4u));
        ctl_mb->err = err;
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

/* core 1, once, as an app job, with the app's core 0 waiting in RAM: set up the RAM, the
 * controller and its interrupts, then run it until it settles (the RF trim). 0 = ready. */
static u32 ctl_init(u32 arg)
{
    const xb_boot_t *bp = (const xb_boot_t *)(uintptr_t)arg;
    u32 t0, t_init, idle = 0;
    memcpy(__ctl_ramtext_start, __ctl_ramtext_lma, (size_t)(__ctl_ramtext_end - __ctl_ramtext_start));
    memcpy(__ctl_data_start, __ctl_data_lma, (size_t)(__ctl_data_end - __ctl_data_start));
    memset(__ctl_bss_start, 0, (size_t)(__ctl_bss_end - __ctl_bss_start));
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
    bt_max_pwr_set(bp->pwr_max, 6, 6, bp->pwr_ble);
    btctrler_task_init(&ctl_transport, NULL);
    /* The RF trim runs in the controller task during the settle loop. Its inner loops are in RAM, but
     * JieLi's interrupt handlers are in the flash, and flash reads can come back wrong under the trim's
     * high current (felucca-bsp#2). Core 1 died twice at the same place in the trim (rd_adc_peak).
     * So the interrupts stay off until the trim has settled: no flash reads from interrupts. */
    (void)ctl_port_irq_save();            /* off, and the tasks' critical sections keep them off */
    t0 = ctl_ms();
    while ((s32)(ctl_ms() - t0) < (s32)bp->settle_ms) {   /* the RF trim runs in the controller task */
        if (ctl_os_run_once())
            idle = 0;
        else if (++idle > 2000u && (s32)(ctl_ms() - t0) > 50)
            break;                        /* every task waits: settled */
    }
    /* The interrupts stay off until the first poll on core 1. */
    ctl_mb->dbg[XB_DBG_INIT_MS] = ctl_ms() - t_init;
    if (ctl_mb->err != XB_E_NONE) {
        ctl_mb->state = XB_ST_FAILED;
        return ctl_mb->err;
    }
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
