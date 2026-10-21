// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Cooperative task table, task queues, semaphores and mutexes for the controller image.
 * No preemption: a task runs until it pends. Interrupt handlers may post (queue, semaphore);
 * they never pend. The "idle context" is poll() on its own stack: it runs each runnable task
 * once and returns to the app's idle loop on core 1 (ctl_os_run_once). */
#include "ctl_port.h"


#ifndef CTL_VENDOR_HOST_TEST
#define CTL_VENDOR_HOST_TEST 0
#endif
#define CTL_MAX_TASKS (CTL_VENDOR_HOST_TEST ? 2 : 1)
#define CTL_TASKQ_WORDS 64

struct ctl_task {
    const char *name;
    void (*fn)(void *);
    void *arg;
    struct ctl_ctx ctx;
    u32 *stack;
    u32 wake_ms;          /* for os_time_dly / timed pend (zero is a valid deadline) */
    u8 used, timed;
    volatile u8 waiting;
    u16 qhead, qtail;     /* message ring (words); a message = [count, type, args...] */
    int q[CTL_TASKQ_WORDS];
};

static struct ctl_task tasks[CTL_MAX_TASKS];
static struct ctl_task *cur;           /* NULL = idle context */
static struct ctl_ctx idle_ctx;
#if CTL_VENDOR_HOST_TEST
static u32 bootstrap_deadline;
static u8 bootstrap_active, bootstrap_running;

/* Only ctl_init may enable this window, around the single btstack_init call.
 * The vendor waits for controller-ready before creating its host task. Running
 * the existing scheduler on the already-owned init stack avoids a third task. */
int ctl_os_bootstrap_begin(u32 timeout_ms)
{
    if (cur || bootstrap_active || !timeout_ms || timeout_ms > 5000u) return -1;
    bootstrap_deadline = ctl_ms() + timeout_ms;
    bootstrap_active = 1;
    return 0;
}
void ctl_os_bootstrap_end(void) { bootstrap_active = 0; }

static int bootstrap_pump(void)
{
    if (!bootstrap_active || bootstrap_running) { ctl_fail(XB_E_ASSERT); return -1; }
    if ((s32)(ctl_ms() - bootstrap_deadline) >= 0) { ctl_fail(XB_E_TASK); return -1; }
    bootstrap_running = 1;
    ctl_os_run_once();
    bootstrap_running = 0;
    ctl_port_wdt_feed();
    return 0;
}

static void bootstrap_delay(u32 end)
{
    while ((s32)(ctl_ms() - end) < 0)
        if (bootstrap_pump()) return;
}
#endif


/* Stack size by task name. The SDK uses a name table; this image has
 * only the controller task, so one default is enough. */
#define CTL_TASK_STACK_WORDS 512

static struct ctl_task *find(const char *name)
{
    if (!name) return cur;
#if CTL_VENDOR_HOST_TEST
    for (int i = 0; i < CTL_MAX_TASKS; ++i)
        if (tasks[i].used && !strcmp(tasks[i].name, name)) return &tasks[i];
    return NULL;
#else
    /* The ABI-2 controller image has only one task. */
    return tasks[0].used ? &tasks[0] : NULL;
#endif
}

static void task_trampoline(void)
{
    cur->fn(cur->arg);
    cur->used = 0;                     /* task returned: free its slot */
    ctl_port_ctx_switch(&cur->ctx, &idle_ctx);
}

int task_create(void (*fn)(void *p), void *p, const char *name)
{
#if CTL_VENDOR_HOST_TEST
    if (!fn || !name || find(name)) return -1;
#endif
    for (int i = 0; i < CTL_MAX_TASKS; i++) {
        struct ctl_task *t = &tasks[i];
        if (t->used) continue;
        memset(t, 0, sizeof(*t));
        t->stack = malloc(CTL_TASK_STACK_WORDS * 4);
        if (!t->stack) return -1;
        t->name = name; t->fn = fn; t->arg = p; t->used = 1; t->waiting = 0;
        ctl_port_ctx_init(&t->ctx, t->stack, CTL_TASK_STACK_WORDS * 4, task_trampoline);
        return 0;
    }
    return -1;
}

int task_kill(const char *name)
{
    struct ctl_task *t = find(name);
    if (!t) return OS_TASK_NOT_EXIST;
    t->used = 0;
    if (t == cur) ctl_port_ctx_switch(&t->ctx, &idle_ctx);
    free(t->stack);
#if CTL_VENDOR_HOST_TEST
    t->stack = NULL;
#endif
    return 0;
}

const char *os_current_task(void) { return cur ? cur->name : "idle"; }
const char *os_current_task_rom(void) { return os_current_task(); }

/* The caller checked its wait condition with interrupts disabled. Publish the
 * wait before restoring them: an ISR posted at restore must leave us runnable.
 * Restore the caller's IRQ state before switching to the idle context. */
static void block_current(u32 flags, u32 deadline, int timed)
{
    struct ctl_task *t = cur;
    if (t) {
        t->wake_ms = deadline;
        t->timed = (u8)timed;
        t->waiting = 1;
    }
    ctl_port_irq_restore(flags);
    if (t) ctl_port_ctx_switch(&t->ctx, &idle_ctx);
    else ctl_port_cpu_relax();   /* a timer callback cannot block */
}

static int taskq_count(struct ctl_task *t)
{
    return (int)((t->qhead - t->qtail) & (CTL_TASKQ_WORDS - 1));
}

int os_taskq_post_type(const char *name, int type, int argc, int *argv)
{
    struct ctl_task *t = find(name);
    if (!t) return OS_TASK_NOT_EXIST;
#if CTL_VENDOR_HOST_TEST
    if (argc < 0 || argc > CTL_TASKQ_WORDS - 3 || (argc && !argv)) return OS_Q_FULL;
#endif
    u32 f = ctl_port_irq_save();
    if (CTL_TASKQ_WORDS - 1 - taskq_count(t) < argc + 2) { ctl_port_irq_restore(f); return OS_Q_FULL; }
    t->q[t->qhead++ & (CTL_TASKQ_WORDS - 1)] = argc + 1;
    t->q[t->qhead++ & (CTL_TASKQ_WORDS - 1)] = type;
    for (int i = 0; i < argc; i++) t->q[t->qhead++ & (CTL_TASKQ_WORDS - 1)] = argv[i];
    t->qhead &= (CTL_TASKQ_WORDS - 1);
    t->waiting = 0;
    ctl_port_irq_restore(f);
    return OS_NO_ERR;
}

int os_taskq_pend(const char *fmt, int *argv, int argc)
{
    (void)fmt;
    struct ctl_task *t = cur;
    if (!t) return OS_Q_EMPTY;
    for (;;) {
        u32 f = ctl_port_irq_save();
        if (taskq_count(t)) {
            int n = t->q[t->qtail++ & (CTL_TASKQ_WORDS - 1)];
            for (int i = 0; i < n; i++) {
                int v = t->q[t->qtail++ & (CTL_TASKQ_WORDS - 1)];
                if (i < argc) argv[i] = v;
            }
            t->qtail &= (CTL_TASKQ_WORDS - 1);
            ctl_port_irq_restore(f);
            return OS_TASKQ;
        }
        block_current(f, 0, 0);
    }
}

/* Semaphores and mutexes use the caller's OS_SEM / OS_MUTEX storage
 * (FreeRTOS StaticSemaphore_t in the SDK, much larger than this). */
struct ctl_sem { volatile int count; u32 magic; };
#define SEM_MAGIC 0x53454D31u

int os_sem_create(void *sem, int count)
{
    struct ctl_sem *s = sem; s->count = count; s->magic = SEM_MAGIC; return OS_NO_ERR;
}
int os_sem_post(void *sem)
{
    struct ctl_sem *s = sem;
    u32 f = ctl_port_irq_save(); s->count++;
    for (int i = 0; i < CTL_MAX_TASKS; i++) tasks[i].waiting = 0;   /* re-check waiters */
    ctl_port_irq_restore(f);
    return OS_NO_ERR;
}
int os_sem_pend(void *sem, int timeout)   /* timeout in ticks (10 ms), 0 = forever */
{
    struct ctl_sem *s = sem;
    u32 end = ctl_ms() + (u32)timeout * (1000u / OS_TICKS_PER_SEC);
    for (;;) {
        u32 f = ctl_port_irq_save();
        if (s->count > 0) { s->count--; ctl_port_irq_restore(f); return OS_NO_ERR; }
#if CTL_VENDOR_HOST_TEST
        if (!cur) {
            ctl_port_irq_restore(f);
            /* btctrler_task_ready posts a controller message then waits on an
             * 80-byte SDK semaphore. Only the explicit startup window may
             * pump that task; timer/normal-idle waits still fail closed. */
            if (timeout && (s32)(ctl_ms() - end) >= 0) return OS_TIMEOUT;
            if (bootstrap_pump()) return OS_TIMEOUT;
            continue;
        }
#endif
        if (timeout && (s32)(ctl_ms() - end) >= 0) {
            ctl_port_irq_restore(f);
            return OS_TIMEOUT;
        }
        block_current(f, end, timeout != 0);
    }
}
int os_sem_set(void *sem, int count) { ((struct ctl_sem *)sem)->count = count; return OS_NO_ERR; }
int os_sem_del(void *sem, int block) { (void)sem; (void)block; return OS_NO_ERR; }

int os_mutex_create(void *m) { return os_sem_create(m, 1); }
int os_mutex_pend(void *m, int timeout) { return os_sem_pend(m, timeout); }
int os_mutex_post(void *m) { return os_sem_post(m); }
int os_mutex_del(void *m, int force) { (void)m; (void)force; return OS_NO_ERR; }

void os_time_dly(int ticks)
{
#if CTL_VENDOR_HOST_TEST
    if (ticks < 0) { ctl_fail(XB_E_ASSERT); return; }
    if (!cur && ticks) { bootstrap_delay(ctl_ms() + (u32)ticks * (1000u / OS_TICKS_PER_SEC)); return; }
#endif
    u32 end = ctl_ms() + (u32)ticks * (1000u / OS_TICKS_PER_SEC);
    for (;;) {
        u32 f = ctl_port_irq_save();
        if ((s32)(ctl_ms() - end) >= 0) { ctl_port_irq_restore(f); return; }
        block_current(f, end, 1);
    }
}

void ctl_os_init(void)
{
    memset(tasks, 0, sizeof(tasks)); cur = NULL;
#if CTL_VENDOR_HOST_TEST
    bootstrap_active = bootstrap_running = 0;
#endif
}

/* One pass: timers and the host mailbox, then each task that may be runnable.
 * A task re-checks its own wait condition. */
int ctl_os_run_once(void)
{
    int ran = 0;
    u32 now;
    ctl_timer_poll();
    ctl_host_poll();
    now = ctl_ms();
    for (int i = 0; i < CTL_MAX_TASKS; i++) {
        struct ctl_task *t = &tasks[i];
        if (!t->used) continue;
        if (t->waiting && t->timed && (s32)(now - t->wake_ms) >= 0) t->waiting = 0;
        if (t->waiting) continue;
        t->wake_ms = 0;
        t->timed = 0;
        cur = t;
        ctl_port_ctx_switch(&idle_ctx, &t->ctx);
        cur = NULL;
#if CTL_VENDOR_HOST_TEST
        if (!t->used && t->stack) { free(t->stack); t->stack = NULL; }
#endif
        ran = 1;
    }
    return ran;
}
