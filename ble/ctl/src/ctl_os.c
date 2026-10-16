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
/* attempt 45 DIAG: per task (0 controller, 1 host) posts and pend returns (saturate at 15), first runs */
u8 ctl_d_post[2], ctl_d_pend[2], ctl_d_ran, ctl_d_tc_fail;   /* attempt 46: task_create failures */
__attribute__((noinline, section(".bt_stack_code.d45"))) static void d_inc(u8 *c) { if (*c < 15u) (*c)++; }
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
    if (cur - tasks < 2) ctl_d_ran |= (u8)(1u << (cur - tasks));
    cur->fn(cur->arg);
    cur->used = 0;                     /* task returned: free its slot */
    ctl_port_ctx_switch(&cur->ctx, &idle_ctx);
}

__attribute__((section(".bt_stack_code.d46"))) int task_create(void (*fn)(void *p), void *p, const char *name)   /* attempt 46: piece 2 (piece 1 is full) */
{
    for (int i = 0; i < CTL_MAX_TASKS; i++) {
        struct ctl_task *t = &tasks[i];
        if (t->used) continue;
        memset(t, 0, sizeof(*t));
        u32 words = stack_words(name);
        ctl_mark(0x340u + (u32)i * 0x10u);   /* attempt 41: a task is created (0x340, 0x350) */
        t->stack = malloc(words * 4);
        if (!t->stack) { ctl_mark(0x34Fu); d_inc(&ctl_d_tc_fail); return -1; }
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
static u8 in_run_once;                 /* ctl_os_run_once is on the stack */
int ctl_os_run_once(void);
/* Attempt 51: a50's core 1 fault was at ctl_port_ctx_switch's return (PC = RETS 0x020874F6), a
 * switch into a context whose saved frame was not valid. A wait inside an interrupt handler would
 * save the interrupted context on the interrupt stack, which later interrupts overwrite. So a wait
 * in an interrupt, or a switch into a frame outside its task's stack, now stops the image (FAILED,
 * err TASK, the unit goes on) with the culprit in DIAG:
 *   dbg[STAGE] 0x5110  a wait in an interrupt: RA = the waiting function's caller
 *              0x512i  task i's saved stack pointer outside its stack: DELAYS = it, RA = its word
 * Before each switch, dbg[STALL] = 0x51000000 | task << 20 | poll count (the last switch). */
__attribute__((noinline, section(".bt_stack_code.d51"))) static void d_stop(u32 stage, u32 v, u32 ra)
{
    ctl_mb->dbg[XB_DBG_STAGE] = stage;
    ctl_mb->dbg[XB_DBG_DELAYS] = v;
    ctl_mb->dbg[XB_DBG_RA] = ra;
    ctl_fail(XB_E_TASK);
}
static void block_current(u32 ra)
{
    if (ctl_port_in_irq()) d_stop(0x5110u, 0, ra);
    /* Attempts 43 and 44: the init is not a task. btstack_init waits there in os_time_dly, and
     * btctrler_task_ready waits there in os_sem_pend, until the controller task answers. So the
     * init runs the tasks while it waits. A timer callback (inside ctl_os_run_once) only spins. */
    if (!cur) {
        if (!in_run_once) (void)ctl_os_run_once();
        else ctl_port_cpu_relax();
        return;
    }
    cur->waiting = 1;
    ctl_port_ctx_switch(&cur->ctx, &idle_ctx);
}

static int taskq_count(struct ctl_task *t)
{
    return (int)((t->qhead - t->qtail) & (CTL_TASKQ_WORDS - 1));
}

__attribute__((used, section(".ctl_ram2.sw"))) volatile u32 ctl_sw_ring[16];
__attribute__((used)) volatile u32 ctl_sw_n;
/* attempt 57 fix: le_hw_upload_data (btctrler) frees each received DMA buffer right after its
 * handler returns, but the handler (btstack hci_packet_handler) only queues the ACL packet's
 * pointer; the host reads and frees it later (a56: the buffer was already free when the host took
 * the message, stage 0x5601). So an ACL message gets its own copy here, in a fresh buffer of the
 * same RX pool (hw_dma_alloc(0, ...), as hw_alloc_rx), laid out the same (packet at buffer + 16):
 * the host frees the copy, the controller frees its original. No buffer: OS_Q_FULL (the handler
 * waits and posts again, outside an interrupt). */
extern void *hw_dma_alloc(int pool, u32 n);
__attribute__((noinline)) static int acl_copy(int *argv)
{
    u32 n = (u32)argv[1];
    u8 *b;
    if (n > 600u || !(b = hw_dma_alloc(0, n + 20u))) return -1;
    memcpy(b + 16, (const void *)(uintptr_t)argv[0], n);
    argv[0] = (int)(uintptr_t)(b + 16);
    return 0;
}
int os_taskq_post_type(const char *name, int type, int argc, int *argv)
{
    struct ctl_task *t = find(name);
    if (!t) return OS_TASK_NOT_EXIST;
    int cp[2];
    if (type == 0x400002 && argc == 2) {
        cp[0] = argv[0]; cp[1] = argv[1];
        if (acl_copy(cp)) { if (ctl_q_full < 255u) ctl_q_full++; return OS_Q_FULL; }
        argv = cp;
    }
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
    if (t - tasks < 2) d_inc(&ctl_d_post[t - tasks]);
    ctl_port_irq_restore(f);
    return OS_NO_ERR;
}

/* attempt 58 DIAG (piece 2): which messages the host task receives (event 0x400001, ACL 0x400002,
 * 0x400004 ...) and whether an LE connection-complete event arrives, in ctl_sw_ring {type, count}. */
__attribute__((noinline)) static void d_hostmsg(int type, int n)
{
    u32 j = ctl_sw_n & 14u;
    ctl_sw_ring[j] = (u32)type; ctl_sw_ring[j + 1u] = (u32)n; ctl_sw_n += 2u;
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
            if (t == &tasks[1] && argc >= 1) d_hostmsg(argv[0], n);
            ctl_port_irq_restore(f);
            if (t - tasks < 2) d_inc(&ctl_d_pend[t - tasks]);
            return OS_TASKQ;
        }
        ctl_port_irq_restore(f);
        block_current((u32)(uintptr_t)__builtin_return_address(0));
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
        block_current((u32)(uintptr_t)__builtin_return_address(0));
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
        block_current((u32)(uintptr_t)__builtin_return_address(0));
    }
}

void ctl_os_init(void) { memset(tasks, 0, sizeof(tasks)); cur = NULL; }

/* One pass: timers and the host mailbox, then each task that may be runnable.
 * A task re-checks its own wait condition. */
__attribute__((noinline, section(".bt_stack_code.d45"))) u32 ctl_os_d_waiting(void) /* attempt 45 DIAG: bit i = task i waits */
{
    return (tasks[0].used && tasks[0].waiting ? 1u : 0u) | (tasks[1].used && tasks[1].waiting ? 2u : 0u);
}
__attribute__((noinline, section(".bt_stack_code.d51"))) static void d_check(struct ctl_task *t, int i)
{
    u32 *sp = t->ctx.sp, *lo = t->stack, *hi = lo + stack_words(t->name);
    if (ctl_mb->state == XB_ST_READY)
        ctl_mb->dbg[XB_DBG_STALL] = 0x51000000u | (u32)i << 20 | (ctl_mb->polls & 0xFFFFFu);
    if (sp < lo || sp + 13 > hi) d_stop(0x5120u + (u32)i, (u32)(uintptr_t)sp, sp >= lo && sp < hi ? sp[0] : 0);
}
/* attempt 56 DIAG (was the attempt 52 switch ring): the last 8 baseband DMA buffer allocs/frees
 * (ctl_heap.c __wrap_hw_dma_alloc / __wrap_hw_dma_free), each {buffer | op, caller RA}:
 * op bit 0 = alloc, bit 1 = a free of a block whose pool word is not a pool (double free: halted).
 * tools/ble_autotest.py reads it by address from ctl.elf */
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
        d_check(t, i);
        cur = t;
        ctl_port_ctx_switch(&idle_ctx, &t->ctx);
        cur = NULL;
        ran = 1;
    }
    in_run_once = 0;
    return ran;
}
