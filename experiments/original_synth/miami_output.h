/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_OUTPUT_H
#define ORIGINAL_MIAMI_OUTPUT_H
#include "miami_program.h"
#include "miami_fx_chain.h"
#include "master.h"
typedef struct {original_miami_fx_chain fx;original_master master;} original_miami_output;
/* Fresh core/fixed-effect projection. No claim about complete VST type-switch
 * construction or startup warmup. Program and output are exclusively owned by
 * the same event/render executor. Original public macro10 owns both FX writes. */
void original_miami_output_init(original_miami_output *s);
int original_miami_output_apply(original_miami_output *s,const original_miami_program *program,unsigned changed_macro);
/* Selected fixed Decimator2/Bypass profile, with the original scalar conversion,
 * both non-unity wrappers, then squared native96/master lifecycle and cleanup.
 * reset_frames are explicit sorted effect-local resets before latch/sample;
 * this API does not infer host event bytes or scheduling from MIDI messages.
 * Buffers follow original_miami_fx_workspace. n0 preserves all state. */
int original_miami_output_process(original_miami_output *s,const original_miami_program *program,original_miami_fx_workspace *workspace,const uint32_t *in_left,const uint32_t *in_right,uint32_t *out_left,uint32_t *out_right,uint32_t frames,unsigned flush_to_zero,const uint32_t *reset_frames,uint32_t reset_count);
/* Backend-only seam: inputs are known +zero, nonnull buffers and nonzero size
 * already checked.
 * Returns zero without mutation unless both fixed mixers are settled and
 * Decimator holds zero. Caller then uses the ordinary output path. */
int original_miami_output_zero(original_miami_output *s,const original_miami_program *program,original_miami_fx_workspace *workspace,uint32_t *left,uint32_t *right,uint32_t frames);
#endif
