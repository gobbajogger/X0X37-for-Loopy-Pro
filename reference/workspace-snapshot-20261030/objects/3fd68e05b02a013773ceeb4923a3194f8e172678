/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the production sequencer test sink for a bounded NOTE/TIE check. */
#define main original_seq_test_main
#include "seq_test.c"
#undef main
int main(void)
{
    int b, s;
    test_303(); /* NOTE -> held TIE -> REST -> orphan TIE -> NOTE */
    reset();
    for (b = 0; b < NBASS; ++b)
        for (s = 0; s < 16; ++s) P[0].bass[b].step[s].flags = G_TIE;
    seq_start(&S);
    run(44100u * 4u);
    CHECK(find(E_ON, -1, 0) < 0, "all-TIE lines must not trigger bass notes");
    printf("all-TIE, both303s,2bars: bass-on events=%d\n", find(E_ON, -1, 0) < 0 ? 0 : 1);
    reset();
    for (b = 0; b < NBASS; ++b) P[0].bass[b].step[4].flags = G_TIE;
    seq_start(&S);
    run(44100u * 4u);
    CHECK(find(E_ON, -1, 0) < 0, "isolated orphan TIE must not trigger bass notes");
    printf("isolated orphan TIE,both303s,2bars: bass-on events=%d\n", find(E_ON, -1, 0) < 0 ? 0 : 1);
    reset(); S.bpm = 120.0f;
    for (b = 0; b < NBASS; ++b) {
        P[0].bass[b].len = 64;
        P[0].bass[b].step[63].flags = G_NOTE; P[0].bass[b].step[63].note = 48;
        P[0].bass[b].step[0].flags = P[0].bass[b].step[1].flags = G_TIE;
    }
    pattern_t before[NPAT]; memcpy(before, P, sizeof before);
    seq_start(&S); run(44100u * 9u);
    for (b = 0; b < NBASS; ++b) {
        int on = find(E_ON, b, 0), off = on < 0 ? -1 : find(E_OFF, b, on + 1);
        CHECK(on >= 0 && off >= 0, "wrap hold needs a real onset and release for bass %d", b);
        if (on >= 0 && off >= 0)
            CHECK(abs((int)(evs[off].t - evs[on].t) - 13781) <= 1,
                  "wrap ties sustain to the last tie half-gate (2.5 steps) for bass %d", b);
        CHECK(on >= 0 && find(E_ON, b, on + 1) < 0, "ties must not retrigger bass %d", b);
    }
    CHECK(!memcmp(before, P, sizeof before), "sequencer must not mutate stored note/tie data");
    printf("wrap-held NOTE positive, both303s: onset/sustain/release and data preservation checked\n");
    printf("TIE semantics %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
