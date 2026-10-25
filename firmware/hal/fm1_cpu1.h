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
/* Words the BLE guard still copies after a failed start. Nothing in this firmware writes them.
 * They survive a reset, so a previous image can leave a value: 0xC1000000 | TIMER4 time in 100 ms
 * (bits 16-23) | stalls << 8 | (rc & 0xFF), 0xEE = still waiting. */
static volatile uint32_t fm1_c1_wait_end __attribute__((section(".noinit")));
/* TIMER4 ms of that old wait, at its last look. Survives a reset. */
static volatile uint32_t fm1_c1_wait_ms __attribute__((section(".noinit")));
/* Optional checks nothing calls. Results stay in .noinit for the guard:
 * - fm1_c1_watch: 4 words copied to fm1_c1_wait_seen
 * - fm1_c1_sum_lo..hi: words added up; fm1_c1_sum_ref the sum before a start.
 *   fm1_c1_sum_bad = ms of the first different sum | 0xBAD00000, fm1_c1_sum_got that sum. */
static const volatile uint32_t *fm1_c1_watch;
static const volatile uint32_t *fm1_c1_sum_lo, *fm1_c1_sum_hi;
static uint32_t fm1_c1_sum_ref;
static volatile uint32_t fm1_c1_wait_seen[4] __attribute__((section(".noinit")));
static volatile uint32_t fm1_c1_sum_bad, fm1_c1_sum_got __attribute__((section(".noinit")));
/* the sum of fm1_c1_sum_lo..hi (inline: the RAM code makes no calls) */
static inline __attribute__((always_inline)) uint32_t fm1_c1_sum(void);
/* watch (4 words) and the words lo..hi. Nothing calls this. */
__attribute__((unused)) static void fm1_cpu1_wait_checks(const void *watch, const void *lo, const void *hi)
{
    fm1_c1_sum_lo = (const volatile uint32_t *)lo;
    fm1_c1_sum_hi = (const volatile uint32_t *)hi;
    fm1_c1_sum_ref = fm1_c1_sum();
    fm1_c1_watch = (const volatile uint32_t *)watch;
}
static inline __attribute__((always_inline)) uint32_t fm1_c1_sum(void)
{
    const volatile uint32_t *p = fm1_c1_sum_lo;
    uint32_t s = 0;
    while (p < fm1_c1_sum_hi)
        s = (s << 1 | s >> 31) ^ *p++;
    return s;
}
static uint8_t fm1_c1_on;                       /* started and answering: core 0 may hand it work */
#define FM1_C1_CHECK(fn, arg, job) ((uint32_t)(uintptr_t)(fn) ^ (arg) ^ (job) * 0x9E3779B1u)
static uint8_t fm1_c1_gave_up;                  /* 1 = it read a wrong job, 2 = it did not finish one,
                                                 * 3 = it did not park, 4 = the Bluetooth start did not end */
/* core 1's interrupt stack (fm1_cpu1.S) and its stack limit window: the app's own by default, the
 * Bluetooth controller's when one runs (fm1_cpu1_ble_stacks, before fm1_cpu1_start) */
uint32_t fm1_c1_ssp = (uint32_t)(uintptr_t)_c1_sstack_top;
static uint32_t fm1_c1_lim_lo, fm1_c1_lim_hi;
static uint32_t fm1_c1_park_seq;                /* core 0: the last park request's number */

/* Core 0 seen from core 1 (the Bluetooth start, test 24). With a controller, core 1 watches core 0's
 * ms counter (fm1_c1_c0_clock, set by src/ble.c). When it has not moved for 100 ms, core 1 copies
 * core 0's ETM (its last 4 branches), DBG_MSG and EMU_MSG, 4 times 10 ms apart, into fm1_c0_dead
 * (.ble_ram: a watchdog reset keeps it). Then it stops looking. */
#define FM1_C0_DEAD_MAGIC 0xC0DEAD01u
typedef struct {
    uint32_t magic, n, t4_ms, c0_ms, dbg, emu;  /* n: samples taken */
    uint32_t etm[4][4];                         /* 4 samples of core 0's ETM_PC0..3 */
} fm1_c0_dead_t;
static fm1_c0_dead_t fm1_c0_dead __attribute__((section(".ble_ram")));
static const volatile uint32_t *volatile fm1_c1_c0_clock;
/* RAM canary (test 26): the 8 KiB under core 0's stack guard (0x01C72000..0x01C74000, the pool's
 * spare: tools/build.py keeps >= 8 KiB of the pool free) is set to all ones before the controller
 * starts. Core 1 looks at 16 words per loop; a word that changed is recorded (the first 8: address,
 * value, TIMER4 ms) and set back. Shows whether RAM loses bits with nothing writing it. */
#define FM1_CAN_LO 0x01C72000u
#define FM1_CAN_HI 0x01C74000u
typedef struct {
    uint32_t armed, n, scans, rec[8][3];
} fm1_can_t;
static fm1_can_t fm1_can __attribute__((section(".ble_ram")));
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
    uint32_t c0_last = 0, c0_t = 0, c0_next = 0;
    uint32_t can_i = 0;
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
            if (fm1_can.armed == 0xCA9A11E5u) {          /* the RAM canary (inline: no calls) */
                volatile uint32_t *cw = (volatile uint32_t *)(uintptr_t)FM1_CAN_LO;
                uint32_t e = can_i + 16u;
                for (; can_i < e; can_i++) {
                    uint32_t v = cw[can_i];
                    if (v != 0xFFFFFFFFu) {
                        uint32_t k = fm1_can.n;
                        if (k < 8u) {
                            fm1_can.rec[k][0] = FM1_CAN_LO + 4u * can_i;
                            fm1_can.rec[k][1] = v;
                            fm1_can.rec[k][2] = FM1_C1_T4 / 24000u;
                        }
                        fm1_can.n = k + 1u;
                        cw[can_i] = 0xFFFFFFFFu;
                    }
                }
                if (can_i >= (FM1_CAN_HI - FM1_CAN_LO) / 4u) {
                    can_i = 0;
                    fm1_can.scans++;
                }
            }
            if (fm1_c1_c0_clock && fm1_c0_dead.n < 4u) {   /* core 0 alive? (inline: no calls) */
                uint32_t m = *fm1_c1_c0_clock, t = FM1_C1_T4;
                if (m != c0_last || !c0_t) {
                    c0_last = m;
                    c0_t = t;
                } else if (t - c0_t > 2400000u && (int32_t)(t - c0_next) >= 0) {
                    uint32_t k = fm1_c0_dead.n;
                    if (!k) {
                        fm1_c0_dead.t4_ms = t / 24000u;
                        fm1_c0_dead.c0_ms = m;
                        fm1_c0_dead.dbg = *(volatile uint32_t *)0x1EEE244u;
                        fm1_c0_dead.emu = *(volatile uint32_t *)0x1EEF0D4u;
                        fm1_c0_dead.magic = FM1_C0_DEAD_MAGIC;
                    }
                    fm1_c0_dead.etm[k][0] = *(volatile uint32_t *)0x1EEF1C4u;
                    fm1_c0_dead.etm[k][1] = *(volatile uint32_t *)0x1EEF1C8u;
                    fm1_c0_dead.etm[k][2] = *(volatile uint32_t *)0x1EEF1CCu;
                    fm1_c0_dead.etm[k][3] = *(volatile uint32_t *)0x1EEF1D0u;
                    fm1_c0_dead.n = k + 1u;
                    c0_next = t + 240000u;
                    FM1_C1_SYNC();
                }
            }
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
