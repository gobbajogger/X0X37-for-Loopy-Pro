/* SPDX-License-Identifier: GPL-3.0-only
 * Focused freestanding String Machine regression. Compile without any DSP
 * objects: the device and host share the exact header implementation. */
#include "../../firmware/src/dsp/drone_string.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void render_blocks(drone_string_t *d, unsigned blocks, float *peak)
{
    float l[256], r[256];
    for (unsigned b = 0; b < blocks; ++b) {
        memset(l, 0, sizeof l);
        memset(r, 0, sizeof r);
        assert(drone_string_render(d, l, r, 256));
        for (unsigned i = 0; i < 256; ++i) {
            assert(isfinite(l[i]) && isfinite(r[i]));
            assert(l[i] > -1.0f && l[i] < 1.0f);
            assert(r[i] > -1.0f && r[i] < 1.0f);
            float a = fm_fabsf(l[i]) + fm_fabsf(r[i]);
            if (a > *peak) *peak = a;
        }
    }
}

static unsigned active_voices(const drone_string_t *d)
{
    unsigned n = 0;
    for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i)
        n += d->voice[i].stage != 0;
    return n;
}

int main(void)
{
    drone_string_t a, b;
    drone_string_init(&a);
    assert(sizeof a < 1024u);
    assert(!drone_string_active(&a));
    assert(a.macro[6] == 0.5f && !a.damper);
    assert(!drone_string_render(&a, 0, 0, 0));
    assert(!drone_string_render(&a, (float[256]){0}, (float[256]){0}, 257));
    assert(!drone_string_set(&a, 8, .5f));
    assert(!drone_string_set(&a, 1, -1.0f));
    assert(!drone_string_note(&a, 128, 100));
    assert(!drone_string_note(&a, 60, 128));
    assert(!drone_string_active(&a));

    float l[258] = {0}, r[258] = {0};
    l[0] = r[0] = 12345.0f;
    l[257] = r[257] = -12345.0f;
    assert(drone_string_note(&a, 60, 115));
    assert(drone_string_active(&a));
    assert(drone_string_render(&a, &l[1], &r[1], 256));
    assert(l[0] == 12345.0f && r[0] == 12345.0f);
    assert(l[257] == -12345.0f && r[257] == -12345.0f);
    float peak = 0.0f;
    for (unsigned i = 1; i <= 256; ++i) {
        assert(isfinite(l[i]) && isfinite(r[i]));
        float abs = fm_fabsf(l[i]) + fm_fabsf(r[i]);
        if (abs > peak) peak = abs;
    }
    assert(peak > .00001f); /* output starts in the existing first audio block */
    assert(drone_string_note(&a, 60, 0));
    assert(a.voice[0].stage == DRONE_RELEASE);
    render_blocks(&a, 400, &peak);
    assert(!drone_string_active(&a)); /* release bounded, not a stuck drone */

    drone_string_init(&a);
    assert(drone_string_set(&a, 7, 1.0f));
    assert(drone_string_note(&a, 48, 100));
    assert(drone_string_note(&a, 48, 0));
    assert(drone_string_active(&a)); /* damper holds the released key */
    assert(drone_string_set(&a, 7, 0.0f));
    render_blocks(&a, 400, &peak);
    assert(!drone_string_active(&a));

    drone_string_init(&a);
    const unsigned chord[] = {48, 55, 60, 67};
    for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i)
        assert(drone_string_note(&a, chord[i], 127));
    assert(active_voices(&a) == DRONE_STRING_VOICES);
    assert(drone_string_note(&a, 72, 127)); /* bounded voice stealing */
    assert(active_voices(&a) == DRONE_STRING_VOICES);
    float old_step = a.voice[0].step;
    assert(drone_string_set(&a, 6, .75f));
    assert(a.voice[0].step != old_step);
    assert(drone_string_set(&a, 6, .5f));
    assert(drone_string_set(&a, 0, 1.0f));
    assert(drone_string_set(&a, 1, 1.0f));
    assert(drone_string_set(&a, 2, 1.0f));
    render_blocks(&a, 500, &peak);
    assert(peak > .001f);
    drone_string_all_off(&a);
    assert(!drone_string_active(&a) && !a.damper);

    /* Exact deterministic samples across fresh instances, including flutter. */
    drone_string_init(&a);
    drone_string_init(&b);
    assert(drone_string_note(&a, 62, 101));
    assert(drone_string_note(&b, 62, 101));
    float al[256] = {0}, ar[256] = {0}, bl[256] = {0}, br[256] = {0};
    for (unsigned block = 0; block < 16; ++block) {
        memset(al, 0, sizeof al); memset(ar, 0, sizeof ar);
        memset(bl, 0, sizeof bl); memset(br, 0, sizeof br);
        assert(drone_string_render(&a, al, ar, 256));
        assert(drone_string_render(&b, bl, br, 256));
        assert(!memcmp(al, bl, sizeof al) && !memcmp(ar, br, sizeof ar));
    }

    printf("drone-string: PASS, voices=%u, state=%zu bytes, peak=%.6f\n",
           DRONE_STRING_VOICES, sizeof a, (double)peak);
    return 0;
}
