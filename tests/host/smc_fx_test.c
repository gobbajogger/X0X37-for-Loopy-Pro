/* SPDX-License-Identifier: GPL-3.0-only */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "busfx.h"
#include "smc_fx_map.h"

static int failures;
#define BPM 120.0f
#define T0 0u

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        failures++;
    }
}

static float rms(const float *x, int n)
{
    double a = 0;
    int i;
    for (i = 0; i < n; i++)
        a += (double)x[i] * (double)x[i];
    return (float)sqrt(a / n);
}

static float max_abs(const float *x, int n)
{
    float m = 0;
    int i;
    for (i = 0; i < n; i++) {
        float a = x[i] < 0 ? -x[i] : x[i];
        if (a > m)
            m = a;
    }
    return m;
}

static void fill_sine(float *x, int n, float hz, float amp)
{
    int i;
    for (i = 0; i < n; i++)
        x[i] = amp * sinf(2.0f * 3.14159265f * hz * (float)i / 44100.0f);
}

static void test_wire(void)
{
    busfx_t s;
    float x[128], y[128];
    int i;
    busfx_init(&s);
    for (i = 0; i < 128; i++)
        x[i] = y[i] = ((i * 17) % 50) * 0.01f - 0.25f;
    busfx_apply(&s, 0, 0, y, 0, 0, 0, 1, 128, 127.0f, 0, BFX_GATE, 128, 128, BPM, T0);
    for (i = 0; i < 128; i++)
        if (y[i] != x[i])
            break;
    expect(i == 128, "open cutoff is a wire");
}

static void test_bpm_range(void)
{
    expect(busfx_sp16(10.0f) == busfx_sp16(20.0f), "BPM floors at 20");
    expect(busfx_sp16(400.0f) == busfx_sp16(300.0f), "BPM caps at 300");
}

static void test_main_throw(void)
{
    expect(busfx_send(BFX_SEND_NOM, 1, BFX_REV, BFX_REV) == 1.0f, "RVB rev throw opens");
    expect(busfx_send(BFX_SEND_NOM, 0, BFX_REV, BFX_REV) == BFX_SEND_NOM, "RVB rev throw releases");
    expect(busfx_send(BFX_SEND_NOM, 1, BFX_DLY, BFX_DLY) == 1.0f, "DLY throw opens");
}

static void test_stutter_stereo(void)
{
    busfx_t s;
    busfx_stut_t stut;
    float l[2048], r[2048];
    int i;
    busfx_init(&s);
    busfx_stut_init(&stut);
    for (i = 0; i < 2048; i++) {
        l[i] = 1.0f;
        r[i] = 0.0f;
    }
    busfx_apply(&s, &stut, 0, l, r, 0, 0, 2, 2048, 127.0f, 1, BFX_STUT, 2048, 2048, BPM, 0u);
    expect(max_abs(r, 2048) < 0.08f, "stutter right stays dry when left-only input");
    expect(max_abs(l, 2048) > 0.2f, "stutter left still moves");
}

static void test_settling(void)
{
    busfx_t s;
    float x[512];
    busfx_init(&s);
    fill_sine(x, 512, 200.0f, 0.5f);
    busfx_apply(&s, 0, 0, x, 0, 0, 0, 1, 256, 127.0f, 1, BFX_HP, 256, 256, BPM, T0);
    expect(busfx_settling(&s), "HP hold leaves filters settling");
    busfx_apply(&s, 0, 0, x + 256, 0, 0, 0, 1, 256, 127.0f, 0, BFX_HP, 256, 256, BPM, T0);
    expect(busfx_settling(&s), "release keeps HP settling");
}

static void test_rhythm_gate(void)
{
    busfx_t s;
    float x[16384], dry[16384];
    uint32_t sp16 = busfx_sp16(BPM), sp8 = busfx_sp8(BPM);
    busfx_init(&s);
    fill_sine(x, 16384, 110.0f, 0.5f);
    memcpy(dry, x, sizeof dry);
    busfx_apply(&s, 0, 0, x, 0, 0, 0, 1, 16384, 127.0f, 1, BFX_GATE16, 16384, 16384, BPM, T0);
    expect(rms(x + 12000, 4096) < rms(dry + 12000, 4096) * 0.82f, "16th gate thins a held tone");
    expect(busfx_rhythm_target(BFX_GATE16, BPM, 0) == 1.0f, "16th gate opens at phase zero");
    expect(busfx_rhythm_target(BFX_GATE16, BPM, (sp16 * 3u) / 4u) == 0.0f, "16th gate closes late in division");
    expect(busfx_rhythm_target(BFX_GATE8, BPM, 0) == 1.0f &&
               busfx_rhythm_target(BFX_GATE8, BPM, (sp8 * 3u) / 4u) == 0.0f,
           "8th gate toggles at tempo");
}

static void test_map(void)
{
    int page, bus, k;
    expect(SMCFX_N == 8, "eight momentary strips");
    expect(SMCFX_NOTE[SMCFX_303B] == 27 && SMCFX_PART[3] == 3, "303B is note 27 on PART_303B");
    expect(SMCFX_NOTE[SMCFX_RVB] == 29 && SMCFX_NOTE[SMCFX_MDLY] == 30 && SMCFX_NOTE[SMCFX_FLT] == 31,
           "main strips use SELECT notes 29-31");
    for (bus = 0; bus < SMCFX_N; bus++) {
        for (k = 0; k < SMCFX_SUIT_N[bus]; k++)
            expect(SMCFX_SUIT[bus][k] < SMCFX_NFX, "suit entries are valid effects");
        if (bus == SMCFX_RVB)
            expect(SMCFX_SUIT[bus][1] == SMCFX_GATE8 && SMCFX_SUIT[bus][0] == SMCFX_REV,
                   "RVB lists throw then tempo gates");
    }
    for (page = 0; page < SMCFX_NPAGE; page++)
        for (bus = 0; bus < SMCFX_N; bus++)
            expect(smcfx_suit_ok(bus, SMCFX_PAGE[page][bus]), "preset pages only assign suited effects");
    expect(smcfx_cycle(SMCFX_RVB, SMCFX_REV) == SMCFX_GATE8, "RVB cycles to 8th gate after throw");
    expect(smcfx_bus(27) == SMCFX_303B && smcfx_bus(31) == SMCFX_FLT, "all eight SELECT notes map");
}

int main(void)
{
    test_wire();
    test_bpm_range();
    test_main_throw();
    test_stutter_stereo();
    test_settling();
    test_rhythm_gate();
    test_map();
    if (failures) {
        fprintf(stderr, "%d failed\n", failures);
        return 1;
    }
    puts("PASS smc fx: BPM range, throws, stutter stereo, settling, rhythm, eight-bus map");
    return 0;
}
