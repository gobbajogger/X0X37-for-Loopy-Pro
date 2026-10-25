/* SPDX-License-Identifier: GPL-3.0-only */
/* Full store ordering on the AC79 (shared caches; see fm1_cpu1.h). */
#pragma once

static inline void fm1_data_sync(void)
{
    __asm__ volatile("csync" ::: "memory");
}
