/* SPDX-License-Identifier: GPL-3.0-only */
/* Instrumentation only: unchanged production engine is included directly. */
#define engine_bass_is_wobble wheel_real_engine_bass_is_wobble
#include WHEEL_ENGINE_SOURCE
#undef engine_bass_is_wobble
extern void wheel_probe_bank_query_hook(void);
int engine_bass_is_wobble(int voice)
{
    int result=wheel_real_engine_bass_is_wobble(voice);
    wheel_probe_bank_query_hook();
    return result;
}
unsigned wheel_probe_queue(void){return cq_w-cq_r;}
unsigned wheel_probe_owners(void){return bass_wheel_mask;}
unsigned wheel_probe_revision(int voice)
{
    return original_bass[voice].kind==ORIGINAL_SOUND_MIAMI?
        original_bass[voice].dsp.miami.program.revision:original_bass[voice].dsp.chicago.program.revision;
}
static uint32_t wheel_bits(float f){uint32_t u;__builtin_memcpy(&u,&f,4);return u;}
uint32_t wheel_probe_native105_bits(int voice)
{
    float f=-99.0f;
    if(original_bass[voice].kind==ORIGINAL_SOUND_MIAMI)
        (void)original_miami_program_native(&original_bass[voice].dsp.miami.program,105,&f);
    else (void)original_chicago_program_native(&original_bass[voice].dsp.chicago.program,105,&f);
    return wheel_bits(f);
}
uint32_t wheel_probe_bipolar_bits(int voice)
{
    return wheel_bits(original_bass[voice].kind==ORIGINAL_SOUND_MIAMI?
        2.0f*original_bass[voice].dsp.miami.controls.bend_normalized-1.0f:
        original_bass[voice].dsp.chicago.controls.routing.bend_bipolar);
}

/* Invoke real production motion boundaries without a synthetic sequencer. */
void wheel_probe_motion_step(int voice,int pat,int step)
{motion_step(&mot,PART_303A+voice,pat,step,(step+1)%16,16);}
void wheel_probe_motion_tick(unsigned n){motion_tick(&mot,n);}
void wheel_probe_motion_release(void){motion_release(&mot);}
