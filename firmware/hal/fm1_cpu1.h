/* SPDX-License-Identifier: GPL-3.0-only */
/* The AC79's second core (docs/plans/2026-10-04-dual-core-and-clock.md), with what Melodee's
 * dual-core audio (github.com/keremimo/melodee, GPL-3.0) found on the device.
 *
 * Start (the AC79 SDK's EnableOtherCpu): core 1's interrupt bank off (0x1EEF300..), its entry
 * address into the word at 0x01C7FFF8, bit 3 of 0x10008 held while it starts, C1_CON (0x1EEE004)
 * bit 3 set (enabled) and bit 1 cleared (out of hold). It boots through the chip's ROM, which the
 * PC limits refuse, so they are open until it has reported in from fm1_c1_main. Hold is bit 1
 * set, then bit 3 cleared.
 *
 * Work: one job at a time, a function and an argument; core 0 hands it over (mb.job) and waits
 * for it (mb.done), single writer each way, csync around each (the cores share the caches).
 *
 * Units that cannot run it (fm1-x0x#10, measured by Jangada, github.com/zednaked/jangada): on some
 * FM-1s core 1 reads words that are not in RAM (0x00200000 where core 0 reads 0), idle now and then,
 * often under load. So core 1 runs only the next job, and only when its check word (the function,
 * the argument and the job's number, written before the job) matches; the jobs it finished stay in a
 * register (Melodee's, keremimo/melodee#21: a stale function read as the next job fails the check).
 * Anything else it counts (mb.bad) and leaves, and core 0 holds it at the first one (fm1_cpu1_wait):
 * one core from then on. The mailbox keeps each core's writes on cache lines of their own
 * (Jangada's layout).
 *
 * Flash: the waiting loop is in RAM (.c1_text), so an idle core 1 never fetches from the flash;
 * it works only for the audio interrupt, which waits for it before it returns, and core 0 turns
 * the flash off only with interrupts off. So the flash is never off under it.
 *
 * Bluetooth (SMK-37, src/ble.c): a separate controller image can share core 1. Its interrupts run
 * on core 1 (also inside an audio job) and the idle loop calls its poll entry (mb.poll) between
 * jobs. Both run from the flash, so core 0 parks core 1 before it turns the flash off
 * (fm1_cpu1_park): core 1 then waits in RAM with its interrupts off and still takes audio jobs.
 * Without a controller (mb.poll 0), nothing here changes: interrupts stay off on core 1. */
#pragma once
#include <stdint.h>

#define FM1_C1_CON (*(volatile uint32_t *)0x1EEE004u)
#define FM1_C1_CLK (*(volatile uint32_t *)0x00010008u)
#define FM1_C1_T4 (*(volatile uint32_t *)0x10804u)       /* TIMER4, 24 MHz */
#define FM1_C1_SYNC() __asm__ volatile("csync" ::: "memory")
#define FM1_C1_ICFG(i) (*(volatile uint32_t *)(0x1EEF300u + 4u * (i)))   /* core 1's interrupt bank */
#define FM1_C1_PARK_MS 20u                      /* longest wait for core 1 to park */

extern void fm1_c1_entry(void);
extern uint32_t _c1_ustack[], _c1_sstack_top[];
#define FM1_C1_STACK_WORDS (2816u / 4u)
#define FM1_C1_MARK 0x43314D4Bu                  /* "C1MK": how deep core 1's stack has gone */
typedef struct {
    volatile uint32_t job, arg;                 /* core 0: job + 1 hands fn(arg) over */
    void (*volatile fn)(uint32_t);
    volatile uint32_t check;                    /* FM1_C1_CHECK of fn, arg and job */
    void (*volatile poll)(void);                /* core 0: the Bluetooth controller's idle entry (0: none) */
    volatile uint32_t park;                     /* core 0: a request number = park (the flash goes off), 0 = run */
    uint32_t pad0[10];
    volatile uint32_t alive, done;              /* core 1: started; the last job it finished */
    volatile uint32_t bad, bad_job, bad_done, bad_fn;   /* jobs it would not run: how many, the last one */
    volatile uint32_t parked;                   /* core 1: the request number it parked for (in RAM, interrupts off) */
    volatile uint32_t poll_max;                 /* core 1: the longest poll call, 24 MHz ticks */
    uint32_t pad1[8];
} fm1_c1_mb_t;
static fm1_c1_mb_t fm1_c1_mb __attribute__((aligned(64)));
volatile uint32_t fm1_c1_trace;                 /* breadcrumbs (fm1_cpu1.S, fm1_c1_main) */
static uint8_t fm1_c1_on;                       /* started and answering: core 0 may hand it work */
#define FM1_C1_CHECK(fn, arg, job) ((uint32_t)(uintptr_t)(fn) ^ (arg) ^ (job) * 0x9E3779B1u)
static uint8_t fm1_c1_gave_up;                  /* 1 = it read a wrong job, 2 = it did not finish one,
                                                 * 3 = it did not park, 4 = the Bluetooth start did not end */
/* core 1's interrupt stack (fm1_cpu1.S) and its stack limit window: the app's own by default, the
 * Bluetooth controller's when one runs (fm1_cpu1_ble_stacks, before fm1_cpu1_start) */
uint32_t fm1_c1_ssp = (uint32_t)(uintptr_t)_c1_sstack_top;
static uint32_t fm1_c1_lim_lo, fm1_c1_lim_hi;
static uint32_t fm1_c1_park_seq;                /* core 0: the last park request's number */

void fm1_c1_main(void);
void __attribute__((section(".c1_text"), noreturn, used)) fm1_c1_main(void)
{
    /* its own guards, as Melodee sets them (its fm1_core1_main): a stack limit in core 1's EMU bank
     * (core 0's is at 0x1EEF0D0) over both its stacks less the lowest 256 bytes, and EMU_CON bit 2
     * and bits 16..20 off, bit 3 (the stack limit) on. With a Bluetooth controller the window
     * reaches down over the controller's RAM too (its interrupt, poll and task stacks). */
    uint32_t lo = fm1_c1_lim_lo ? fm1_c1_lim_lo : (uint32_t)(uintptr_t)_c1_ustack + 256u;
    uint32_t hi = fm1_c1_lim_hi ? fm1_c1_lim_hi : (uint32_t)(uintptr_t)_c1_sstack_top - 1u;
    uint32_t done, last_bad = 0xFFFFFFFFu, parked = 0;
    *(volatile uint32_t *)0x1EEF2D8u = hi;
    *(volatile uint32_t *)0x1EEF2DCu = lo;
    *(volatile uint32_t *)0x1EEF2E0u = hi;
    *(volatile uint32_t *)0x1EEF2E4u = lo;
    *(volatile uint32_t *)0x1EEF2D0u = (*(volatile uint32_t *)0x1EEF2D0u & ~((1u << 2) | (0x1Fu << 16))) | (1u << 3);
    fm1_c1_trace = 0xC1000002;
    done = fm1_c1_mb.done;
    fm1_c1_mb.alive = 1;
    FM1_C1_SYNC();
    for (;;) {
        uint32_t j = fm1_c1_mb.job;
        FM1_C1_SYNC();
        if (j == done) {
            void (*poll)(void) = fm1_c1_mb.poll;
            uint32_t t0, dt, pk;
            if (!poll)
                continue;                       /* no Bluetooth controller: as before */
            pk = fm1_c1_mb.park;
            if (pk) {                           /* the flash goes off: wait here, in RAM */
                if (!parked) {
                    __asm__ volatile("cli" ::: "memory");
                    parked = 1;
                }
                if (fm1_c1_mb.parked != pk) {   /* echo this request's number: core 0 waits for it */
                    FM1_C1_SYNC();
                    fm1_c1_mb.parked = pk;
                    FM1_C1_SYNC();
                }
                continue;
            }
            if (parked) {
                parked = 0;
                fm1_c1_mb.parked = 0;
                FM1_C1_SYNC();
            }
            __asm__ volatile("csync\n\tsti" ::: "memory");   /* the controller's interrupts */
            t0 = FM1_C1_T4;
            poll();                             /* the controller, from the flash */
            dt = FM1_C1_T4 - t0;
            if (dt > fm1_c1_mb.poll_max)
                fm1_c1_mb.poll_max = dt;
            continue;
        }
        if (j == done + 1u) {
            void (*fn)(uint32_t) = fm1_c1_mb.fn;
            uint32_t arg = fm1_c1_mb.arg;
            if (fn && fm1_c1_mb.check == FM1_C1_CHECK(fn, arg, j)) {
                fn(arg);                        /* the work itself runs from the flash */
                done = j;
                FM1_C1_SYNC();
                fm1_c1_mb.done = j;
                FM1_C1_SYNC();
                continue;
            }
            fm1_c1_mb.bad_fn = (uint32_t)(uintptr_t)fn;
        }
        if (j != last_bad) {                    /* each wrong value once (it stays until core 0 holds it) */
            last_bad = j;
            fm1_c1_mb.bad_job = j;
            fm1_c1_mb.bad_done = done;
            FM1_C1_SYNC();
            fm1_c1_mb.bad++;
            FM1_C1_SYNC();
        }
    }
}

/* every interrupt off on core 1 (its bank), from core 0 */
static inline void fm1_cpu1_irqs_off(void)
{
    uint32_t i;
    for (i = 0; i < 32u; i++)
        FM1_C1_ICFG(i) = 0;
    FM1_C1_SYNC();
}

/* hold core 1, registers only: also first thing at boot, before the RAM it may still be reading is
 * cleared (after a reset that did not hold it) */
static inline void fm1_cpu1_halt(void)
{
    FM1_C1_CON |= 0x2u;
    FM1_C1_CON &= ~0x8u;
    FM1_C1_SYNC();
}

static void fm1_cpu1_hold(void)
{
    fm1_cpu1_halt();
    fm1_c1_on = 0;
    fm1_c1_mb.poll = 0;
    fm1_c1_mb.park = 0;
}

/* before fm1_cpu1_start: core 1's interrupt stack top and the low end of its stack limit window
 * (a Bluetooth controller's RAM) */
static void fm1_cpu1_ble_stacks(uint32_t ssp_top, uint32_t lim_lo)
{
    fm1_c1_ssp = ssp_top;
    fm1_c1_lim_lo = lim_lo;
    fm1_c1_lim_hi = (uint32_t)(uintptr_t)_c1_sstack_top - 1u;
}

/* 0 = core 1 runs and answered; else it is held again. The top of RAM must be writable
 * (fm1_guard_unlock_top) and the PC limits open (fm1_guard_pc_open): the caller's */
static int fm1_cpu1_start(void)
{
    uint32_t saved, t0, i;
    if (fm1_c1_on)
        return 0;
    fm1_cpu1_hold();
    for (i = 0; i < FM1_C1_STACK_WORDS; i++)
        _c1_ustack[i] = FM1_C1_MARK;
    fm1_c1_mb.alive = 0;
    fm1_c1_trace = 0;
    fm1_c1_mb.done = fm1_c1_mb.job;
    fm1_c1_mb.fn = 0;
    fm1_c1_mb.parked = 0;
    fm1_c1_mb.poll_max = 0;
    FM1_C1_SYNC();
    fm1_cpu1_irqs_off();                        /* its interrupt bank: all off (core 0's is at 0x1EEF100) */
    *(volatile uint32_t *)0x01C7FFF8u = (uint32_t)(uintptr_t)&fm1_c1_entry;
    saved = FM1_C1_CLK;
    FM1_C1_CLK = saved | 0x8u;
    FM1_C1_SYNC();
    FM1_C1_CON |= 0x8u;
    FM1_C1_CON &= ~0x2u;
    t0 = FM1_C1_T4;
    while (!fm1_c1_mb.alive && FM1_C1_T4 - t0 < 24000u * 20u)   /* 20 ms */
        FM1_C1_SYNC();
    FM1_C1_CLK = saved;
    if (!fm1_c1_mb.alive) {
        fm1_cpu1_hold();
        return -1;
    }
    fm1_c1_on = 1;
    return 0;
}

/* hand core 1 a job: 1 = it took it (fm1_cpu1_wait before touching what it works on) */
static inline int fm1_cpu1_run(void (*fn)(uint32_t), uint32_t arg)
{
    uint32_t job;
    if (!fm1_c1_on)
        return 0;
    job = fm1_c1_mb.job + 1u;
    fm1_c1_mb.fn = fn;
    fm1_c1_mb.arg = arg;
    fm1_c1_mb.check = FM1_C1_CHECK(fn, arg, job);
    FM1_C1_SYNC();
    fm1_c1_mb.job = job;
    FM1_C1_SYNC();
    return 1;
}

/* 0 = done; -1 = it read a wrong job or did not finish in time: core 1 is held from now on (the
 * caller does the work) */
static inline int fm1_cpu1_wait(void)
{
    uint32_t t0 = FM1_C1_T4;
    for (;;) {
        FM1_C1_SYNC();
        if (fm1_c1_mb.bad) {                    /* a unit that cannot run it: one core */
            fm1_cpu1_hold();
            fm1_c1_gave_up = 1;
            return -1;
        }
        if (fm1_c1_mb.done == fm1_c1_mb.job)
            return 0;
        if (FM1_C1_T4 - t0 > 24000u * 8u) {     /* 8 ms: far past any stretch (or it faulted) */
            fm1_cpu1_hold();
            fm1_c1_gave_up = 2;
            return -1;
        }
    }
}

/* One job with core 0 in RAM until it ends (the Bluetooth controller's start: its RF trim turns
 * the flash off). Core 0's interrupts off; call through a pointer (FL_FAR). No calls in here (RAM
 * code: tools/build.py checks). 0 = done, -1 = not in time, -2 = a wrong job. */
#define FM1_C1_RAMFN __attribute__((section(".ram_text"), noinline, used))
static FM1_C1_RAMFN int fm1_cpu1_run_wait_ram(void (*fn)(uint32_t), uint32_t arg, uint32_t ticks)
{
    uint32_t job = fm1_c1_mb.job + 1u, t0;
    fm1_c1_mb.fn = fn;
    fm1_c1_mb.arg = arg;
    fm1_c1_mb.check = FM1_C1_CHECK(fn, arg, job);
    FM1_C1_SYNC();
    fm1_c1_mb.job = job;
    FM1_C1_SYNC();
    t0 = FM1_C1_T4;
    for (;;) {
        FM1_C1_SYNC();
        if (fm1_c1_mb.done == job)
            return 0;
        if (fm1_c1_mb.bad)
            return -2;
        if (FM1_C1_T4 - t0 > ticks)
            return -1;
    }
}

/* Park core 1 before the flash goes off (main loop only, core 0's interrupts still on): 1 = parked,
 * 0 = nothing to park (no controller), -1 = it did not park in time: core 1 is held from now on
 * (one core, no Bluetooth). Undo with fm1_cpu1_unpark. */
static int fm1_cpu1_park(void)
{
    uint32_t t0, seq;
    if (!fm1_c1_on || !fm1_c1_mb.poll)
        return 0;
    seq = ++fm1_c1_park_seq;                    /* a new number each time: a stale "parked" never counts */
    if (!seq)
        seq = fm1_c1_park_seq = 1u;
    fm1_c1_mb.park = seq;
    FM1_C1_SYNC();
    t0 = FM1_C1_T4;
    while (fm1_c1_mb.parked != seq) {
        FM1_C1_SYNC();
        if (FM1_C1_T4 - t0 > 24000u * FM1_C1_PARK_MS) {
            fm1_cpu1_hold();
            fm1_c1_gave_up = 3;
            return -1;
        }
    }
    return 1;
}
static void fm1_cpu1_unpark(void)
{
    if (fm1_c1_mb.park) {
        fm1_c1_mb.park = 0;
        FM1_C1_SYNC();
    }
}
#define FM1_CPU1_PARK 1                         /* fm1_flash.h: park around each flash operation */
