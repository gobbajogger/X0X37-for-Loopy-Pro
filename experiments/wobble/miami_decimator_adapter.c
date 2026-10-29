/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_decimator_adapter.h"
#include "miami_decimator_tables_private.h"
int miami_decimator_adapter_init(miami_decimator_adapter_t *s)
{
    if (!s) return 0;
    miami_decimator_adapter_t next = {0};
    (void)miami_decimator_seed(&next.kernel,0,0,0,0);
    next.frequency_integer = 64;
    next.wet_integer = 127;
    next.wet_gain = miami_decimator_wet[127];
    (void)miami_decimator_stage_step(&next.kernel,miami_decimator_frequency[63]);
    *s = next;
    return 1;
}
int miami_decimator_adapter_reset(miami_decimator_adapter_t *s)
{
    if (!s) return 0;
    s->kernel.phase = 0;
    s->kernel.held_left = s->kernel.held_right = 0;
    return 1;
}
int miami_decimator_adapter_set(miami_decimator_adapter_t *s, unsigned parameter, int32_t value)
{
    if (!s || parameter < 1 || parameter > 3) return 0;
    uint8_t p = (uint8_t)((uint32_t)value & UINT32_C(127));
    if (parameter == 2) {
        s->wet_integer = p;
        s->wet_gain = miami_decimator_wet[p];
    } else {
        if (parameter == 1) s->frequency_integer = p;
        else s->bit_integer = p;
        (void)miami_decimator_stage_step(&s->kernel,miami_decimator_frequency[127-s->frequency_integer]);
    }
    return 1;
}
int miami_decimator_adapter_process(miami_decimator_adapter_t *s, int16_t *left,
                                    int16_t *right, uint32_t frames,
                                    const uint32_t *reset_frames, uint32_t reset_count)
{
    if (!s || s->wet_gain < 0 || (frames && (!left || !right || left == right)) ||
        (reset_count && !reset_frames)) return 0;
    for (uint32_t i=0;i<reset_count;++i)
        if (reset_frames[i]>=frames || (i && reset_frames[i]<reset_frames[i-1])) return 0;
    /* A constructor's pending state must survive an empty invocation. */
    if (!frames) return 1;
    uint32_t event=0;
    for (uint32_t i=0;i<frames;++i) {
        while (event<reset_count && reset_frames[event]==i) {
            (void)miami_decimator_adapter_reset(s);
            ++event;
        }
        int16_t out[2];
        (void)miami_decimator_frame(&s->kernel,i,left[i],right[i],s->wet_gain,out);
        left[i]=out[0];right[i]=out[1];
    }
    return 1;
}
