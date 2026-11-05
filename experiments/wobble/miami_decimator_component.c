/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_decimator_component.h"
/* Portable arithmetic right shift15: C signed negative >> is implementation-defined.
 * Products here are within [-1073709056,1073676289]; negation/addition fit int32. */
static int32_t floor_q15(int32_t product)
{
    if (product >= 0) return product / 32768;
    return -((-product + 32767) / 32768);
}
static int16_t mix(int16_t held, int16_t input, int32_t wet)
{
    int32_t dry = 32767 - wet;
    /* Do NOT combine products before shifting: original rounds separately. */
    int32_t value = floor_q15((int32_t)held * wet) + floor_q15((int32_t)input * dry);
    if (value < -32767) value = -32767;
    if (value > 32767) value = 32767;
    return (int16_t)value;
}
int miami_decimator_seed(miami_decimator_component_t *s, uint32_t phase,
                         int32_t left, int32_t right, uint32_t step)
{
    if (!s || phase > 65535 || step > 65535 || left < -32767 || left > 32767 ||
        right < -32767 || right > 32767) return 0;
    miami_decimator_component_t next = {0};
    next.phase = (uint16_t)phase;
    next.step = next.pending_step = (uint16_t)step;
    next.held_left = (int16_t)left;
    next.held_right = (int16_t)right;
    *s = next;
    return 1;
}
int miami_decimator_stage_step(miami_decimator_component_t *s, uint32_t step)
{
    if (!s || step > 65535) return 0;
    s->pending_step = (uint16_t)step;
    s->pending = 1;
    return 1;
}
int miami_decimator_advance_zero(miami_decimator_component_t *s,uint32_t frames)
{
    if (!s || s->held_left || s->held_right) return 0;
    if (!frames) return 1;
    if (s->pending) { s->step=s->pending_step; s->pending=0; }
    s->phase=(uint16_t)((uint32_t)s->phase+(uint32_t)s->step*frames);
    return 1;
}
int miami_decimator_frame(miami_decimator_component_t *s, uint64_t local_index,
                          int16_t left, int16_t right, int32_t wet, int16_t out[2])
{
    if (!s || !out || wet < 0 || wet > 32767) return 0;
    if ((local_index & UINT64_C(63)) == 0 && s->pending) {
        s->step = s->pending_step;
        s->pending = 0;
    }
    uint32_t sum = (uint32_t)s->phase + s->step;
    s->phase = (uint16_t)sum;
    if (sum & UINT32_C(65536)) {
        s->held_left = left == INT16_MIN ? -32767 : left;
        s->held_right = right == INT16_MIN ? -32767 : right;
    }
    out[0] = mix(s->held_left, left, wet);
    out[1] = mix(s->held_right, right, wet);
    return 1;
}
