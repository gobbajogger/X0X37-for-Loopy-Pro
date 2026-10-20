// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Platform services the controller libraries call: IRQ hookup, critical sections, low-power
 * hooks, delays, clocks, CRC, logging (off), reset. Hardware access goes through ctl_port_*
 * (ctl_port.c). */
#include "ctl_port.h"

/* The SDK uses strcmp for fixed controller names; a small generic implementation keeps the
 * dual-role image inside its flash slot without pulling the toolchain's full libc version. */
int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* ---- interrupts ---- */
void request_irq(unsigned char index, unsigned char prio, void (*handler)(void), unsigned char cpu_id)
{
    (void)prio; (void)cpu_id;          /* this image runs on core 1: always route to it */
    ctl_port_request_irq(index, CTL_BLE_IRQ_PRIO, handler);
}

static u32 irq_nest, irq_saved;
void local_irq_disable(void) { u32 f = ctl_port_irq_save(); if (irq_nest++ == 0) irq_saved = f; }
void local_irq_enable(void)  { if (irq_nest && --irq_nest == 0) ctl_port_irq_restore(irq_saved); }
void __local_irq_disable(void) { local_irq_disable(); }
void __local_irq_enable(void)  { local_irq_enable(); }
void sys_local_irq_disable(void) { local_irq_disable(); }
void sys_local_irq_enable(void)  { local_irq_enable(); }
int cpu_in_irq(void) { return ctl_port_in_irq(); }
int cpu_irq_disabled(void) { return ctl_port_irq_disabled(); }

/* AES block is used only by this core: a lock is not needed. */
void aes_hw_lock(void) {}
void aes_hw_unlock(void) {}

/* ---- low power: never sleep. low_power_get must return non-null
 *      (felucca-bsp #2 finding). ---- */
static u32 lp_dummy;
void *low_power_get(void *priv, const void *ops) { (void)priv; (void)ops; return &lp_dummy; }
void low_power_put(void *priv) { (void)priv; }
void low_power_on(void) {}
void low_power_request(char *name) { (void)name; }
void low_power_exit_request(void) {}
void low_power_hw_unsleep_lock(void) {}
void low_power_hw_unsleep_unlock(void) {}
s32 low_power_trace_drift(u32 usec) { (void)usec; return 0; }
u8 low_power_get_osc_type(void) { return ctl_bp->osc_type; }
int power_is_poweroff_post(void) { return 0; }

/* ---- clocks and delays (RAM: the RF trim calls them while flash is off) ---- */
/* The radio library (wf_rf_common_init) asks only for "osc": the crystal, 24 MHz on the FM-1 and the
 * SMK-37 (fm1-ctl's shim gives 24 MHz too). Before, "osc" got the system clock (360 MHz): the radio
 * then set its clocks for a wrong crystal, and both cores stopped in the TX IQ trim (watchdog reset). */
#define CTL_OSC_HZ 24000000
int clk_get(const char *name)
{
    if (name && name[0] == 'o') return CTL_OSC_HZ;
    if (name && name[0] == 'l') return (int)ctl_bp->lsb_clk_hz;
    if (name && name[0] == 'h') return (int)ctl_bp->hsb_clk_hz;
    return (int)ctl_bp->sys_clk_hz;
}
/* RAM code: the RF trim calls these while the flash is off. They read the app's free-running
 * counter, so they call nothing and need no calibration. (The app's core 0 feeds the watchdog.) */
static volatile const u32 *dly_tick;
static u32 dly_per_us;
CTL_RAMTEXT void delay_us(unsigned int us)
{
    volatile const u32 *t = dly_tick;
    u32 t0, span = (u32)us * dly_per_us;
    u32 same = 0, last;
    t0 = last = *t;                        /* boot_ok requires tick_addr before ctl_delay_init */
    for (;;) {
        u32 now = *t;
        if ((u32)(now - t0) >= span)
            break;
        if (now != last) {
            last = now;
            same = 0;
        } else if (++same > 1000000u) {   /* the tick counter stopped: count it, go on */
            if (ctl_mb)
                ctl_mb->dbg[XB_DBG_STALL]++;
            break;
        }
    }
}
CTL_RAMTEXT void delay(unsigned int cnt)
{
    for (volatile unsigned int i = cnt; i; i--) {}
}
void ctl_delay_init(void)
{
    dly_tick = (volatile const u32 *)(uintptr_t)ctl_bp->tick_addr;
    dly_per_us = ctl_bp->ticks_per_ms ? ctl_bp->ticks_per_ms / 1000u : 24u;
    if (!dly_per_us) dly_per_us = 1;
}

/* ---- ADC: not wired in this image ---- */
u32 adc_add_sample_ch(u32 ch) { (void)ch; return 0; }
u32 adc_get_value(u32 ch) { (void)ch; return 0; }

/* ---- CRC (software) ---- */
u16 CRC16(const void *ptr, u32 len)
{
    const u8 *p = ptr; u16 crc = 0;     /* CRC-16/XMODEM: poly 0x1021, init 0 */
    while (len--) {
        crc ^= (u16)(*p++) << 8;
        for (int b = 0; b < 8; b++) crc = (crc & 0x8000u) ? (u16)((crc << 1) ^ 0x1021u) : (u16)(crc << 1);
    }
    return crc;
}
u16 crc_get_16bit(const void *ptr, u32 len) { return CRC16(ptr, len); }
u32 crc_get_32bit(const char *src)
{
    u32 crc = 0xFFFFFFFFu;               /* CRC-32 (IEEE) of a NUL-terminated string */
    while (src && *src) {
        crc ^= (u8)*src++;
        for (int b = 0; b < 8; b++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* ---- WiFi block: not used by this image ---- */
void wlc_main(void) {}

/* ---- logging: off ---- */
void log_print(int level, const char *tag, const char *fmt, ...) { (void)level; (void)tag; (void)fmt; }
int printf(const char *fmt, ...) { (void)fmt; return 0; }
int puts(const char *s) { (void)s; return 0; }
int putchar(int c) { return c; }
void printf_buf(u8 *buf, u32 len) { (void)buf; (void)len; }
void put_buf(const u8 *buf, int len) { (void)buf; (void)len; }
void cpu_assert_debug(void)
{
    if (ctl_mb)                           /* the caller, for DIAG (XB_DBG_INIT_MS is free when init fails) */
        ctl_mb->dbg[XB_DBG_INIT_MS] = (u32)(uintptr_t)__builtin_return_address(0);
    ctl_fail(XB_E_ASSERT);
}

/* ---- reset: the app owns the system. The controller stops; the app goes on without it. ---- */
void P33_SYSTEM_RESET(void)
{
    if (ctl_mb)                           /* the library's caller, for DIAG */
        ctl_mb->dbg[XB_DBG_INIT_MS] = (u32)(uintptr_t)__builtin_return_address(0);
    ctl_fail(XB_E_RESET);
}
