/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_FULL_MG_H
#define ORIGINAL_CHICAGO_FULL_MG_H
#include <stdint.h>
/* Raw32 fields preserve exact load/store bits, including full32 noise words.
 * No float calculation or clock/RNG advancement is performed by this helper. */
typedef struct {uint32_t phase,unipolar,bipolar,hold,noise,wrapped;} original_chicago_mg_voice_bits;
typedef struct {uint32_t phase,sample,hold,noise,wrapped;} original_chicago_mg_global_bits;
typedef struct {
 unsigned voice_id,head_id;
 uint8_t key_sync[2],shape[2];
 original_chicago_mg_voice_bits current[2],head[2];
 original_chicago_mg_global_bits global[2];
} original_chicago_full_mg_input;
typedef struct {
 original_chicago_mg_voice_bits voice[2];
 original_chicago_mg_global_bits global[2];
 uint8_t reset_global_mask,seed_core_counter,reevaluate_voice_mask;
} original_chicago_full_mg_output;
/* Selected81/all22 domain: both MG shapes remain triangle0, primary45/51
 * key-sync choices are explicit inputs. Full current=head uses GLOBAL phases;
 * key-sync1 directly resets global phase−1/wrap1 then evaluates56e60/56f38:
 * triangle sample+0, wrap0, original hold/noise preserved. Voice wrap becomes1.
 * Current!=head copies head phase/hold/noise only, preserving own cached
 * outputs/wrap. Both voice evaluators must subsequently run after full reset;
 * frozen voice_full/immediate_raw/resume already do that. Apply MG1 thenMG2
 * global/voice writes at their original producer points after masked pitch
 * writes, before filter/EG/stereo reset and forced57010. For current=head,
 * seed coreb4cc=Fs after common full reset and before forced helper.
 *
 * Direct initializer reset temporarily sets globalphase−1/wrap1, then its
 * evaluator clearswrap0. Ordinary sync notification55e64/55f84 ALSO ends at
 * phase−1/wrap0, but owns separate tempo accumulator/shape notification work.
 * This helper does not tick that notification clock, advance globalstage,
 * or create an extra+690 entry. No ctor/global/head seed is invented.
 * Caller supplies the live ordered head, including a newly allocated record
 * or retained pending record, and snapshots taken at the actual initializer.
 * A missing/invalid head or nonselected waveform returns0 unchanged. */
int original_chicago_full_mg_resolve(const original_chicago_full_mg_input *input,original_chicago_full_mg_output *out);
#endif
