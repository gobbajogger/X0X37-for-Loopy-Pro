/* SPDX-License-Identifier: GPL-3.0-only */
/* Momentary bus FX: cutoff, gates, rhythm FX, sends, and the page map. */
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

static float max_step(const float *x, int n)
{
    float m = 0;
    int i;
    for (i = 1; i < n; i++) {
        float d = x[i] - x[i - 1];
        if (d < 0)
            d = -d;
        if (d > m)
            m = d;
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
    busfx_apply(&s, y, 0, 0, 0, 1, 128, 127.0f, 0, BFX_GATE, 128, 128, BPM, T0);
    for (i = 0; i < 128; i++)
        if (y[i] != x[i])
            break;
    expect(i == 128, "open cutoff is a wire");
}

static void test_cutoff_close(void)
{
    busfx_t s;
    float x[2304];
    busfx_init(&s);
    fill_sine(x, 2304, 2000.0f, 0.5f);
    busfx_apply(&s, x, 0, 0, 0, 1, 256, 127.0f, 0, BFX_REV, 256, 256, BPM, T0);
    expect(rms(x, 256) > 0.3f, "open cutoff keeps a tone");
    busfx_apply(&s, x + 256, 0, 0, 0, 1, 2048, 0.0f, 0, BFX_REV, 2048, 2048, BPM, T0);
    expect(max_step(x + 256, 256) < 0.35f, "closing cutoff does not click");
    expect(rms(x + 2048, 256) < 0.05f, "minimum cutoff closes the tone");
    expect(rms(x + 2048, 256) > 1.0e-6f, "minimum cutoff is not a hard mute");
}

static void test_gate_inside_buffer(void)
{
    busfx_t s;
    float x[256];
    int i;
    busfx_init(&s);
    for (i = 0; i < 256; i++)
        x[i] = 0.4f;
    x[8] = 0.95f;
    busfx_apply(&s, x, 0, 0, 0, 1, 256, 127.0f, 0, BFX_GATE, 4, 20, BPM, T0);
    expect(max_step(x, 256) < 0.7f, "press and release inside one buffer does not jump");
    expect(x[8] > 0.5f, "the transient stays");
    expect(x[255] > 0.35f, "release returns the steady tone");
    {
        double e = 0;
        for (i = 0; i < 256; i++)
            e += fabsf(x[i]);
        expect(e > 10.0, "the buffer is not dropped out");
    }
}

static void test_rhythm_gate(void)
{
    busfx_t s;
    float x[16384], dry[16384];
    uint32_t sp16 = busfx_sp16(BPM), sp8 = busfx_sp8(BPM);
    busfx_init(&s);
    fill_sine(x, 16384, 110.0f, 0.5f);
    memcpy(dry, x, sizeof dry);
    busfx_apply(&s, x, 0, 0, 0, 1, 16384, 127.0f, 1, BFX_GATE16, 16384, 16384, BPM, T0);
    expect(rms(x + 12000, 4096) < rms(dry + 12000, 4096) * 0.82f, "16th gate thins a held tone");
    expect(busfx_rhythm_target(BFX_GATE16, BPM, 0) == 1.0f, "16th gate opens at phase zero");
    expect(busfx_rhythm_target(BFX_GATE16, BPM, (sp16 * 3u) / 4u) == 0.0f, "16th gate closes late in division");
    expect(busfx_rhythm_target(BFX_GATE8, BPM, 0) == 1.0f &&
               busfx_rhythm_target(BFX_GATE8, BPM, (sp8 * 3u) / 4u) == 0.0f,
           "8th gate toggles at tempo");
    busfx_init(&s);
    fill_sine(x, 8192, 110.0f, 0.5f);
    busfx_apply(&s, x, 0, 0, 0, 1, 8192, 127.0f, 1, BFX_GATE8, 8192, 8192, BPM, T0);
    expect(rms(x + 6000, 2048) < 0.32f, "8th gate applies on audio");
}

static void test_two_buttons(void)
{
    busfx_t a, b;
    float x[128], y[128];
    busfx_init(&a);
    busfx_init(&b);
    fill_sine(x, 128, 1000.0f, 0.5f);
    fill_sine(y, 128, 200.0f, 0.5f);
    busfx_apply(&a, x, 0, 0, 0, 1, 128, 127.0f, 1, BFX_GATE, 128, 128, BPM, T0);
    busfx_apply(&b, y, 0, 0, 0, 1, 128, 40.0f, 1, BFX_HP, 128, 128, BPM, T0);
    expect(rms(x, 128) > 0.05f && rms(y, 128) > 0.01f, "two buses stay audible together");
}

static void test_highpass_returns(void)
{
    busfx_t s;
    float dry[1024], wet[1024];
    busfx_init(&s);
    fill_sine(dry, 1024, 80.0f, 0.6f);
    memcpy(wet, dry, sizeof wet);
    busfx_apply(&s, wet, 0, 0, 0, 1, 512, 127.0f, 1, BFX_HP, 512, 512, BPM, T0);
    expect(rms(wet + 256, 256) < rms(dry + 256, 256) * 0.55f, "held high-pass thins the low end");
    busfx_apply(&s, wet + 512, 0, 0, 0, 1, 512, 127.0f, 0, BFX_HP, 512, 512, BPM, T0);
    expect(rms(wet + 768, 256) > rms(dry + 768, 256) * 0.85f, "release brings the low end back");
}

static void test_sends(void)
{
    expect(busfx_send(0.2f, 1, BFX_REV, BFX_REV) == 1.0f, "reverb throw opens the send");
    expect(busfx_send(0.2f, 0, BFX_REV, BFX_REV) == 0.2f, "release restores the saved send");
    expect(busfx_send(0.0f, 1, BFX_DLY, BFX_DLY) == 1.0f, "delay throw opens the send");
    expect(busfx_send(0.4f, 1, BFX_GATE, BFX_DLY) == 0.4f, "a gate does not steal the delay send");
}

static void test_lowpass_returns(void)
{
    busfx_t s;
    float dry[1024], wet[1024];
    busfx_init(&s);
    fill_sine(dry, 1024, 2000.0f, 0.6f);
    memcpy(wet, dry, sizeof wet);
    busfx_apply(&s, wet, 0, 0, 0, 1, 512, 127.0f, 1, BFX_LP, 512, 512, BPM, T0);
    expect(rms(wet + 256, 256) < rms(dry + 256, 256) * 0.75f, "held low-pass dulls the tone");
    busfx_apply(&s, wet + 512, 0, 0, 0, 1, 512, 127.0f, 0, BFX_LP, 512, 512, BPM, T0);
    expect(rms(wet + 768, 256) > rms(dry + 768, 256) * 0.85f, "release brings brightness back");
}

static int suit_has(int bus, uint8_t fx)
{
    int k;
    for (k = 0; k < SMCFX_SUIT_N[bus]; k++)
        if (SMCFX_SUIT[bus][k] == fx)
            return 1;
    return 0;
}

static void test_map(void)
{
    int page, bus, k, prev;
    expect(SMCFX_N == 7, "seven momentary strips");
    expect(SMCFX_NOTE[SMCFX_RVB] == 29 && SMCFX_NOTE[SMCFX_MDLY] == 30 && SMCFX_NOTE[SMCFX_FLT] == 31,
           "main strips use SELECT notes 29-31");
    for (bus = 0; bus < SMCFX_N; bus++) {
        expect(SMCFX_SUIT_N[bus] >= 1 && SMCFX_SUIT_N[bus] <= SMCFX_SUIT_MAX, "each bus has a curated list");
        for (k = 0; k < SMCFX_SUIT_N[bus]; k++)
            expect(SMCFX_SUIT[bus][k] < SMCFX_NFX, "suit entries are valid effects");
        if (bus == SMCFX_303A)
            expect(!suit_has(bus, SMCFX_GATE), "303A omits break decay gate");
        if (bus == SMCFX_BRK)
            expect(SMCFX_SUIT[bus][0] == SMCFX_GATE, "BREAK leads with decay gate");
        if (bus == SMCFX_RVB) {
            expect(suit_has(bus, SMCFX_GATE8) && suit_has(bus, SMCFX_GATE16) && suit_has(bus, SMCFX_TRANCE),
                   "RVB lists tempo gates on the reverb send");
            expect(SMCFX_SUIT[bus][0] == SMCFX_REV, "RVB leads with reverb throw");
        }
    }
    expect(SMCFX_PAGE[0][SMCFX_BRK] == SMCFX_GATE, "BREAK ROOM default is decay gate");
    expect(SMCFX_PAGE[0][SMCFX_RVB] == SMCFX_REV, "RVB ROOM default is reverb throw");
    for (page = 0; page < SMCFX_NPAGE; page++) {
        for (bus = 0; bus < SMCFX_N; bus++)
            expect(smcfx_suit_ok(bus, SMCFX_PAGE[page][bus]), "preset pages only assign suited effects");
    }
    prev = SMCFX_REV;
    prev = smcfx_cycle(SMCFX_909, (uint8_t)prev);
    expect(prev == SMCFX_DLY, "909 cycles throws");
    expect(smcfx_cycle(SMCFX_RVB, SMCFX_REV) == SMCFX_GATE8, "RVB cycles to 8th gate after throw");
    expect(!smcfx_suit_ok(SMCFX_909, SMCFX_GATE), "decay gate stays off drum machine buses");
    expect(smcfx_bus(24) == SMCFX_909 && smcfx_bus(31) == SMCFX_FLT && smcfx_bus(27) < 0,
           "303B select is not a momentary strip");
}

int main(void)
{
    test_wire();
    test_cutoff_close();
    test_gate_inside_buffer();
    test_rhythm_gate();
    test_two_buttons();
    test_highpass_returns();
    test_sends();
    test_lowpass_returns();
    test_map();
    if (failures) {
        fprintf(stderr, "%d failed\n", failures);
        return 1;
    }
    puts("PASS smc fx: wire, gates, rhythm, HP/LP, sends, seven-bus map");
    return 0;
}
