/* SPDX-License-Identifier: GPL-3.0-only */
/* Box fixture: direct idle advance must retain every 909 state byte and restart. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "dsp/drum909.h"

static drum909_t reference, candidate;
static float l[256], r[256], rev[256], dly[256];
static float cl[256], cr[256], cv[256], cd[256];

static void clear_audio(void)
{
    memset(l, 0, sizeof l); memset(r, 0, sizeof r);
    memset(rev, 0, sizeof rev); memset(dly, 0, sizeof dly);
    memset(cl, 0, sizeof cl); memset(cr, 0, sizeof cr);
    memset(cv, 0, sizeof cv); memset(cd, 0, sizeof cd);
}

int main(void)
{
    static const int sizes[] = {0, 1, 63, 64, 65, 127, 128, 255, 256, 257};
    for (int voice = 0; voice < DR_NUM; ++voice)
    for (int played = 0; played < 2; ++played) {
        memset(&reference, 0, sizeof reference);
        memset(&candidate, 0, sizeof candidate);
        drum909_init(&reference); drum909_init(&candidate);
        if (played) {
            drum909_trigger(&reference, voice, 0.8f);
            drum909_trigger(&candidate, voice, 0.8f);
            for (int block = 0; block < 16384 && drum909_active(&reference); ++block) {
                clear_audio();
                drum909_render_st(&reference, l, r, rev, dly, 256);
                drum909_render_st(&candidate, cl, cr, cv, cd, 256);
                assert(!memcmp(&reference, &candidate, sizeof reference));
            }
            assert(!drum909_active(&reference) && !drum909_active(&candidate));
        }
        for (int block = 0; block < 4000; ++block) {
            int n = sizes[block % (sizeof sizes / sizeof sizes[0])];
            assert(!drum909_active(&reference));
            clear_audio();
            drum909_render(&reference, l, rev, dly, n);
            drum909_advance_noise(&candidate, n);
            assert(!memcmp(&reference, &candidate, sizeof reference));
            for (int i = 0; i < 256; ++i)
                assert(l[i] == 0.0f && rev[i] == 0.0f && dly[i] == 0.0f);
        }
        drum909_trigger(&reference, voice, 0.8f);
        drum909_trigger(&candidate, voice, 0.8f);
        for (int block = 0; block < 64; ++block) {
            int n = sizes[1 + block % 8];
            clear_audio();
            drum909_render_st(&reference, l, r, rev, dly, n);
            drum909_render_st(&candidate, cl, cr, cv, cd, n);
            assert(!memcmp(l, cl, sizeof l) && !memcmp(r, cr, sizeof r));
            assert(!memcmp(rev, cv, sizeof rev) && !memcmp(dly, cd, sizeof dly));
            assert(!memcmp(&reference, &candidate, sizeof reference));
        }
    }
    return 0;
}
