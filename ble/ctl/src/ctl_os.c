// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Cooperative task table, task queues, semaphores and mutexes for the controller image.
 * No preemption: a task runs until it pends. Interrupt handlers may post (queue, semaphore);
 * they never pend. The "idle context" is poll() on its own stack: it runs each runnable task
 * once and returns to the app's idle loop on core 1 (ctl_os_run_once). */
#include "ctl_port.h"


#define CTL_MAX_TASKS   1   /* btctrler_task_init creates one controller task */
#define CTL_TASKQ_WORDS 64

struct ctl_task {
    const char *name;
    void (*fn)(void *);
    void *arg;
    struct ctl_ctx ctx;
    u32 *stack;
    u32 wake_ms;          /* for os_time_dly / timed pend */
    u8 used, waiting;
    u16 qhead, qtail;     /* message ring (words); a message = [count, type, args...] */
    int q[CTL_TASKQ_WORDS];
};

static struct ctl_task tasks[CTL_MAX_TASKS];
static struct ctl_task *cur;           /* NULL = idle context */
static struct ctl_ctx idle_ctx;

/* Stack size by task name. The SDK uses a name table; this image has
 * only the controller task, so one default is enough. */
#define CTL_TASK_STACK_WORDS 512

static struct ctl_task *find(const char *name)
{
    if (!name) return cur;
    /* This image has one controller task. The SDK's named queue always addresses it. */
    return tasks[0].used ? &tasks[0] : NULL;
}

static void task_trampoline(void)
{
    cur->fn(cur->arg);
    cur->used = 0;                     /* task returned: free its slot */
    ctl_port_ctx_switch(&cur->ctx, &idle_ctx);
}

int task_create(void (*fn)(void *p), void *p, const char *name)
{
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
    return 0;
}

const char *os_current_task(void) { return cur ? cur->name : "idle"; }
const char *os_current_task_rom(void) { return os_current_task(); }

/* Give the CPU back to the idle loop until 'ready' says the wait is over. */
static void block_current(void)
{
    if (!cur) { ctl_port_cpu_relax(); return; }   /* idle context (a timer callback): it cannot block, it spins */
    cur->waiting = 1;
    ctl_port_ctx_switch(&cur->ctx, &idle_ctx);
}

static int taskq_count(struct ctl_task *t)
{
    return (int)((t->qhead - t->qtail) & (CTL_TASKQ_WORDS - 1));
}

int os_taskq_post_type(const char *name, int type, int argc, int *argv)
{
    struct ctl_task *t = find(name);
    if (!t) return OS_TASK_NOT_EXIST;
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
        ctl_port_irq_restore(f);
        block_current();
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
    u32 f = ctl_port_irq_save(); s->count++; ctl_port_irq_restore(f);
    for (int i = 0; i < CTL_MAX_TASKS; i++) tasks[i].waiting = 0;   /* re-check waiters */
    return OS_NO_ERR;
}
int os_sem_pend(void *sem, int timeout)   /* timeout in ticks (10 ms), 0 = forever */
{
    struct ctl_sem *s = sem;
    u32 end = ctl_ms() + (u32)timeout * (1000u / OS_TICKS_PER_SEC);
    for (;;) {
        u32 f = ctl_port_irq_save();
        if (s->count > 0) { s->count--; ctl_port_irq_restore(f); return OS_NO_ERR; }
        ctl_port_irq_restore(f);
        if (timeout && (s32)(ctl_ms() - end) >= 0) return OS_TIMEOUT;
        if (cur && timeout) cur->wake_ms = end;
        block_current();
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
    u32 end = ctl_ms() + (u32)ticks * (1000u / OS_TICKS_PER_SEC);
    while ((s32)(ctl_ms() - end) < 0) {
        if (cur) cur->wake_ms = end;
        block_current();
    }
}

void ctl_os_init(void) { memset(tasks, 0, sizeof(tasks)); cur = NULL; }

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
        if (t->waiting && t->wake_ms && (s32)(now - t->wake_ms) >= 0) t->waiting = 0;
        if (t->waiting) continue;
        t->wake_ms = 0;
        cur = t;
        ctl_port_ctx_switch(&idle_ctx, &t->ctx);
        cur = NULL;
        ran = 1;
    }
    return ran;
}
