/* SPDX-License-Identifier: MPL-2.0 */
/* Execute the production scheduler with real host context switches. Interrupt
 * posts occur at IRQ restore and immediately before the task yields. */
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#define CTL_PORT_H
#define OS_NO_ERR 0
#define OS_TIMEOUT 11
#define OS_TASKQ 13
#define OS_TASK_NOT_EXIST 14
#define OS_Q_FULL 21
#define OS_Q_EMPTY 22
#define OS_TICKS_PER_SEC 100
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
struct ctl_ctx { ucontext_t host; };
void ctl_port_ctx_init(struct ctl_ctx *, void *, u32, void (*)(void));
void ctl_port_ctx_switch(struct ctl_ctx *, struct ctl_ctx *);
u32 ctl_port_irq_save(void);
void ctl_port_irq_restore(u32);
void ctl_port_cpu_relax(void);
void ctl_timer_poll(void);
u32 ctl_ms(void);
void ctl_host_poll(void);
#include "../../ble/ctl/src/ctl_os.c"

static unsigned char host_stack[65536];
static u32 clock_ms, irq_off;
static int kind, timeout_ticks, completed, result, inject_at, inject_kind;
static int args[4];
static struct ctl_sem semaphore;
static unsigned cases;

static void post_interrupt(void)
{
    int event = inject_kind;
    inject_kind = 0;
    if (event == 1) {
        int value = 123;
        assert(os_taskq_post_type("btctrler", 42, 1, &value) == OS_NO_ERR);
    } else if (event == 2) {
        assert(os_sem_post(&semaphore) == OS_NO_ERR);
    } else if (event == 3) {
        assert(os_mutex_post(&semaphore) == OS_NO_ERR);
    }
}

void ctl_port_ctx_init(struct ctl_ctx *c, void *stack, u32 size, void (*entry)(void))
{
    (void)stack; (void)size;
    assert(getcontext(&c->host) == 0);
    c->host.uc_stack.ss_sp = host_stack;
    c->host.uc_stack.ss_size = sizeof host_stack;
    c->host.uc_link = NULL;
    makecontext(&c->host, entry, 0);
}

void ctl_port_ctx_switch(struct ctl_ctx *from, struct ctl_ctx *to)
{
    assert(irq_off == 0); /* waiting must not leave the idle loop IRQ-disabled */
    if (from != &idle_ctx && inject_at == 2 && inject_kind) post_interrupt();
    assert(swapcontext(&from->host, &to->host) == 0);
}

u32 ctl_port_irq_save(void) { u32 old = irq_off; irq_off = 1; return old; }
void ctl_port_irq_restore(u32 old)
{
    irq_off = old;
    if (!old && inject_at == 1 && inject_kind) post_interrupt();
}
void ctl_port_cpu_relax(void) { clock_ms++; }
void ctl_timer_poll(void) {}
u32 ctl_ms(void) { return clock_ms; }
void ctl_host_poll(void) {}

static void waiter(void *unused)
{
    (void)unused;
    if (kind == 1) result = os_taskq_pend("taskq", args, 4);
    else if (kind == 2) result = os_sem_pend(&semaphore, timeout_ticks);
    else if (kind == 3) result = os_mutex_pend(&semaphore, timeout_ticks);
    else { os_time_dly(timeout_ticks); result = OS_NO_ERR; }
    completed++;
}

static void setup(int task_kind, int ticks)
{
    free(tasks[0].stack);
    ctl_os_init();
    kind = task_kind; timeout_ticks = ticks;
    completed = 0; result = -1; inject_at = inject_kind = 0;
    clock_ms = 1000; irq_off = 0;
    memset(args, 0, sizeof args);
    os_sem_create(&semaphore, 0);
    assert(task_create(waiter, NULL, "btctrler") == 0);
}

static void boundary_post(int task_kind, int point, int ticks)
{
    setup(task_kind, ticks);
    inject_at = point; inject_kind = task_kind;
    assert(ctl_os_run_once() == 1);
    assert(!completed && tasks[0].waiting == 0);
    assert(ctl_os_run_once() == 1);
    assert(completed == 1);
    assert(result == (task_kind == 1 ? OS_TASKQ : OS_NO_ERR));
    if (task_kind == 1) assert(args[0] == 42 && args[1] == 123 && taskq_count(&tasks[0]) == 0);
    else assert(semaphore.count == 0);
    printf("PASS boundary post kind=%d point=%d timeout=%d\n", task_kind, point, ticks);
    cases++;
}

int main(void)
{
    for (int point = 1; point <= 2; point++) {
        boundary_post(1, point, 0);
        boundary_post(2, point, 0);
        boundary_post(2, point, 3);
        boundary_post(3, point, 0);
        boundary_post(3, point, 3);
    }
    for (int task_kind = 1; task_kind <= 3; task_kind++) {
        setup(task_kind, 0);
        assert(ctl_os_run_once() == 1 && tasks[0].waiting);
        for (int i = 0; i < 10; i++) assert(ctl_os_run_once() == 0);
        inject_kind = task_kind; post_interrupt();
        assert(ctl_os_run_once() == 1 && completed == 1);
        assert(result == (task_kind == 1 ? OS_TASKQ : OS_NO_ERR));
        cases++;
    }
    for (int task_kind = 2; task_kind <= 4; task_kind++) {
        setup(task_kind, 2);
        assert(ctl_os_run_once() == 1 && !completed);
        clock_ms = 1019;
        assert(ctl_os_run_once() == 0);
        clock_ms = 1020;
        assert(ctl_os_run_once() == 1 && completed == 1);
        assert(result == (task_kind == 4 ? OS_NO_ERR : OS_TIMEOUT));
        cases++;
    }
    /* A finite deadline that wraps to zero is still a timed wait. */
    setup(2, 1); clock_ms = UINT32_MAX - 9u;
    assert(ctl_os_run_once() == 1 && !completed);
    clock_ms = UINT32_MAX;
    assert(ctl_os_run_once() == 0);
    clock_ms = 0;
    assert(ctl_os_run_once() == 1 && completed == 1 && result == OS_TIMEOUT);
    cases++;
    /* Timed spurious wakeups recheck the condition and keep the original end. */
    setup(2, 3);
    assert(ctl_os_run_once() == 1);
    clock_ms = 1010; tasks[0].waiting = 0;
    assert(ctl_os_run_once() == 1 && !completed && tasks[0].waiting);
    clock_ms = 1029; assert(ctl_os_run_once() == 0);
    clock_ms = 1030; assert(ctl_os_run_once() == 1 && result == OS_TIMEOUT);
    cases++;
    free(tasks[0].stack); tasks[0].stack = NULL;
    /* Idle-context timed pends spin without trying to switch stacks. */
    ctl_os_init(); os_sem_create(&semaphore, 0); clock_ms = 0;
    assert(os_sem_pend(&semaphore, 1) == OS_TIMEOUT && clock_ms == 10);
    cases++;
    printf("controller wait regression: %u cases passed\n", cases);
    return 0;
}
