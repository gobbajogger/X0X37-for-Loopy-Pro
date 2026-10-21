/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* ED_SMC_READ39: fixed, read-only DEV profile regions. No arbitrary flash access.
 * Request region, relative offset (three 7-bit bytes), count in byte pairs.
 * Reply repeats region/offset, then at most224 bytes packed by editor.c. */
static inline int xsr_request(const uint8_t *a, uint32_t na,
                              uint32_t *address, uint32_t *length)
{
    uint32_t base, size, off, n;
    if (!a || na != 5u) return 0;
    for (unsigned i=0;i<5;i++) if (a[i]&128u) return 0;
    switch (a[0]) {
    case 0: base=0x84000u; size=0x17000u; break;
    case 1: base=0xAB000u; size=0x2000u; break;
    case 2: base=0xAD000u; size=0x2000u; break;
    case 3: base=0xFE000u; size=0x1000u; break;
    default: return 0;
    }
    off=(uint32_t)a[1]|(uint32_t)a[2]<<7|(uint32_t)a[3]<<14;
    n=a[4]>112u?224u:(uint32_t)a[4]*2u;
    if (off>=size || !n) return 0;
    if (n>size-off) n=size-off;
    *address=base+off; *length=n;
    return 1;
}
