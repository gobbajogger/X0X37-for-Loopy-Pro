/* SPDX-License-Identifier: GPL-3.0-only */
/* Host meter path: Core 1 work is counted, the C1 join is not billed to PR_808,
 * audio stays bit-exact. Built with X0X_CORE_PLAN=(1u<<JB_A) so Miami is on C1. */
#define AUDIO_PATH_IDENTITY_LIBRARY
#include "audio_path_identity.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static uint64_t nsec(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

static void adopt_miami(void)
{
    int32_t out[512];
    int i;
    boot();
    engine_guard_enable(0);
    engine_usb_full(0);
    seq_stop(&seq);
    assert(plat_cpu2_set(1) == 0);
    assert(engine_bass_select_preset(0, FACTORY_BANK_MIAMI, 0, &proj.sound) >= 0);
    for (i = 0; i < 400 && engine_bass_bank(0) != FACTORY_BANK_MIAMI; ++i)
        engine_render(out, 256);
    assert(engine_bass_bank(0) == FACTORY_BANK_MIAMI);
}

static void render_blocks(int32_t *out, int n, int *heard)
{
    int i, s;
    for (i = 0; i < n; ++i) {
        engine_render(out, 256);
        if (heard)
            for (s = 0; s < 512; ++s)
                if (out[s] != 0)
                    *heard = 1;
    }
}

static void play_scene(int32_t *pcm32)
{
    int32_t out[512];
    int i;
    adopt_miami();
    render_blocks(out, 16, 0);
    engine_bass_on(0, 48, 0, 0);
    render_blocks(out, 16, 0);
    for (i = 0; i < 32; ++i) {
        engine_render(out, 256);
        memcpy(pcm32 + i * 512, out, sizeof out);
    }
}

static int pcm_equal(const int32_t *a, const int32_t *b, int n)
{
    int i;
    for (i = 0; i < n; ++i)
        if (a[i] != b[i])
            return 0;
    return 1;
}

int main(void)
{
    int32_t out[512], first[512 * 32], second[512 * 32];
    uint32_t before[ENG_PROF_N], delta[ENG_PROF_N];
    uint32_t wait0, wait1;
    int i, heard = 0, fail = 0;
    unsigned k;

    adopt_miami();
    render_blocks(out, 16, 0);
    engine_bass_on(0, 48, 0, 0);
    render_blocks(out, 16, 0);

    for (k = 0; k < (unsigned)ENG_PROF_N; ++k)
        before[k] = eng_prof[k];
    wait0 = eng_prof_wait;
    for (i = 0; i < 32; ++i) {
        engine_render(out, 256);
        memcpy(first + i * 512, out, sizeof out);
        for (int s = 0; s < 512; ++s)
            if (out[s] != 0)
                heard = 1;
    }
    for (k = 0; k < (unsigned)ENG_PROF_N; ++k)
        delta[k] = eng_prof[k] - before[k];
    wait1 = eng_prof_wait - wait0;

    printf("heard=%d PR_909=%u PR_808=%u PR_303A=%u PR_303B=%u PR_BRK=%u PR_FX=%u PR_MST=%u PR_SEQ=%u wait=%u\n",
           heard, delta[PR_909], delta[PR_808], delta[PR_303A], delta[PR_303B],
           delta[PR_BRK], delta[PR_FX], delta[PR_MST], delta[PR_SEQ], wait1);

    if (!heard) {
        fprintf(stderr, "FAIL: Miami produced silence\n");
        fail = 1;
    }
    if (delta[PR_303A] == 0) {
        fprintf(stderr, "FAIL: PR_303A is 0 while Miami is playing\n");
        fail = 1;
    }
    /* Silent 808 must not include the C1 join or Miami takeover. */
    if (delta[PR_808] * 4u >= delta[PR_303A]) {
        fprintf(stderr, "FAIL: PR_808=%u is billed with a silent 808 (PR_303A=%u); join wait must not go to 808\n",
                delta[PR_808], delta[PR_303A]);
        fail = 1;
    }

    /* Fresh boot, same scene: PCM is bit-exact (meter counters are not audio). */
    play_scene(second);
    if (!pcm_equal(first, second, 512 * 32)) {
        fprintf(stderr, "FAIL: PCM differs across two fresh-boot Miami runs\n");
        fail = 1;
    } else
        printf("pcm_repeat=bit-exact\n");
    if (getenv("X0X_PCM_DUMP")) {
        FILE *f = fopen(getenv("X0X_PCM_DUMP"), "wb");
        if (!f || fwrite(first, sizeof first[0], (size_t)(512 * 32), f) != (size_t)(512 * 32) || fclose(f)) {
            fprintf(stderr, "FAIL: could not write X0X_PCM_DUMP\n");
            fail = 1;
        } else
            printf("pcm_dump=%s\n", getenv("X0X_PCM_DUMP"));
    }

    /* Fold: a private C1 credit is added on C0 after the join, then cleared. */
    {
        uint32_t p0, p1, left;
        engine_prof_c1_fold_now(0);
        engine_prof_c1_credit(PR_303A, 50000u);
        if (engine_prof_c1_peek(PR_303A) != 50000u) {
            fprintf(stderr, "FAIL: credit did not land in eng_prof_c1\n");
            fail = 1;
        }
        p0 = eng_prof[PR_303A];
        engine_render(out, 256);
        p1 = eng_prof[PR_303A];
        left = engine_prof_c1_peek(PR_303A);
        if (p1 - p0 < 50000u) {
            fprintf(stderr, "FAIL: fold after plat_cpu2_wait did not add C1 credit (d=%u)\n",
                    p1 - p0);
            fail = 1;
        } else if (left != 0) {
            fprintf(stderr, "FAIL: eng_prof_c1 was not cleared after fold (left=%u)\n", left);
            fail = 1;
        } else
            printf("fold_credit=ok added=%u\n", p1 - p0);

        /* Direct fold cost: the ISR extra work (8 adds + 8 clears + a barrier). */
        {
            const int reps = 1000000;
            uint64_t t0, t1, ns;
            int r;
            engine_prof_c1_fold_now(0);
            t0 = nsec();
            for (r = 0; r < reps; ++r)
                engine_prof_c1_fold_now(1);
            t1 = nsec();
            ns = (t1 - t0) / (uint64_t)reps;
            printf("fold_isr_ns_per_call=%llu (host CLOCK_MONOTONIC; device cycles not measured)\n",
                   (unsigned long long)ns);
        }

        /* Discard path: keep=0 must not add. */
        {
            uint32_t a0, a1;
            engine_prof_c1_credit(PR_303B, 777u);
            a0 = eng_prof[PR_303B];
            engine_prof_c1_fold_now(0);
            a1 = eng_prof[PR_303B];
            if (a1 != a0 || engine_prof_c1_peek(PR_303B) != 0) {
                fprintf(stderr, "FAIL: discard fold hid or kept a C1 credit\n");
                fail = 1;
            }
        }
    }

    if (fail)
        return 1;
    printf("PASS perf-meter-c1\n");
    return 0;
}
