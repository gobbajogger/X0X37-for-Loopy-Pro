/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_PROGRAM_OUTPUT_H
#define ORIGINAL_CHICAGO_PROGRAM_OUTPUT_H
#include "chicago_program.h"
#include "chicago_output.h"
/* Sequential event/render owner. This callback is total for the canonical
 * type/switch/Edit1/Edit2 values and must update the actual original FX2
 * wrapper. It must not substitute another algorithm for an unsupported type.
 * The caller owns all FX2 state, history, queues and type transitions. */
typedef void (*original_chicago_fx_parameter)(void *context,unsigned parameter,unsigned value);
int original_chicago_output_init_routing(original_chicago_output *s,const original_chicago_routing *routing,original_chicago_fx_parameter fx2_set,void *context);
/* Run after compiling a successful public edit. It applies only the output
 * targets owned by that macro:5->DriveEdit1,17/18/19->FX2type/Edit1/Edit2.
 * Other macros remain owned by the program/voice/ARP paths; master_squared
 * from the SAME routing snapshot is supplied to output_process. This does
 * not invent a host queue, complete Chicago voice or missing FX2 processor. */
int original_chicago_output_apply_routing(original_chicago_output *s,const original_chicago_routing *routing,unsigned changed_macro,original_chicago_fx_parameter fx2_set,void *context);
#endif
