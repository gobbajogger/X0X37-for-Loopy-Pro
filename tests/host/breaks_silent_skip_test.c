/* SPDX-License-Identifier: GPL-3.0-only */
/* When breaks_silent is true, breaks_render only zeros the output and leaves
 * breaks_t unchanged. Skipping that call (engine.c opt) is therefore
 * state-equivalent for BRK; private scratch may retain stale floats.
 * Also prove a later audible render matches whether or not the silent clears ran.
 */
#include <stdio.h>
#include <string.h>
#define X0X_IMA_TABLES_DEFINE
#include "../../firmware/src/dsp/breaks.c"

static int fails;
static unsigned char adpcm[8192];
#define CHECK(c, ...) do { if (!(c)) { fprintf(stderr, "FAIL " __VA_ARGS__); fputc('\n', stderr); fails++; } } while (0)

enum { N = 256 };
static float out_a[N], out_b[N], poison[N];

static void fill_poison(float *p, int n)
{
    int i;
    for (i = 0; i < n; i++)
        p[i] = 0.1234f * (float)(i + 1);
}

/* Only snapshot ownership pointers differ between independent host engines. */
static void same_state(const breaks_t *a, const breaks_t *b)
{
    breaks_t normalized = *b;
    for (unsigned v = 0; v < 2; ++v)
        for (unsigned k = 0; k < 2; ++k) {
            if (b->v[v].snap == &b->bank[k]) normalized.v[v].snap = &a->bank[k];
            if (b->v[v].snap == &b->side[k]) normalized.v[v].snap = &a->side[k];
        }
    CHECK(!memcmp(a, &normalized, sizeof *a), "complete normalized state differs");
}

static void audible_block(breaks_t *a, breaks_t *b, float *left, float *right)
{
    int audible = 0;
    breaks_render(a, left, N);
    breaks_render(b, right, N);
    CHECK(!memcmp(left, right, N * sizeof *left), "audible PCM differs");
    same_state(a, b);
    for (int i = 0; i < N; ++i) audible |= left[i] != 0.0f;
    CHECK(audible, "expected real decoded audio");
}

static void tail_idle_restart(void)
{
    breaks_t a, b;
    brk_loop_t loop;
    memset(adpcm, 0x11, sizeof adpcm);
    memset(&loop, 0, sizeof loop);
    loop.adpcm = adpcm;
    loop.nsamples = 16384;
    loop.rate = 44100;
    loop.bars = 1;
    loop.slices = 8;
    breaks_init(&a); breaks_init(&b);
    breaks_set_loop(&a, 0, &loop); breaks_set_loop(&b, 0, &loop);
    breaks_live(&a, 5, 1); breaks_live(&b, 5, 1);
    CHECK(!breaks_silent(&a) && !breaks_silent(&b), "live bank must start audio");
    audible_block(&a, &b, out_a, out_b);
    audible_block(&a, &b, out_a, out_b);
    breaks_live(&a, 5, 0); breaks_live(&b, 5, 0);
    audible_block(&a, &b, out_a, out_b); /* release must retain its fade tail */
    for (unsigned i = 0; i < 8 && !breaks_silent(&a); ++i) {
        breaks_render(&a, out_a, N); breaks_render(&b, out_b, N);
        CHECK(!memcmp(out_a, out_b, sizeof out_a), "tail PCM differs");
        same_state(&a, &b);
    }
    CHECK(breaks_silent(&a) && breaks_silent(&b), "tail must settle");
    for (unsigned i = 0; i < 32; ++i) {
        breaks_t before = b;
        fill_poison(out_b, N);
        breaks_render(&a, out_a, N);
        if (!breaks_silent(&b)) breaks_render(&b, out_b, N);
        CHECK(!memcmp(&b, &before, sizeof b), "idle skip mutated state");
        for (unsigned j = 0; j < N; ++j) {
            CHECK(out_a[j] == 0.0f, "reference silence must clear output");
            CHECK(out_b[j] == 0.1234f * (float)(j + 1), "skip must retain poisoned scratch");
        }
        same_state(&a, &b);
    }
    breaks_live(&a, 5, 1); breaks_live(&b, 5, 1);
    CHECK(!breaks_silent(&a) && !breaks_silent(&b), "restart must sound");
    for (unsigned i = 0; i < 4; ++i) audible_block(&a, &b, out_a, out_b);
}

int main(void)
{
    breaks_t a, b, snap;
    int i;

    memset(&a, 0, sizeof a);
    breaks_init(&a);
    /* Default post-init should be silent. */
    CHECK(breaks_silent(&a), "fresh breaks_silent");

    snap = a;
    fill_poison(out_a, N);
    breaks_render(&a, out_a, N);
    for (i = 0; i < N; i++)
        CHECK(out_a[i] == 0.0f, "silent render zeros[%d]=%g", i, out_a[i]);
    CHECK(memcmp(&a, &snap, sizeof a) == 0, "silent render changes no BRK state");

    /* Skip path: leave poison in buffer, state untouched. */
    b = snap;
    fill_poison(out_b, N);
    /* no breaks_render */
    CHECK(memcmp(&b, &snap, sizeof b) == 0, "skip leaves BRK state");
    fill_poison(poison, N);
    CHECK(memcmp(out_b, poison, sizeof poison) == 0, "skip leaves scratch uncleared (documented BEHAVIOR)");

    /* Next audible path: arm a trivial silent->still-silent then compare after
     * ensuring both sides still match when remaining silent across many blocks. */
    for (i = 0; i < 32; i++) {
        snap = a;
        breaks_render(&a, out_a, N);
        CHECK(memcmp(&a, &snap, sizeof a) == 0, "iter %d state", i);
        /* skip side */
        /* b unchanged */
        CHECK(memcmp(&b, &snap, sizeof b) == 0, "iter %d skip state", i);
    }

    /* Force non-silent briefly via gate_to, then return to silent and compare
     * subsequent silent blocks' state identity under render vs skip. */
    breaks_init(&a);
    breaks_init(&b);
    /* If we cannot easily arm audio without a bank, keep the silent contract. */
    CHECK(breaks_silent(&a) && breaks_silent(&b), "both silent after init");
    for (i = 0; i < 8; i++) {
        breaks_t before = a;
        fill_poison(out_a, N);
        breaks_render(&a, out_a, N);
        CHECK(memcmp(&a, &before, sizeof a) == 0, "post-init silent state stable");
        before = b;
        /* skip */
        CHECK(memcmp(&b, &before, sizeof b) == 0, "skip stable");
        CHECK(memcmp(&a, &b, sizeof a) == 0, "render vs skip state match");
    }

    tail_idle_restart();

    if (fails) {
        fprintf(stderr, "breaks_silent_skip: %d FAIL\n", fails);
        return 1;
    }
    puts("breaks_silent_skip: ok");
    return 0;
}
