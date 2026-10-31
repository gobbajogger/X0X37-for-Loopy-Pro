// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Bluetooth controller image for the X0X SMK-37 (core 1): shared definitions of the shim.
 * Written for this project. The prototypes of the functions that the JieLi libraries call
 * follow the JieLi AC79 SDK headers (Apache-2.0).
 *
 * How the image runs (see ble/README.md):
 *   - The app starts core 1 with its own idle loop. It hands the image's init() to core 1 once,
 *     as a job, at power-on, before the audio starts. init() sets up the controller, routes the
 *     BLE interrupts to core 1 and runs the controller until it settles (the RF trim).
 *   - After that, core 1's idle loop calls the image's poll() between the audio jobs. poll() runs
 *     the timers, the host mailbox and the cooperative tasks once, on its own stack, and returns.
 *   - The BLE interrupts run on core 1 at any time (also inside an audio job), on the image's own
 *     interrupt stack, except while the app parks core 1 for a flash write.
 */
#ifndef CTL_PORT_H
#define CTL_PORT_H
#include <stdint.h>
#include <stddef.h>

#define XB_SYNC() __asm__ volatile("csync" ::: "memory")
#include "x0x_ble_abi.h"

/* libc (newlib from the SDK) */
void *memcpy(void *d, const void *s, size_t n);
void *memset(void *d, int c, size_t n);
int strcmp(const char *a, const char *b);
void *malloc(size_t n);
void free(void *p);

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;

#define CTL_RAMTEXT __attribute__((section(".ctl_ram_text"), noinline))

/* OS return codes, values from include_lib/system/os/os_error.h */
#define OS_NO_ERR         0
#define OS_TIMEOUT        11
#define OS_TASKQ          13
#define OS_TASK_NOT_EXIST 14
#define OS_Q_FULL         21
#define OS_Q_EMPTY        22
#define OS_TICKS_PER_SEC  100

extern const xb_boot_t *ctl_bp;
extern xb_mbox_t *ctl_mb;

/* Scheduler and timer internals */
void ctl_os_init(void);
int  ctl_os_run_once(void);       /* one pass: timers, host mailbox, each runnable task; 1 = a task ran */
void ctl_timer_poll(void);
u32  ctl_ms(void);
void ctl_ms_init(void);
void ctl_host_poll(void);
void ctl_heap_init(void);
void ctl_port_irq_reroute(u8 prio);   /* core 1's interrupt bank again (first poll) */
#define CTL_BLE_IRQ_PRIO 7             /* felucca-bsp #2 finding: BLE IRQs need priority 7 */
void ctl_fail(u32 err);           /* the controller stops for good: state FAILED */
u32 ctl_heap_stat(void);          /* heap peak and failures, for DIAG (ctl_heap.c) */

/* ---- Port layer (pi32v2, core 1). ctl_port.c and ctl_port_asm.S ---- */
struct ctl_ctx { void *sp; };
void ctl_port_ctx_init(struct ctl_ctx *c, void *stack, u32 size, void (*entry)(void));
void ctl_port_ctx_switch(struct ctl_ctx *from, struct ctl_ctx *to);
u32  ctl_port_irq_save(void);
void ctl_port_irq_restore(u32 flags);
int  ctl_port_in_irq(void);
int  ctl_port_irq_disabled(void);
void ctl_port_request_irq(u8 index, u8 prio, void (*handler)(void));
void ctl_port_tick_start(u32 hz);
void ctl_port_wdt_feed(void);
void ctl_port_wait_for_irq(void);
void ctl_port_cpu_relax(void);

/* assembly helpers (ctl_port_asm.S) */
void ctl_asm_ctx_switch(void **from_sp, void *const *to_sp);
u32  ctl_asm_stack_call(u32 (*fn)(u32), u32 arg, void *stack_top);
u32  ctl_asm_sp(void);
u32  ctl_asm_guard_call(u32 (*fn)(u32), u32 arg, void *stack_top, void **saved_sp);
void ctl_asm_unwind(void *saved_sp, u32 rc) __attribute__((noreturn));

#endif
