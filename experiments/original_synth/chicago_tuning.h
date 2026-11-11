/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_GLOBAL_TUNING_H
#define ORIGINAL_CHICAGO_GLOBAL_TUNING_H
#include <stdint.h>
typedef struct {const float *custom_offsets;uint32_t ready;uint16_t random;uint8_t scale,tonic;} original_chicago_tuning;
/* One caller-owned shared RNG corresponds to original process-global2c55a0.
 * Seed0x6d70 ONCE at its actual lifetime, not on each note/voice/preset reset.
 * Converted secondary1/2 choose scale0..15 andtonic0..11. Optional144-entry
 * custom table is borrowed immutable and validated once; caller must keep
 * lifetime/ownership or rebind on change. const alone does not guarantee it. */
int original_chicago_tuning_bind(original_chicago_tuning *out,unsigned scale,unsigned tonic,const float *custom_offsets,uint16_t random_state);
/* A088/B200 use this once at each actual immediate initializer. Never call it
 * for descriptor staging. Mode0 adds+0 without consuming RNG; mode1 advances
 * shared signed16 generator once; modes2/3 retain original signed remainder
 * including negative table aliases; modes4..15 use original custom offsets. */
int original_chicago_tuning_target(original_chicago_tuning *s,int32_t raw_note,float *tuned_pitch);
/* Pure secondary0 converter55168. logarithm is an explicit binary32 logf
 * provider; host libm equality is not proof of another platform's libm bits.
 * This does NOT assert where the converted MasterTune is audibly consumed. */
typedef float (*original_chicago_logf_provider)(float);
int original_chicago_master_tune_convert(float normalized,original_chicago_logf_provider logarithm,float *converted);
#endif
