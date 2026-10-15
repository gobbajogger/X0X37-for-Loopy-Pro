// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Cooperative task table, task queues, semaphores and mutexes for the controller image.
 * No preemption: a task runs until it pends. Interrupt handlers may post (queue, semaphore);
 * they never pend. The "idle context" is poll() on its own stack: it runs each runnable task
 * once and returns to the app's idle loop on core 1 (ctl_os_run_once). */
#include "ctl_port.h"


#define CTL_MAX_TASKS   2   /* the controller's task; ABI 3: and the host's ("btstack") */
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

#ifdef CTL_HOST
static struct ctl_task tasks[CTL_MAX_TASKS] __attribute__((section(".ctl_ram2")));   /* RAM 2 (ctl.ld) */
#else
static struct ctl_task tasks[CTL_MAX_TASKS];
#endif
static struct ctl_task *cur;           /* NULL = idle context */
u32 ctl_q_full;                        /* task queue full (DIAG) */
static struct ctl_ctx idle_ctx;

/* Stack size by task name, as the SDK's table: the controller 512 words; the host ("btstack") 768
 * (the SDK's size for it). The stacks are painted (0xA5) so DIAG shows how deep they went. */
#define CTL_TASK_STACK_WORDS 512
static u32 stack_words(const char *name)
{
    const char *n = name;
    while (n && *n) {                     /* "btstack" or "#C0btstack" */
        if (strncmp(n, "btstack", 7) == 0 && n[7] == 0) return 768u;
        n++;
    }
    return CTL_TASK_STACK_WORDS;
}
u32 ctl_stack_free(const u8 *lo, const u8 *hi)
{
    const u8 *p = lo;
    while (p < hi && *p == 0xA5u) p++;
    return (u32)(p - lo);
}
u32 ctl_os_stack_free(void)
{
    u32 m = 0xFFFFFFFFu;
    for (int i = 0; i < CTL_MAX_TASKS; i++) {
        struct ctl_task *t = &tasks[i];
        if (!t->used || !t->stack) continue;
        u32 f = ctl_stack_free((const u8 *)t->stack, (const u8 *)t->stack + stack_words(t->name) * 4u);
        if (f < m) m = f;
    }
    return m == 0xFFFFFFFFu ? 0 : m;
}

static struct ctl_task *find(const char *name)
{
    if (!name) return cur;
    for (int i = 0; i < CTL_MAX_TASKS; i++)
        if (tasks[i].used && strcmp(tasks[i].name, name) == 0) return &tasks[i];
    return NULL;
}

static void task_trampoline(void)
{
    ctl_mark(0x360u + (u32)(cur - tasks));   /* attempt 41: the task runs for the first time */
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
        u32 words = stack_words(name);
        ctl_mark(0x340u + (u32)i * 0x10u);   /* attempt 41: a task is created (0x340, 0x350) */
        t->stack = malloc(words * 4);
        if (!t->stack) { ctl_mark(0x34Fu); return -1; }
        ctl_mark_v(0x344u + (u32)i * 0x10u, (u32)(uintptr_t)t->stack);   /* malloc gave this */
        memset(t->stack, 0xA5, words * 4);
        ctl_mark(0x345u + (u32)i * 0x10u);
        t->name = name; t->fn = fn; t->arg = p; t->used = 1; t->waiting = 0;
        ctl_port_ctx_init(&t->ctx, t->stack, words * 4, task_trampoline);
        ctl_mark(0x346u + (u32)i * 0x10u);
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
    if (CTL_TASKQ_WORDS - 1 - taskq_count(t) < argc + 2) {
        if (ctl_q_full < 255u) ctl_q_full++;   /* DIAG: a lost message */
        ctl_port_irq_restore(f);
        return OS_Q_FULL;
    }
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

static u8 in_run_once;                 /* attempt 43: ctl_os_run_once is on the stack */
void os_time_dly(int ticks)
{
    u32 end = ctl_ms() + (u32)ticks * (1000u / OS_TICKS_PER_SEC);
    if (!cur && !in_run_once) {
        /* attempt 43: called from the init (not a task, not a timer callback). btstack_init waits
         * here until btctrler_task_ready(): the controller task must run meanwhile. Before, this
         * spun, the task never ran, and the watchdog reset the unit (attempts 40 to 42). */
        while ((s32)(ctl_ms() - end) < 0)
            (void)ctl_os_run_once();
        return;
    }
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
    in_run_once = 1;
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
    in_run_once = 0;
    return ran;
}
