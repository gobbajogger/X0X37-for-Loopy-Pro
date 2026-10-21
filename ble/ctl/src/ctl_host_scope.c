/* SPDX-License-Identifier: MPL-2.0 */
#include "ctl_port.h"
#include "ctl_host_scope.h"
#if CTL_VENDOR_CPUPOOL_TEST
/* Only the pinned startup phase, never the lifetime of the host task. */
static void *owner CTL_HIGHCPU_BSS, *host_owner CTL_HIGHCPU_BSS;
static unsigned phase CTL_HIGHCPU_BSS, ordinal CTL_HIGHCPU_BSS;
static unsigned completed CTL_HIGHCPU_BSS, diagnostic CTL_HIGHCPU_BSS;
static void scope_fail(unsigned code, size_t size)
{
    diagnostic = (phase << 24) | ((ordinal & 255u) << 16) | ((unsigned)size & 65535u);
    ctl_fail(code);
    for (;;) {} /* fail hook must not return into unchecked vendor initialization */
}
unsigned ctl_scope_diagnostic(void) { return diagnostic; }
void ctl_scope_begin(unsigned next)
{
    if (phase || next < 1 || next > 3 || completed != next - 1 ||
        ctl_port_in_irq() || !ctl_os_scope_owner()) scope_fail(XB_E_TASK, 0);
    if (next > 1 && host_owner != ctl_os_scope_owner()) scope_fail(XB_E_TASK, 0);
    owner = ctl_os_scope_owner();
    if (next == 1) host_owner = owner;
    ordinal = 0; phase = next;
}
void ctl_scope_task_enter(const char *name)
{
    if (!strcmp(name, "btstack")) ctl_scope_begin(1);
}
void ctl_scope_end(unsigned expected)
{
    if (phase != expected || owner != ctl_os_scope_owner() || ctl_port_in_irq() ||
        ordinal != (phase == 1 ? 2u : 1u)) scope_fail(XB_E_TASK, 0);
    completed = phase; phase = 0; owner = NULL;
}
void ctl_scope_no_yield(void)
{
    if (phase) scope_fail(XB_E_TASK, 0);
}
int ctl_scope_allocate(size_t size)
{
    if (!phase || ctl_port_in_irq()) return 0; /* IRQ allocations stay low */
    if (owner != ctl_os_scope_owner()) scope_fail(XB_E_TASK, size);
    /* An assertion on the verified init contract, NOT a size-based selector. */
    unsigned expected = phase == 1 ? (ordinal == 0 ? 1240u : 128u) :
                        phase == 2 ? 56u : 480u;
    if (ordinal >= (phase == 1 ? 2u : 1u) || size != expected)
        scope_fail(XB_E_HEAP, size);
    ++ordinal;
    return 1;
}
void ctl_scope_oom(size_t size) { scope_fail(XB_E_HEAP, size); }
#endif
