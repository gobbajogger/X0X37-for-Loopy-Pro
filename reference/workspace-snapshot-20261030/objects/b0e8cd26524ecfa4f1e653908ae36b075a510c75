// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Millisecond clock and a small software timer table (sys_timer_* and sys_timeout_*).
 * The clock reads the app's free-running counter (boot parameters tick_addr, ticks_per_ms), so
 * the image needs no timer interrupt. Callbacks run from poll(), not from an interrupt. The
 * "_to_task" variants run in the same context (the SDK runs them in the named task); the
 * controller only uses short callbacks. */
#include "ctl_port.h"

#define CTL_MAX_TIMERS 16

struct ctl_timer {
    void (*fn)(void *);
    void *priv;
    u32 period;      /* 0 = one-shot (timeout) */
    u32 due;
    u16 id;          /* 0 = free slot */
};

static struct ctl_timer timers[CTL_MAX_TIMERS];
static u32 ms_now, tick_last, tick_per_ms = 24000u;
static volatile const u32 *tick;
static u16 next_id = 1;

void ctl_ms_init(void)
{
    tick = (volatile const u32 *)(uintptr_t)ctl_bp->tick_addr;
    tick_per_ms = ctl_bp->ticks_per_ms ? ctl_bp->ticks_per_ms : 24000u;
    tick_last = tick ? *tick : 0;
    ms_now = 0;
}

/* whole milliseconds since ctl_ms_init; must run at least once per counter wrap (179 s at 24 MHz):
 * poll() calls it each pass */
u32 ctl_ms(void)
{
    u32 f = ctl_port_irq_save(), d;
    if (tick) {
        d = *tick - tick_last;
        if (d >= tick_per_ms) {
            u32 k = d / tick_per_ms;
            ms_now += k;
            tick_last += k * tick_per_ms;
        }
    }
    d = ms_now;
    ctl_port_irq_restore(f);
    return d;
}
u32 sys_timer_get_ms(void) { return ctl_ms(); }

static u16 add(void *priv, void (*fn)(void *), u32 msec, int periodic)
{
    u32 now = ctl_ms(), f = ctl_port_irq_save();
    for (int i = 0; i < CTL_MAX_TIMERS; i++) {
        struct ctl_timer *t = &timers[i];
        if (t->id) continue;
        if (++next_id == 0) next_id = 1;
        t->fn = fn; t->priv = priv; t->period = periodic ? (msec ? msec : 1) : 0;
        t->due = now + msec; t->id = next_id;
        ctl_port_irq_restore(f);
        return t->id;
    }
    ctl_port_irq_restore(f);
    return 0;
}

static struct ctl_timer *by_id(u16 id)
{
    for (int i = 0; i < CTL_MAX_TIMERS; i++) if (id && timers[i].id == id) return &timers[i];
    return NULL;
}

u16 sys_timer_add(void *priv, void (*fn)(void *), u32 msec) { return add(priv, fn, msec, 1); }
u16 sys_timeout_add(void *priv, void (*fn)(void *), u32 msec) { return add(priv, fn, msec, 0); }
u16 sys_timer_add_to_task(const char *task, void *priv, void (*fn)(void *), u32 msec) { (void)task; return add(priv, fn, msec, 1); }
u16 sys_timeout_add_to_task(const char *task, void *priv, void (*fn)(void *), u32 msec) { (void)task; return add(priv, fn, msec, 0); }
void sys_timer_del(u16 id) { struct ctl_timer *t = by_id(id); if (t) t->id = 0; }
void sys_timeout_del(u16 id) { sys_timer_del(id); }
int sys_timer_modify(u16 id, u32 msec)
{
    struct ctl_timer *t = by_id(id);
    if (!t) return -1;
    if (t->period) t->period = msec ? msec : 1;
    t->due = ctl_ms() + msec;
    return 0;
}
void sys_timer_re_run(u16 id) { struct ctl_timer *t = by_id(id); if (t) t->due = ctl_ms() + t->period; }
void sys_timer_set_user_data(u16 id, void *priv) { struct ctl_timer *t = by_id(id); if (t) t->priv = priv; }
void *sys_timer_get_user_data(u16 id) { struct ctl_timer *t = by_id(id); return t ? t->priv : NULL; }

void ctl_timer_poll(void)
{
    u32 now = ctl_ms();
    for (int i = 0; i < CTL_MAX_TIMERS; i++) {
        struct ctl_timer *t = &timers[i];
        if (!t->id || (s32)(now - t->due) < 0) continue;
        void (*fn)(void *) = t->fn; void *priv = t->priv;
        if (t->period) t->due += t->period; else t->id = 0;
        fn(priv);
    }
}
