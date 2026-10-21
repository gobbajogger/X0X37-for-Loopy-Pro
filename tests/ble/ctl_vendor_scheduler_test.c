/* SPDX-License-Identifier: MPL-2.0 */
/* Native execution of the actual cooperative scheduler, not a scheduler model.
 * Native context stacks are enlarged for libc/ucontext; target stack sizing is
 * explicitly not covered by this test. */
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
#define OS_NO_ERR 0
#define OS_TIMEOUT 11
#define OS_TASKQ 13
#define OS_TASK_NOT_EXIST 14
#define OS_Q_FULL 21
#define OS_Q_EMPTY 22
#define OS_TICKS_PER_SEC 100
#define XB_E_ASSERT 1
#define XB_E_TASK 3

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
struct ctl_ctx { ucontext_t uc; };
static unsigned live_allocs, failures, clock_ms;
static int interrupts;
static void *test_malloc(size_t n) { ++live_allocs; return malloc(n < 65536 ? 65536 : n); }
static void test_free(void *p) { if (p) { assert(live_allocs); --live_allocs; free(p); } }
static u32 ctl_ms(void) { return clock_ms; }
static u32 ctl_port_irq_save(void) { u32 old = (u32)interrupts; interrupts = 1; return old; }
static void ctl_port_irq_restore(u32 old) { interrupts = (int)old; }
static void ctl_port_ctx_init(struct ctl_ctx *c, void *stack, u32 n, void (*entry)(void))
{
    (void)n; assert(!getcontext(&c->uc)); c->uc.uc_stack.ss_sp = stack;
    c->uc.uc_stack.ss_size = 65536; c->uc.uc_link = NULL;
    makecontext(&c->uc, entry, 0);
}
static void ctl_port_ctx_switch(struct ctl_ctx *from, struct ctl_ctx *to)
{ assert(!swapcontext(&from->uc, &to->uc)); }
static void ctl_fail(u32 e) { assert(e == XB_E_ASSERT || e == XB_E_TASK); ++failures; }
static void ctl_port_wdt_feed(void) {}
int ctl_os_run_once(void);
static int advance_clock;
static void ctl_timer_poll(void) { if (advance_clock) ++clock_ms; }
static void ctl_host_poll(void) {}
static void ctl_port_cpu_relax(void) { assert(!"idle context attempted to spin"); }
#define malloc test_malloc
#define free test_free
#include "../../ble/ctl/src/ctl_os.c"
#undef malloc
#undef free

static int observed[2], stop_count;
static void consumer(void *arg)
{
    int id = (int)(intptr_t)arg, msg[8];
    for (;;) {
        assert(os_taskq_pend(NULL, msg, 8) == OS_TASKQ);
        if (msg[0] == 9) { ++stop_count; return; }
        assert(msg[0] == id + 100);
        observed[id] += msg[1];
    }
}
static int semaphore_reached;
static struct ctl_sem sem;
static void sem_waiter(void *arg)
{
    (void)arg;
    assert(os_sem_pend(&sem, 0) == OS_NO_ERR);
    semaphore_reached = 1;
    os_time_dly(2);
    semaphore_reached = 2;
}
static void bootstrap_controller(void *arg)
{
    os_sem_post(arg);
}
int main(void)
{
    int value = 3;
    ctl_os_init();
    assert(task_create(consumer, (void *)0, "btctrler") == 0);
    assert(task_create(consumer, (void *)1, "btstack") == 0);
    assert(task_create(consumer, 0, "btstack") != 0);
    assert(task_create(consumer, 0, "extra") != 0);
    assert(os_taskq_post_type("unknown", 100, 1, &value) == OS_TASK_NOT_EXIST);
    assert(os_taskq_post_type("btstack", 101, -1, &value) == OS_Q_FULL);
    assert(os_taskq_post_type("btstack", 101, 62, &value) == OS_Q_FULL);
    assert(os_taskq_post_type("btstack", 101, 1, NULL) == OS_Q_FULL);
    assert(os_taskq_post_type("btctrler", 100, 1, &value) == 0);
    value = 7;
    assert(os_taskq_post_type("btstack", 101, 1, &value) == 0);
    assert(ctl_os_run_once() == 1);
    assert(observed[0] == 3 && observed[1] == 7);
    assert(ctl_os_run_once() == 0);
    /* Ring wraps, rejects overflow, and wakes only the named recipient. */
    value = 1;
    for (int batch = 0; batch < 4; ++batch) {
        for (int i = 0; i < 21; ++i) assert(os_taskq_post_type("btstack", 101, 1, &value) == 0);
        assert(os_taskq_post_type("btstack", 101, 1, &value) == OS_Q_FULL);
        ctl_os_run_once();
    }
    assert(observed[0] == 3 && observed[1] == 91);
    assert(os_taskq_post_type("btctrler", 9, 0, NULL) == 0);
    assert(os_taskq_post_type("btstack", 9, 0, NULL) == 0);
    ctl_os_run_once();
    assert(stop_count == 2 && live_allocs == 0);
    os_sem_create(&sem, 0);
    assert(task_create(sem_waiter, NULL, "btstack") == 0);
    ctl_os_run_once(); assert(semaphore_reached == 0);
    os_sem_post(&sem); ctl_os_run_once(); assert(semaphore_reached == 1);
    clock_ms += 19; ctl_os_run_once(); assert(semaphore_reached == 1);
    clock_ms += 1; ctl_os_run_once(); assert(semaphore_reached == 2);
    assert(live_allocs == 0);
    /* Unsupported idle/timer blocking fails closed instead of starving all tasks. */
    assert(os_sem_pend(&sem, 0) == OS_TIMEOUT && failures == 1);
    os_time_dly(1); assert(failures == 2);
    assert(!interrupts);
    os_sem_create(&sem, 0);
    assert(task_create(bootstrap_controller, &sem, "btctrler") == 0);
    assert(ctl_os_bootstrap_begin(100) == 0);
    assert(os_sem_pend(&sem, 0) == OS_NO_ERR && live_allocs == 0);
    ctl_os_bootstrap_end();
    assert(ctl_os_bootstrap_begin(0) != 0);
    assert(ctl_os_bootstrap_begin(5001) != 0);
    assert(ctl_os_bootstrap_begin(100) == 0);
    assert(ctl_os_bootstrap_begin(100) != 0);
    advance_clock = 1;
    u32 before = clock_ms;
    os_time_dly(2); assert(clock_ms == before + 20 && failures == 2);
    ctl_os_bootstrap_end();
    os_time_dly(1); assert(failures == 3);
    assert(ctl_os_bootstrap_begin(5) == 0);
    before = clock_ms;
    os_time_dly(2); assert(clock_ms == before + 5 && failures == 4);
    ctl_os_bootstrap_end();
    assert(ctl_os_bootstrap_begin(3) == 0);
    before = clock_ms;
    assert(os_sem_pend(&sem, 0) == OS_TIMEOUT);
    assert(clock_ms == before + 3 && failures == 5);
    ctl_os_bootstrap_end();
    puts("vendor scheduler: named queues, wrap/full, wait/wake, cleanup, idle guard PASS");
}
