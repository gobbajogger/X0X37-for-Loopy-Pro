/* SPDX-License-Identifier: GPL-3.0-only */
/* Existing fm1_cpu1_halt and fm1_audio_stop register operations, separated
 * from app-owned CPU1 globals so the persistent RAM loader can link them.
 * NOT an RF shutdown: requires cold pre-controller recovery admission. */
static inline void bp_loader_quiesce(void)
{
    *(volatile uint32_t *)0x1EEE004u |= 2u;
    *(volatile uint32_t *)0x1EEE004u &= ~8u;
    __asm__ volatile("csync" ::: "memory");
    *(volatile uint16_t *)0x12E00u &= ~0x800u;
}
