/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_MP4_H
#define ORIGINAL_CHICAGO_MP4_H
#include "miami_ms20.h"
typedef struct {float history[5];original_ramp a,e,resonance;uint8_t prepared;} original_chicago_mp4;
/* 16385 authentic generated entries for the explicitly selected Fs.
 * Transferred tables are derived libm results, not live plugin-memory proof. */
typedef struct {const float *a,*e;} original_mp4_tables;
/* Alternate exact-bit table provider; enables lossless storage without changing
 * cutoff indexing or filter arithmetic. Callback returns finite A/E for index. */
typedef int (*original_mp4_reader)(const void *context,uint32_t index,float *a,float *e);
int original_chicago_mp4_prepare_reader(original_chicago_mp4 *s,original_mp4_reader read,const void *context,
 float cutoff,float env_depth,float env,float mod_depth,float mod_cutoff,
 int note,float keyboard_track,float resonance,float mod_resonance,uint32_t duration,
 unsigned kick,float *postgain);
int original_chicago_mp4_init(original_chicago_mp4 *s,float a0,float e0);
/* kick is the ORIGINAL caller initialization flag, not every control tick.
 * postgain is a factor for caller post-filter gain preparation, not R feedback.
 * No complete note/ARP/voice allocator, Bite or pan integration is implied. */
int original_chicago_mp4_prepare(original_chicago_mp4 *s,const original_mp4_tables *tables,
 float cutoff,float env_depth,float env,float mod_depth,float mod_cutoff,
 int note,float keyboard_track,float resonance,float mod_resonance,uint32_t duration,
 unsigned kick,float *postgain);
/* Input already includes original oscillator/noise and VCO input gain. */
int original_chicago_mp4_sample(original_chicago_mp4 *s,float input,float *out);
int original_chicago_feedback(float input,float *out);
#endif
