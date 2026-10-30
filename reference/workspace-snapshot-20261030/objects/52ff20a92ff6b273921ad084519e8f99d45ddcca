/* SPDX-License-Identifier: GPL-3.0-only */
/* Deterministic exhaustive setter sweep; compare stdout byte-for-byte against
 * the baseline build with the same host compiler and floating-point flags. */
#include <stdio.h>
#include <string.h>
#include "dsp/drum909.h"
#include "dsp/fxbus.h"
static drum909_t drum;
static fxbus_t fx;
static int16_t delay[88200];
int main(void)
{
    float l[16], r[16], rev[16], dly[16], ol[16], or_[16];
    memset(&drum, 0, sizeof drum);
    memset(&fx, 0, sizeof fx);
    drum909_init(&drum);
    fxbus_init(&fx, delay, 88200);
    for (int voice = 0; voice <= DR_KIT; ++voice)
        for (int param = 0; param < drum909_nparams(voice); ++param)
            for (int value = -1; value <= 128; ++value) {
                drum909_set(&drum, voice, param, value);
                for (int v = 0; v < DR_NUM; ++v)
                    drum909_trigger(&drum, v, 0.8f);
                memset(l, 0, sizeof l); memset(r, 0, sizeof r);
                memset(rev, 0, sizeof rev); memset(dly, 0, sizeof dly);
                drum909_render_st(&drum, l, r, rev, dly, 16);
                fxbus_process_st(&fx, l, r, rev, dly, ol, or_, 16);
                if (fwrite(ol, sizeof ol, 1, stdout) != 1 ||
                    fwrite(or_, sizeof or_, 1, stdout) != 1) return 1;
            }
    for (int param = 0; param < FX_NPARAMS; ++param)
        for (int value = -1; value <= 128; ++value) {
            fxbus_set(&fx, param, value);
            drum909_trigger(&drum, DR_BD, 0.9f);
            memset(l, 0, sizeof l); memset(r, 0, sizeof r);
            memset(rev, 0, sizeof rev); memset(dly, 0, sizeof dly);
            drum909_render_st(&drum, l, r, rev, dly, 16);
            fxbus_process_st(&fx, l, r, rev, dly, ol, or_, 16);
            if (fwrite(ol, sizeof ol, 1, stdout) != 1 ||
                fwrite(or_, sizeof or_, 1, stdout) != 1) return 1;
        }
    return ferror(stdout) ? 1 : 0;
}
