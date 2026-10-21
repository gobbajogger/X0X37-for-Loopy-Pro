/* SPDX-License-Identifier: MPL-2.0 */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
#define CTL_VENDOR_HIGHCPU_TEST 1
#define CTL_VENDOR_CPUPOOL_TEST 1
#define XB_E_HEAP 2
#define XB_E_TASK 3
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
static int current_task, irq, failure;
static jmp_buf fail_jump;
void *ctl_os_scope_owner(void) { return current_task ? &current_task : NULL; }
int ctl_port_in_irq(void) { return irq; }
u32 ctl_port_irq_save(void) { return 0; }
void ctl_port_irq_restore(u32 f) { (void)f; }
void ctl_fail(u32 code) { failure = (int)code; longjmp(fail_jump, 1); }
u8 __ctl_heap_start[16384] __attribute__((aligned(8)));
__asm__(".global __ctl_heap_end\n.set __ctl_heap_end, __ctl_heap_start + 16384");
#include "../../ble/ctl/src/ctl_host_scope.c"
#define malloc study_malloc
#define free study_free
#include "../../ble/ctl/src/ctl_heap.c"
#undef malloc
#undef free
static void reset(void)
{
    phase = ordinal = completed = diagnostic = 0; owner = host_owner = NULL;
    irq = failure = 0; current_task = 1; ctl_heap_init();
}
static int is_high(void *p) { return (u8 *)p >= cpu_pool && (u8 *)p < cpu_pool + sizeof cpu_pool; }
#define FATAL(stmt, code) do { failure = 0; if (!setjmp(fail_jump)) { stmt; assert(!"failure returned"); } assert(failure == code); } while (0)
int main(void)
{
    reset();
    ctl_scope_task_enter("btctrler"); assert(!phase);
    ctl_scope_task_enter("btstack");
    irq = 1; void *interrupt = study_malloc(1240); irq = 0;
    assert(!is_high(interrupt) && ordinal == 0);
    void *dma = ram_malloc(128); assert(!is_high(dma) && ordinal == 0);
    void *a = study_malloc(1240), *b = study_malloc(128);
    assert(is_high(a) && is_high(b)); ctl_scope_end(1);
    ctl_scope_begin(2); void *c = study_malloc(56); ctl_scope_end(2);
    ctl_scope_begin(3); void *d = study_malloc(480); ctl_scope_end(3);
    assert(is_high(c) && is_high(d) && completed == 3 && !phase);
    void *normal = study_malloc(56); assert(!is_high(normal));
    study_free(b); study_free(d); study_free(a); study_free(c);
    study_free(interrupt); ram_free(dma); study_free(normal);
    FATAL(study_free(c), XB_E_HEAP); /* double free, outside phase */
    reset(); ctl_scope_begin(1); FATAL(ctl_scope_no_yield(), XB_E_TASK);
    reset(); ctl_scope_begin(1); FATAL(ctl_scope_begin(2), XB_E_TASK);
    reset(); ctl_scope_begin(1); FATAL(ctl_scope_end(1), XB_E_TASK);
    reset(); ctl_scope_begin(1); FATAL(study_malloc(128), XB_E_HEAP);
    assert((ctl_scope_diagnostic() & 65535u) == 128);
    reset(); ctl_scope_begin(1); current_task = 0; FATAL(study_malloc(1240), XB_E_TASK);
    reset(); ctl_scope_begin(1); cpu_first->size = 8;
    FATAL(study_malloc(1240), XB_E_HEAP); /* cannot fall back to low heap */
    reset(); irq = 1; FATAL(ctl_scope_begin(1), XB_E_TASK);
    reset(); ctl_scope_begin(1); FATAL(study_free(cpu_pool + 7), XB_E_HEAP);
    puts("CPU init scope: phases, owner/IRQ, no yield, range free, OOM/no fallback PASS");
}
