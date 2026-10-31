/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MASTER_H
#define ORIGINAL_MASTER_H
#include <stdint.h>
typedef struct {float current,target,step;uint8_t committed_mute,desired_mute;} original_master;
/* Core constructor state. Whole native VST initialization additionally runs
 * its real reset/restore/warmup sequence; this is not that outer lifecycle. */
void original_master_init(original_master *s);
void original_master_mute(original_master *s,uint8_t byte);
/* volume is already converted native96 = normalized96 squared. Do not square
 * it again. Empty calls preserve every byte. In-place matching channels okay;
 * cross-channel/state/buffer overlap is unsupported. Core-only reset preserves
 * this object. State is exclusively owned by the render/control executor. */
int original_master_process(original_master *s,float converted_volume,const uint32_t *in_left,const uint32_t *in_right,uint32_t *out_left,uint32_t *out_right,uint32_t frames);
/* The same as writing miami_fx_output_rne((int16_t)in[i]) to out, then
 * original_master_process in place: one pass. i32: the low 16 bits of each word. */
int original_master_process_i16(original_master *s,float converted_volume,const int16_t *in_left,const int16_t *in_right,uint32_t *out_left,uint32_t *out_right,uint32_t frames);
int original_master_process_i32(original_master *s,float converted_volume,const int32_t *in_left,const int32_t *in_right,uint32_t *out_left,uint32_t *out_right,uint32_t frames);
#endif
