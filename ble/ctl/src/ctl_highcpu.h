/* SPDX-License-Identifier: MPL-2.0 */
#ifndef CTL_HIGHCPU_H
#define CTL_HIGHCPU_H
#ifndef CTL_VENDOR_HIGHCPU_TEST
#define CTL_VENDOR_HIGHCPU_TEST 0
#endif
#if CTL_VENDOR_HIGHCPU_TEST
#if !defined(CTL_VENDOR_HOST_TEST) || !CTL_VENDOR_HOST_TEST
#error "High CPU data requires the vendor-host diagnostic profile"
#endif
#define CTL_HIGHCPU_BSS __attribute__((section(".ctl_highcpu_bss"), aligned(4)))
/* Call exactly once, after low BSS clear and before any scheduler/profile use.
 * Linker must provide separate NOLOAD bounds; this is not ABI admission. */
int ctl_highcpu_init(void);
#else
#define CTL_HIGHCPU_BSS
#endif
#endif
