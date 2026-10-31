/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * pi32v2 helpers for the controller image. The register use follows the code that the JieLi
 * clang makes for the X0X app: r0-r3 carry the arguments and the result, r4-r15 belong to the
 * caller (a function saves them with [--sp] = {rets, r15-r4} and returns with
 * {pc, r15-r4} = [sp++]). */
	.section .text.ctl_asm,"ax",@progbits
	.balign 4

/* void ctl_asm_ctx_switch(void **from_sp, void *const *to_sp)
 * Save r4-r15 and the return address on this stack, keep sp in *from_sp, take sp from *to_sp and
 * return into the other context. A new context (ctl_port_ctx_init) has 13 words that all hold its
 * entry address, so the same return starts it. */
	.globl ctl_asm_ctx_switch
ctl_asm_ctx_switch:
	[--sp] = {rets, r15-r4}
	r2 = sp
	[r0 + 0] = r2
	r2 = [r1 + 0]
	sp = r2
	{pc, r15-r4} = [sp++]

/* u32 ctl_asm_stack_call(u32 (*fn)(u32), u32 arg, void *stack_top)
 * Call fn(arg) on another stack and come back to this one. */
	.globl ctl_asm_stack_call
ctl_asm_stack_call:
	[--sp] = {rets, r5, r4}
	r4 = sp
	r5 = r0
	r0 = r1
	sp = r2
	call r5
	sp = r4
	{pc, r5, r4} = [sp++]

/* u32 ctl_asm_guard_call(u32 (*fn)(u32), u32 arg, void *stack_top, void **saved_sp)
 * As ctl_asm_stack_call, but it saves every register of the caller and keeps that sp in
 * *saved_sp, so that ctl_asm_unwind can come back here from any depth of fn. */
	.globl ctl_asm_guard_call
ctl_asm_guard_call:
	[--sp] = {rets, r15-r4}
	r4 = sp
	[r3 + 0] = r4
	r5 = r0
	r0 = r1
	sp = r2
	call r5
	sp = r4
	{pc, r15-r4} = [sp++]

/* void ctl_asm_unwind(void *saved_sp, u32 rc): leave fn at once; ctl_asm_guard_call returns rc */
	.globl ctl_asm_unwind
ctl_asm_unwind:
	sp = r0
	r0 = r1
	{pc, r15-r4} = [sp++]

/* u32 ctl_asm_sp(void): the stack pointer now (in an interrupt: the interrupt stack) */
	.globl ctl_asm_sp
ctl_asm_sp:
	r0 = sp
	rts
