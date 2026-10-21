/* SPDX-License-Identifier: MPL-2.0 */
#ifndef CTL_HOST_SCOPE_H
#define CTL_HOST_SCOPE_H
#include <stddef.h>
#include "ctl_highcpu.h"
#ifndef CTL_VENDOR_CPUPOOL_TEST
#define CTL_VENDOR_CPUPOOL_TEST 0
#endif
#if CTL_VENDOR_CPUPOOL_TEST
#if !CTL_VENDOR_HIGHCPU_TEST
#error "CPU init pool requires the bounded high CPU diagnostic"
#endif
void *ctl_os_scope_owner(void);
void ctl_scope_task_enter(const char *name);
void ctl_scope_begin(unsigned phase);
void ctl_scope_end(unsigned phase);
void ctl_scope_no_yield(void);
int ctl_scope_allocate(size_t size);
void ctl_scope_oom(size_t size);
unsigned ctl_scope_diagnostic(void);
#else
#define ctl_scope_task_enter(name) ((void)0)
#define ctl_scope_begin(phase) ((void)0)
#define ctl_scope_end(phase) ((void)0)
#define ctl_scope_no_yield() ((void)0)
#endif
#endif
