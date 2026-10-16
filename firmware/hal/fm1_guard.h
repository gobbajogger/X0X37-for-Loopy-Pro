/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM-1 hardware guards. Every violation raises the CPU exception (vector 1) and
 * ends in the fault report of fm1_irq.h, so install that first.
 *
 *   EMU stack limit   sp outside [_ustack_lo, _sstack_top] -> EMU_MSG bit 3.
 *                     One window for both stacks. Interrupts run on the system
 *                     stack (_sstack_lo.._sstack_top, 8 KiB): the crash records
 *                     of an interrupt (the audio render) always had sp there,
 *                     near its top, whatever the main loop was doing; the main
 *                     loop runs on the user stack. So the audio render and
 *                     everything nested on it share those 8 KiB (PERF shows the
 *                     deepest it has gone: plat_irq_stack_used).
 *   write limits      CPU0 writes into the two 256-byte stack guard bands and
 *                     into 0x01C7FD50..0x01C7FFFF (boot info, mailbox, vectors)
 *                     -> DBG_MSG bit 13. The top window is armed separately
 *                     with fm1_guard_lock_top() once every vector is
 *                     attached; fm1_guard_unlock_top() reopens it.
 *   bus-invalid       fetch/read/write of unmapped space (NULL page, XIP
 *                     writes) -> DBG_MSG bits 4, 5, 16..21.
 *   PC limit          instruction fetch outside [0x02000120, _etext]
 *                     -> DBG_MSG bit 12.
 */
#pragma once
#include <stdint.h>
#include "fm1_irq.h"

#define FM1_X2(off) (*(volatile uint32_t *)(0x1EEE000u + (off)))
#define FM1_WR_LIMIT_H(n) FM1_X2(0x280u + 4u * (n))
#define FM1_WR_LIMIT_L(n) FM1_X2(0x2C0u + 4u * (n))
#define FM1_C0_WR_LIMIT_EN FM1_X2(0x348u)
#define FM1_PC_LIMIT0_H FM1_X2(0x380u)
#define FM1_PC_LIMIT0_L FM1_X2(0x384u)
#define FM1_PC_LIMIT1_H FM1_X2(0x388u)
#define FM1_PC_LIMIT1_L FM1_X2(0x38Cu)
#define FM1_EMU_SSP_H (*(volatile uint32_t *)0x1EEF0D8u)
#define FM1_EMU_SSP_L (*(volatile uint32_t *)0x1EEF0DCu)
#define FM1_EMU_USP_H (*(volatile uint32_t *)0x1EEF0E0u)
#define FM1_EMU_USP_L (*(volatile uint32_t *)0x1EEF0E4u)

extern char _guard0[], _ustack_lo[], _guard1[], _sstack_lo[], _sstack_top[], _etext[];
extern uint32_t _rt_start[], _rt_end[];

enum { FM1_GUARD_STACK = 1, FM1_GUARD_WRITE = 2, FM1_GUARD_BUS = 4, FM1_GUARD_PC = 8 };
#define FM1_GUARD_BUS_BITS ((0x3Fu << 16) | (0x3u << 4))

/* a Bluetooth controller image (src/ble.c): the PC limits reach over its flash block and its RAM
 * too (0 = no controller). Set before fm1_guard_enable(FM1_GUARD_PC). */
static uint32_t fm1_guard_xip_hi, fm1_guard_ram_hi;

static void fm1__dbg_unlock(void)
{
    if (!(FM1_DBG_WR_EN & 1u))
        FM1_DBG_WR_EN = 0xE7u;
}

static void fm1__dbg_lock(void)
{
    if (FM1_DBG_WR_EN & 1u)
        FM1_DBG_WR_EN = 0xE7u;
}

static void fm1_guard_enable(uint32_t which)
{
    fm1__dbg_unlock();
    FM1_DBG_MSG_CLR = 0xFFFFFFFFu;
    if (which & FM1_GUARD_STACK) {
        FM1_EMU_CON &= ~(1u << 3);
        FM1_EMU_SSP_L = (uint32_t)(uintptr_t)_ustack_lo;
        FM1_EMU_SSP_H = (uint32_t)(uintptr_t)_sstack_top;
        FM1_EMU_USP_L = (uint32_t)(uintptr_t)_ustack_lo;
        FM1_EMU_USP_H = (uint32_t)(uintptr_t)_sstack_top;
        FM1_EMU_CON |= 1u << 3;
    }
    if (which & FM1_GUARD_WRITE) {
        FM1_WR_LIMIT_L(0) = (uint32_t)(uintptr_t)_guard0;
        FM1_WR_LIMIT_H(0) = (uint32_t)(uintptr_t)_ustack_lo - 1u;
        FM1_WR_LIMIT_L(1) = (uint32_t)(uintptr_t)_guard1;
        FM1_WR_LIMIT_H(1) = (uint32_t)(uintptr_t)_sstack_lo - 1u;
        FM1_C0_WR_LIMIT_EN |= 3u;
    }
    if (which & FM1_GUARD_BUS)
        FM1_DBG_EN |= FM1_GUARD_BUS_BITS;
    if (which & FM1_GUARD_PC) {
        FM1_PC_LIMIT0_L = 0x02000120u;
        FM1_PC_LIMIT0_H = fm1_guard_xip_hi ? fm1_guard_xip_hi : (uint32_t)(uintptr_t)_etext;
        {   /* RAM code (.ram_text); with none, repeat the XIP window */
            volatile uint32_t rs = (uint32_t)(uintptr_t)_rt_start, re = (uint32_t)(uintptr_t)_rt_end;
            if (fm1_guard_ram_hi > re)
                re = fm1_guard_ram_hi;          /* a Bluetooth controller's RAM code follows */
            FM1_PC_LIMIT1_L = re > rs ? rs : 0x02000120u;
            FM1_PC_LIMIT1_H = re > rs ? re - 1u : (uint32_t)(uintptr_t)_etext;
        }
    }
    fm1__dbg_lock();
}

/* the PC limits apply to the second core too, and it starts in the chip's ROM (found by Melodee's
 * dual-core work: DBG bit 10, c1_pc_limit_err_r): open them while it starts, then
 * fm1_guard_enable(FM1_GUARD_PC) again */
static void fm1_guard_pc_open(void)
{
    fm1__dbg_unlock();
    FM1_PC_LIMIT0_L = 0u;
    FM1_PC_LIMIT0_H = 0xFFFFFFFFu;
    FM1_PC_LIMIT1_L = 0u;
    FM1_PC_LIMIT1_H = 0xFFFFFFFFu;
    fm1__dbg_lock();
}

/* write-protect 0x01C7FD50..0x01C7FFFF (boot info, mailbox, vectors):
 * call after the last fm1_irq_attach */
static void fm1_guard_lock_top(void)
{
    fm1__dbg_unlock();
    FM1_DBG_MSG_CLR = 0xFFFFFFFFu;
    FM1_WR_LIMIT_L(2) = 0x01C7FD50u;
    FM1_WR_LIMIT_H(2) = 0x01C7FFFFu;
    FM1_C0_WR_LIMIT_EN |= 4u;
    fm1__dbg_lock();
}

/* core 0's stacks (_ustack_lo.._sstack_top) closed to writes from core 1 and from the
 * peripherals (DMA): write limit 3. A write raises DBG_MSG bit 11 (core 1) or bit 2 (a
 * peripheral; PRP_WR_LIMIT_ID names it). Core 0 is not limited by it (C0 enable bit 3 stays 0).
 * For the Bluetooth start (src/ble.c): a return address on core 0's interrupt stack lost bits
 * while core 1 ran the controller. */
#define FM1_C1_WR_LIMIT_EN FM1_X2(0x34Cu)
#define FM1_PRP_WR_LIMIT_EN FM1_X2(0x350u)
static void fm1_guard_watch_c0_stacks(void)
{
    fm1__dbg_unlock();
    FM1_DBG_MSG_CLR = 0xFFFFFFFFu;
    FM1_WR_LIMIT_L(3) = (uint32_t)(uintptr_t)_ustack_lo;
    FM1_WR_LIMIT_H(3) = (uint32_t)(uintptr_t)_sstack_top - 1u;
    FM1_C1_WR_LIMIT_EN |= 8u;
    FM1_PRP_WR_LIMIT_EN |= 8u;
    fm1__dbg_lock();
}

/* ---- timer 5 frame watch (the Bluetooth start, test 22) ----
 * isr_timer5 (fm1_isr.S) puts its sp after the pushes in fm1_t5_sp: sp[0..3] r3-r0, sp[4..6]
 * psr, rets, reti, sp[7] the word above. fm1_t5w_begin copies the 8 words; fm1_t5w_check compares
 * them, records the first change (time, checkpoint, word, old, new, error state) and writes the
 * old word back, so core 0 lives on and DIAG can count the changes. fm1_t5w_end keeps the error
 * state of each tick. fm1_t5w is in .ble_ram (not cleared): a watchdog reset keeps it. */
#define FM1_C0_WR_LIMIT_ERR_NUM FM1_X2(0x360u)
#define FM1_C1_WR_LIMIT_ERR_NUM FM1_X2(0x364u)
#define FM1_PRP_WR_LIMIT_ERR_NUM FM1_X2(0x368u)
#define FM1_PRP_WR_LIMIT_ID FM1_X2(0x36Cu)
#define FM1_T5W_MAGIC 0x54355731u           /* "T5W1" */
typedef struct {
    uint32_t magic, ms, ticks, tag;         /* the last tick: time, count, last checkpoint reached */
    uint32_t dbg, c0n, c1n, prpn, prpid;    /* DBG_MSG, write limit errors (core 0, core 1, DMA), DMA id */
    uint32_t hits, h_ms, h_tag, h_slot, h_old, h_new, h_sp, h_dbg, h_prpid, h_c1n, h_prpn;
    uint32_t icfg0_3, icfg0_5;              /* core 0's ICFG words for IRQ 24-31 and 40-47 (Bluetooth: 29, 31, 40-45) */
} fm1_t5w_t;
uint32_t fm1_t5_sp;
static fm1_t5w_t fm1_t5w __attribute__((section(".ble_ram")));
static uint32_t fm1_t5_snap[8], fm1_t5_snap_sp;
/* 1 (src/ble.c, after the Bluetooth start): write limit 4 closes the 8 frame words to every
 * writer (core 0, core 1, DMA) while the C body runs. A write raises the CPU exception, and the
 * crash record shows the writer (DBG_MSG bit 13 core 0, 11 core 1, 2 a DMA). */
static uint8_t fm1_t5w_arm, fm1_t5w_armed;
static void fm1_t5w_close(void)
{
    if (!fm1_t5w_armed)
        return;
    fm1__dbg_unlock();
    FM1_C0_WR_LIMIT_EN &= 0xFFFFFFEFu;
    FM1_C1_WR_LIMIT_EN &= 0xFFFFFFEFu;
    FM1_PRP_WR_LIMIT_EN &= 0xFFFFFFEFu;
    fm1__dbg_lock();
    fm1_t5w_armed = 0;
}
static void fm1_t5w_begin(void)
{
    const uint32_t *f = (const uint32_t *)(uintptr_t)fm1_t5_sp;
    uint32_t i;
    fm1_t5_snap_sp = fm1_t5_sp;
    for (i = 0; i < 8u; i++)
        fm1_t5_snap[i] = f[i];
    fm1_t5w.tag = 0;
    if (fm1_t5w_arm) {
        fm1__dbg_unlock();
        FM1_WR_LIMIT_L(4) = fm1_t5_sp;
        FM1_WR_LIMIT_H(4) = fm1_t5_sp + 31u;
        FM1_C0_WR_LIMIT_EN |= 16u;
        FM1_C1_WR_LIMIT_EN |= 16u;
        FM1_PRP_WR_LIMIT_EN |= 16u;
        fm1__dbg_lock();
        fm1_t5w_armed = 1;
    }
}
static void fm1_t5w_check(uint32_t tag, uint32_t ms)
{
    uint32_t *f = (uint32_t *)(uintptr_t)fm1_t5_snap_sp;
    uint32_t i, v;
    fm1_t5w.tag = tag;
    for (i = 0; i < 8u; i++) {
        v = f[i];
        if (v == fm1_t5_snap[i])
            continue;
        fm1_t5w_close();                    /* the repair below is a write too */
        if (!fm1_t5w.hits) {
            fm1_t5w.h_ms = ms;
            fm1_t5w.h_tag = tag;
            fm1_t5w.h_slot = i;
            fm1_t5w.h_old = fm1_t5_snap[i];
            fm1_t5w.h_new = v;
            fm1_t5w.h_sp = fm1_t5_snap_sp;
            fm1_t5w.h_dbg = FM1_DBG_MSG;
            fm1_t5w.h_prpid = FM1_PRP_WR_LIMIT_ID;
            fm1_t5w.h_c1n = FM1_C1_WR_LIMIT_ERR_NUM;
            fm1_t5w.h_prpn = FM1_PRP_WR_LIMIT_ERR_NUM;
        }
        fm1_t5w.hits++;
        f[i] = fm1_t5_snap[i];
    }
}
static void fm1_t5w_end(uint32_t ms)
{
    fm1_t5w_close();                        /* before the pops and the next entry's pushes */
    fm1_t5w.icfg0_3 = FM1_ICFG(24);
    fm1_t5w.icfg0_5 = FM1_ICFG(40);
    fm1_t5w.dbg = FM1_DBG_MSG;
    fm1_t5w.c0n = FM1_C0_WR_LIMIT_ERR_NUM;
    fm1_t5w.c1n = FM1_C1_WR_LIMIT_ERR_NUM;
    fm1_t5w.prpn = FM1_PRP_WR_LIMIT_ERR_NUM;
    fm1_t5w.prpid = FM1_PRP_WR_LIMIT_ID;
    fm1_t5w.ms = ms;
    fm1_t5w.ticks++;
    fm1_t5w.magic = FM1_T5W_MAGIC;
}
/* at power-on, before the timer starts: a new record (the old one stays for DIAG in fm1_t5w_prev) */
static fm1_t5w_t fm1_t5w_prev __attribute__((section(".ble_ram")));
static void fm1_t5w_boot(void)
{
    fm1_t5w_prev = fm1_t5w;
    fm1_t5w.magic = 0;
    fm1_t5w.ticks = 0;
    fm1_t5w.hits = 0;
}

/* the reserved top of RAM is write-protected now (fm1_guard_lock_top) */
static int fm1_guard_top_locked(void) { return (FM1_C0_WR_LIMIT_EN & 4u) != 0; }

/* the bus-invalid checks (FM1_GUARD_BUS) off: a Bluetooth controller (src/ble.c) runs JieLi's RF code,
 * which the checks were not made for. The value to give back to fm1_guard_bus_resume. */
static uint32_t fm1_guard_bus_pause(void)
{
    uint32_t v;
    fm1__dbg_unlock();
    v = FM1_DBG_EN;
    FM1_DBG_EN = v ^ (v & FM1_GUARD_BUS_BITS);     /* (no ~mask literal: build.py's ROM-address check) */
    fm1__dbg_lock();
    return v & FM1_GUARD_BUS_BITS;
}
/* the EMU stack limit off: a Bluetooth controller's init runs on core 0 on the image's own stack
 * (src/ble.c). The value to give back to fm1_guard_stack_resume. */
static uint32_t fm1_guard_stack_pause(void)
{
    uint32_t v;
    fm1__dbg_unlock();
    v = FM1_EMU_CON & (1u << 3);
    FM1_EMU_CON = FM1_EMU_CON ^ v;
    fm1__dbg_lock();
    return v;
}
static void fm1_guard_stack_resume(uint32_t on)
{
    if (on)
        fm1_guard_enable(FM1_GUARD_STACK);
}
static void fm1_guard_bus_resume(uint32_t bits)
{
    fm1__dbg_unlock();
    FM1_DBG_MSG_CLR = 0xFFFFFFFFu;
    FM1_DBG_EN |= bits;
    fm1__dbg_lock();
}

/* reopen the reserved top of RAM (mailbox write before a UBOOT reset) */
static void fm1_guard_unlock_top(void)
{
    fm1__dbg_unlock();
    FM1_C0_WR_LIMIT_EN &= ~4u;
    fm1__dbg_lock();
}
