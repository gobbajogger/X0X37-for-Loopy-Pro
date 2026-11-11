/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* IRQ entry wrappers: save what a C call may clobber, call the C body,
 * restore, csync, rti. clang does not emit the csync needed before rti,
 * so ISRs are not written in C directly. */
	.macro FM1_ISR name, body
	.section .text.\name,"ax",@progbits
	.globl \name
\name:
	[--sp] = {psr, rets, reti}
	[--sp] = {r3-r0}
	call \body
	{r3-r0} = [sp++]
	{psr, rets, reti} = [sp++]
	csync
	rti
	.endm

/* timer 5: as FM1_ISR, and its sp after the pushes goes to fm1_t5_sp (fm1_guard.h, the frame watch) */
	.section .text.isr_timer5,"ax",@progbits
	.globl isr_timer5
isr_timer5:
	[--sp] = {psr, rets, reti}
	[--sp] = {r3-r0}
	r0 = fm1_t5_sp
	r1 = sp
	[r0 + 0] = r1
	call fm1_timer5_irq
	{r3-r0} = [sp++]
	{psr, rets, reti} = [sp++]
	csync
	rti

	FM1_ISR isr_alnk0, fm1_alnk0_irq
