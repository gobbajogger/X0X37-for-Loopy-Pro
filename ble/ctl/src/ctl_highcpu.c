/* SPDX-License-Identifier: MPL-2.0 */
#include <stdint.h>
#include <stddef.h>
#include "ctl_highcpu.h"
#if CTL_VENDOR_HIGHCPU_TEST
void *memset(void *, int, size_t);
extern unsigned char __ctl_highcpu_start[], __ctl_highcpu_end[];
static unsigned char highcpu_initialized; /* deliberately stays in low BSS */
/* Separate for native boundary tests; never accept arbitrary write ranges. */
static int highcpu_clear(uintptr_t begin, uintptr_t end)
{
    const uintptr_t base = 0x01c7d400u, limit = 0x01c7f000u;
    if (highcpu_initialized || begin != base || end <= begin || end > limit ||
        (end & 3u)) return -1;
    memset((void *)base, 0, limit - base);
    highcpu_initialized = 1;
    return 0;
}
int ctl_highcpu_init(void)
{
    return highcpu_clear((uintptr_t)__ctl_highcpu_start,
                         (uintptr_t)__ctl_highcpu_end);
}
#endif
