// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Port layer: CPU and interrupt access for core 1 (pi32v2). Stage 2: written from the X0X app's
 * own core-1 and interrupt code and the JieLi SDK headers. Not yet tried on a device (Stage 3).
 *
 *   ctx_init / ctx_switch   cooperative task switch (ctl_port_asm.S)
 *   irq_save / irq_restore  cli / sti on this core, with a flag for irq_disabled
 *   in_irq                  sp on the interrupt stack (the image owns every stack it runs on)
 *   request_irq             vector + core 1's 4-bit configuration (enable, priority)
 *   tick_start              the ms clock comes from the app's free-running counter: no timer IRQ
 *   wdt_feed                nothing: the app (core 0) owns the watchdog
 *   wait_for_irq            nothing: poll() returns to the app's idle loop instead of sleeping
 *   cpu_relax               csync
 */
#include "ctl_port.h"

extern u8 __ctl_isr_stack_lo[], __ctl_isr_stack_hi[];
extern const xb_header_t ctl_image_header;

static volatile u32 irq_off;      /* 1 between irq_save and the matching irq_restore */

void ctl_port_ctx_init(struct ctl_ctx *c, void *stack, u32 size, void (*entry)(void))
{
    u32 *top = (u32 *)(((uintptr_t)stack + size) & ~7u), i;
    for (i = 1; i <= 13u; i++)     /* pc and r4-r15: all the entry, so the frame order does not matter */
        top[-(int)i] = (u32)(uintptr_t)entry;
    c->sp = top - 13;
}

void ctl_port_ctx_switch(struct ctl_ctx *from, struct ctl_ctx *to)
{
    ctl_asm_ctx_switch(&from->sp, &to->sp);
}

u32 ctl_port_irq_save(void)
{
    u32 prev;
    __asm__ volatile("cli" ::: "memory");
    prev = irq_off;
    irq_off = 1;
    return prev;
}

void ctl_port_irq_restore(u32 prev)
{
    irq_off = prev;
    if (!prev)
        __asm__ volatile("csync\n\tsti" ::: "memory");
}

int ctl_port_in_irq(void)
{
    u32 sp = ctl_asm_sp();
    return sp >= (u32)(uintptr_t)__ctl_isr_stack_lo && sp <= (u32)(uintptr_t)__ctl_isr_stack_hi;
}

int ctl_port_irq_disabled(void) { return irq_off != 0; }

static int irq_allowed(u8 n)
{
    u32 m = n < 32u ? ctl_image_header.irq_lo : n < 64u ? ctl_image_header.irq_hi : 0;
    return (m >> (n & 31u)) & 1u;
}

void ctl_port_request_irq(u8 n, u8 prio, void (*handler)(void))
{
    volatile u32 *vec = (volatile u32 *)(uintptr_t)ctl_bp->vec_base;
    volatile u32 *cfg = (volatile u32 *)(uintptr_t)ctl_bp->icfg_base;
    u32 sh = (n & 7u) * 4u, f;
    if (!irq_allowed(n) || !vec || !cfg) {   /* only the interrupts the header names */
        ctl_mb->err = XB_E_BOOT;
        return;
    }
    f = ctl_port_irq_save();
    vec[n] = (u32)(uintptr_t)handler;
    cfg[n >> 3] = (cfg[n >> 3] & ~(0xFu << sh)) | ((((u32)prio & 7u) << 1 | 1u) << sh);
    ctl_mb->irqs_routed[n >> 5] |= 1u << (n & 31u);
    XB_SYNC();
    ctl_port_irq_restore(f);
}

/* core 1's interrupt bank again for every routed interrupt: the app cleared the bank when it started
 * core 1, after the init had routed them */
void ctl_port_irq_reroute(u8 prio)
{
    volatile u32 *cfg = (volatile u32 *)(uintptr_t)ctl_bp->icfg_base;
    u32 n, sh;
    for (n = 0; n < 64u; n++)
        if (ctl_mb->irqs_routed[n >> 5] & (1u << (n & 31u))) {
            sh = (n & 7u) * 4u;
            cfg[n >> 3] = (cfg[n >> 3] & ~(0xFu << sh)) | ((((u32)prio & 7u) << 1 | 1u) << sh);
        }
    XB_SYNC();
}

/* core 1 takes interrupts from here on (icfg bit 8, as the app's fm1_irq_enable_all) */
void ctl_port_irq_enable_core(void)
{
    u32 v;
    __asm__ volatile("%0 = icfg" : "=r"(v));
    v |= 0x100u;
    __asm__ volatile("icfg = %0" ::"r"(v) : "memory");
}

void ctl_port_tick_start(u32 hz) { (void)hz; ctl_ms_init(); }
void ctl_port_wdt_feed(void) {}
void ctl_port_wait_for_irq(void) {}
void ctl_port_cpu_relax(void) { __asm__ volatile("csync" ::: "memory"); }
