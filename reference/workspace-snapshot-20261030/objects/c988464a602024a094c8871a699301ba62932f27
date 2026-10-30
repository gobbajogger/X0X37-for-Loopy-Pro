/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Arp Switch/Mode/Range/Speed: enum surface, factory decode, held-note
 * pick + tempo clock, and honest native303 pitch change while arpeggiating.
 * Pick/rate reuse clip smk_clip_keys.h (CK_FN host-friendly). */
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CK_FN __attribute__((unused))
#include "chicago_arp.h"
#include "bass303.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static float buf[44100];

static double rms_range(const float *x, int a, int b)
{
    double s = 0;
    int i, n = b - a;
    if (n <= 0)
        return 0;
    for (i = a; i < b; i++)
        s += (double)x[i] * (double)x[i];
    return sqrt(s / (double)n);
}

/* Zero-crossing period estimate over a window (saw-friendly). */
static double period_est(const float *x, int a, int b)
{
    int i, z = 0, first = -1, last = -1;
    for (i = a + 1; i < b; i++) {
        if ((x[i - 1] < 0.0f && x[i] >= 0.0f) || (x[i - 1] >= 0.0f && x[i] < 0.0f)) {
            if (first < 0)
                first = i;
            last = i;
            z++;
        }
    }
    if (z < 2 || first < 0 || last <= first)
        return 0;
    return (double)(last - first) * 2.0 / (double)(z - 1);
}

typedef struct {
    int notes[16];
    int n;
} step_cap_t;

static void capture_step(int note, void *ctx)
{
    step_cap_t *c = ctx;
    if (c->n < 16)
        c->notes[c->n++] = note;
}

static void test_surface(void)
{
    CHECK(CHI_ARP_SWITCH == 23 && CHI_ARP_MODE == 24 && CHI_ARP_RANGE == 25 && CHI_ARP_SPEED == 26,
          "arp indices 23..26 (after ARELEASE=22)");
    CHECK(CHI_ARP_NPARAMS_END == 27, "engine nparams end 27");
    CHECK(BASS303_NPARAMS == 23 && BASS303_ARELEASE == 22, "bass303 pots 0..22");
    CHECK(CHI_ARP_PARAMS[0].def == CHI_ARP_OFF && !strcmp(CHI_ARP_PARAMS[0].name, "ArpSw"),
          "Switch def Off");
    CHECK(CHI_ARP_PARAMS[1].def == CHI_ARP_UP && !strcmp(CHI_ARP_MODE_N[CHI_ARP_UP], "Up"),
          "Mode def Up");
    CHECK(CHI_ARP_PARAMS[2].def == CHI_ARP_OCT1, "Range def 1 Oct");
    CHECK(CHI_ARP_PARAMS[3].def == CHI_ARP_SPD_1_16, "Speed def 1/16");
    CHECK(chi_arp_ticks(CHI_ARP_SPD_1_2) == 48 && chi_arp_ticks(CHI_ARP_SPD_1_16) == 6 &&
              chi_arp_ticks(CHI_ARP_SPD_1_32) == 3,
          "speed ticks via clip CK_RATE_T");
    CHECK(chi_arp_ticks(CHI_ARP_SPD_1_2) == CK_RATE_T[CHI_ARP_RATE_IX[CHI_ARP_SPD_1_2]],
          "Chicago speed aliases clip rate table");
    CHECK(chi_arp_order(CHI_ARP_UP) == CK_ORD_UP && chi_arp_order(CHI_ARP_DOWN) == CK_ORD_DOWN &&
              chi_arp_order(CHI_ARP_UPDN) == CK_ORD_UPDN,
          "Mode maps to clip CK_ORD_*");
    CHECK(BASS303_AMPMODE == 16 && BASS303_GNAW == 17 && BASS303_BITE == 18 && BASS303_AATTACK == 19,
          "do not retarget AmpMod/Gnaw/Bite/ADSR");
}

static void test_factory_decode(void)
{
    uint8_t out[4];
    {
        const uint8_t init[4] = {0, 128, 0, 213};
        chi_arp_decode_factory(init, out);
        CHECK(out[0] == CHI_ARP_OFF && out[1] == CHI_ARP_UP && out[2] == CHI_ARP_OCT1 &&
                  out[3] == CHI_ARP_SPD_1_16,
              "Init decode Off/Up/1Oct/1/16 (got %u %u %u %u)", out[0], out[1], out[2], out[3]);
    }
    {
        const uint8_t downer[4] = {255, 0, 255, 170};
        chi_arp_decode_factory(downer, out);
        CHECK(out[0] == CHI_ARP_ON && out[1] == CHI_ARP_DOWN && out[2] == CHI_ARP_FULL,
              "Downer Switch/Mode/Range");
        CHECK(out[3] == CHI_ARP_SPD_1_8, "Downer Speed 1/8 (got %u)", out[3]);
    }
    {
        const uint8_t hi[4] = {255, 255, 255, 227};
        chi_arp_decode_factory(hi, out);
        CHECK(out[0] == CHI_ARP_ON && out[1] == CHI_ARP_UPDN && out[2] == CHI_ARP_FULL &&
                  out[3] == CHI_ARP_SPD_1_16,
              "Hi-Arp On/UpDn/Full/1/16 (got %u %u %u %u)", out[0], out[1], out[2], out[3]);
    }
}

static void test_pick_modes(void)
{
    const uint8_t one[1] = {48};
    const uint8_t chord[3] = {48, 52, 55};
    int i;
    CHECK(chi_arp_pick(one, 1, CHI_ARP_UP, CHI_ARP_OCT2, 0) == 48, "Up step0");
    CHECK(chi_arp_pick(one, 1, CHI_ARP_UP, CHI_ARP_OCT2, 1) == 60, "Up step1");
    CHECK(chi_arp_pick(one, 1, CHI_ARP_UP, CHI_ARP_OCT2, 2) == 48, "Up step2");
    CHECK(chi_arp_pick(one, 1, CHI_ARP_DOWN, CHI_ARP_OCT2, 0) == 60, "Down step0");
    CHECK(chi_arp_pick(one, 1, CHI_ARP_DOWN, CHI_ARP_OCT2, 1) == 48, "Down step1");
    for (i = 0; i < 4; i++) {
        int expect = (i % 2 == 0) ? 48 : 60;
        CHECK(chi_arp_pick(one, 1, CHI_ARP_UPDN, CHI_ARP_OCT2, (uint32_t)i) == expect,
              "Up/Dn step %d", i);
    }
    CHECK(chi_arp_pick(chord, 3, CHI_ARP_UP, CHI_ARP_OCT1, 0) == 48, "chord Up 0");
    CHECK(chi_arp_pick(chord, 3, CHI_ARP_UP, CHI_ARP_OCT1, 1) == 52, "chord Up 1");
    CHECK(chi_arp_pick(chord, 3, CHI_ARP_UP, CHI_ARP_OCT1, 2) == 55, "chord Up 2");
    CHECK(chi_arp_pick(chord, 3, CHI_ARP_DOWN, CHI_ARP_OCT1, 0) == 55, "chord Down 0");
    CHECK(chi_arp_pick(chord, 3, CHI_ARP_UP, CHI_ARP_OCT2, 3) == 60, "chord Up oct2");
    /* Same pick as clip: Chicago adapter must equal ck_arp_pick for shared modes. */
    for (i = 0; i < 12; i++) {
        int a = chi_arp_pick(chord, 3, CHI_ARP_UPDN, CHI_ARP_OCT2, (uint32_t)i);
        int b = ck_arp_pick(chord, 3, CK_ORD_UPDN, 2u, (uint32_t)i, 0u);
        CHECK(a == b, "chi_arp_pick == ck_arp_pick UpDn step %d (got %d vs %d)", i, a, b);
    }
}

static void test_runtime_clock(void)
{
    chi_arp_run_t r;
    step_cap_t cap = {{0}, 0};
    uint32_t period;
    chi_arp_run_hold(&r, 48, CHI_ARP_UP, CHI_ARP_OCT2, CHI_ARP_SPD_1_16);
    period = chi_arp_samples_per_step(120.0f, CHI_ARP_SPD_1_16, 44100.0f);
    CHECK(period > 1000 && period < 6000, "1/16 @ 120 bpm period sane (%u)", period);
    chi_arp_run_process(&r, 1, 120.0f, 44100.0f, capture_step, &cap);
    CHECK(cap.n == 1 && cap.notes[0] == 48, "fires immediately on 48");
    chi_arp_run_process(&r, period - 1, 120.0f, 44100.0f, capture_step, &cap);
    CHECK(cap.n == 1, "quiet until next period");
    chi_arp_run_process(&r, 1, 120.0f, 44100.0f, capture_step, &cap);
    CHECK(cap.n == 2 && cap.notes[1] == 60, "second note 60 (octave)");
}

typedef struct {
    bass303_t *b;
    int fired;
} arp_dsp_ctx_t;

static void arp_dsp_on_step(int note, void *ctx)
{
    arp_dsp_ctx_t *c = (arp_dsp_ctx_t *)ctx;
    bass303_note_on_velocity(c->b, note, 0, 0, 100);
    c->fired++;
}

static void test_dsp_pitch_changes(void)
{
    bass303_t b;
    chi_arp_run_t r;
    arp_dsp_ctx_t ctx;
    int pos;
    double p_lo, p_hi;
    bass303_init(&b);
    bass303_set(&b, BASS303_VOLUME, 96);
    bass303_set(&b, BASS303_DRIVE, 0);
    bass303_set(&b, BASS303_ENVMOD, 0);
    bass303_set(&b, BASS303_WAVE, 0);
    bass303_set(&b, BASS303_ASUSTAIN, 127);
    bass303_set(&b, BASS303_ADECAY, 10);
    ctx.b = &b;
    ctx.fired = 0;
    chi_arp_run_hold(&r, 36, CHI_ARP_UP, CHI_ARP_OCT2, CHI_ARP_SPD_1_8);
    pos = 0;
    while (pos < 22050) {
        int m = 128;
        chi_arp_run_process(&r, (uint32_t)m, 120.0f, 44100.0f, arp_dsp_on_step, &ctx);
        bass303_render(&b, buf + pos, m);
        pos += m;
    }
    CHECK(ctx.fired >= 2, "arp fired multiple steps (got %d)", ctx.fired);
    p_lo = period_est(buf, 1000, 4000);
    p_hi = period_est(buf, 12000, 16000);
    CHECK(p_lo > 0 && p_hi > 0, "both windows have pitch");
    CHECK(p_hi < p_lo * 0.85, "later window higher pitch (lo %g hi %g)", p_lo, p_hi);
    CHECK(rms_range(buf, 1000, 4000) > 1e-5 && rms_range(buf, 12000, 16000) > 1e-5, "audio present");
}

int main(void)
{
    test_surface();
    test_factory_decode();
    test_pick_modes();
    test_runtime_clock();
    test_dsp_pitch_changes();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    puts("PASS chicago_arp");
    return 0;
}
