/* SPDX-License-Identifier: GPL-3.0-only */
/* Momentary bus FX: cutoff, decay gate, high-pass, send throws, and the page map. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "busfx.h"
#include "smc_fx_map.h"

static int failures;

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
    busfx_apply(&s, y, 0, 0, 0, 1, 128, 127.0f, 0, BFX_GATE, 128, 128);
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
    busfx_apply(&s, x, 0, 0, 0, 1, 256, 127.0f, 0, BFX_REV, 256, 256);
    expect(rms(x, 256) > 0.3f, "open cutoff keeps a tone");
    busfx_apply(&s, x + 256, 0, 0, 0, 1, 2048, 0.0f, 0, BFX_REV, 2048, 2048);
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
    x[8] = 0.95f; /* a transient while the gate is held */
    busfx_apply(&s, x, 0, 0, 0, 1, 256, 127.0f, 0, BFX_GATE, 4, 20);
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

static void test_two_buttons(void)
{
    busfx_t a, b;
    float x[128], y[128];
    busfx_init(&a);
    busfx_init(&b);
    fill_sine(x, 128, 1000.0f, 0.5f);
    fill_sine(y, 128, 200.0f, 0.5f);
    busfx_apply(&a, x, 0, 0, 0, 1, 128, 127.0f, 1, BFX_GATE, 128, 128);
    busfx_apply(&b, y, 0, 0, 0, 1, 128, 40.0f, 1, BFX_HP, 128, 128);
    expect(rms(x, 128) > 0.05f && rms(y, 128) > 0.01f, "two buses stay audible together");
}

static void test_highpass_returns(void)
{
    busfx_t s;
    float dry[1024], wet[1024];
    busfx_init(&s);
    fill_sine(dry, 1024, 80.0f, 0.6f);
    memcpy(wet, dry, sizeof wet);
    busfx_apply(&s, wet, 0, 0, 0, 1, 512, 127.0f, 1, BFX_HP, 512, 512);
    expect(rms(wet + 256, 256) < rms(dry + 256, 256) * 0.55f, "held high-pass thins the low end");
    busfx_apply(&s, wet + 512, 0, 0, 0, 1, 512, 127.0f, 0, BFX_HP, 512, 512);
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
    busfx_apply(&s, wet, 0, 0, 0, 1, 512, 127.0f, 1, BFX_LP, 512, 512);
    expect(rms(wet + 256, 256) < rms(dry + 256, 256) * 0.75f, "held low-pass dulls the tone");
    busfx_apply(&s, wet + 512, 0, 0, 0, 1, 512, 127.0f, 0, BFX_LP, 512, 512);
    expect(rms(wet + 768, 256) > rms(dry + 768, 256) * 0.85f, "release brings brightness back");
}

static int suit_has_gate(int bus)
{
    int k;
    for (k = 0; k < SMCFX_SUIT_N[bus]; k++)
        if (SMCFX_SUIT[bus][k] == SMCFX_GATE)
            return 1;
    return 0;
}

static void test_map(void)
{
    int page, bus, k, prev;
    expect(SMCFX_N == 4, "four buses");
    expect(SMCFX_PART[SMCFX_909] == 0 && SMCFX_PART[SMCFX_808] == 1 &&
               SMCFX_PART[SMCFX_303A] == 2 && SMCFX_PART[SMCFX_BRK] == 4,
           "buttons sit on 909, 808, 303A and BREAK");
    for (bus = 0; bus < SMCFX_N; bus++) {
        expect(SMCFX_SUIT_N[bus] == 4, "each bus has four cyclable effects");
        for (k = 0; k < 4; k++)
            expect(SMCFX_SUIT[bus][k] < SMCFX_NFX, "suit entries are valid effects");
        if (bus == SMCFX_303A)
            expect(!suit_has_gate(bus), "303A never uses the break decay gate");
        if (bus == SMCFX_BRK)
            expect(SMCFX_SUIT[bus][0] == SMCFX_GATE, "BREAK leads with the decay gate");
    }
    expect(SMCFX_PAGE[0][SMCFX_BRK] == SMCFX_GATE, "BREAK starts on the decay gate");
    expect(SMCFX_PAGE[0][SMCFX_909] == SMCFX_REV, "909 starts on the reverb throw");
    expect(SMCFX_PAGE[0][SMCFX_303A] == SMCFX_HP, "303A starts on the high-pass");
    expect(SMCFX_PAGE[0][SMCFX_808] == SMCFX_DLY, "808 starts on the delay throw");
    for (page = 0; page < SMCFX_NPAGE; page++) {
        for (bus = 0; bus < SMCFX_N; bus++)
            expect(smcfx_suit_ok(bus, SMCFX_PAGE[page][bus]),
                   "preset pages only assign bus-suited effects");
        if (page)
            expect(memcmp(SMCFX_PAGE[page], SMCFX_PAGE[0], SMCFX_N) != 0, "pages are not copies");
    }
    expect(SMCFX_PAGE[1][SMCFX_909] == SMCFX_GATE && SMCFX_PAGE[1][SMCFX_303A] == SMCFX_HP,
           "TIGHT is not a global rotation of ROOM");
    prev = SMCFX_REV;
    prev = smcfx_cycle(SMCFX_909, (uint8_t)prev);
    expect(prev == SMCFX_DLY, "home cycles 909 to the next drum effect");
    prev = smcfx_cycle(SMCFX_909, (uint8_t)prev);
    prev = smcfx_cycle(SMCFX_909, (uint8_t)prev);
    prev = smcfx_cycle(SMCFX_909, (uint8_t)prev);
    expect(prev == SMCFX_REV, "909's four-effect list wraps");
    expect(smcfx_cycle(SMCFX_303A, SMCFX_HP) == SMCFX_DLY, "303A cycles its own four");
    expect(smcfx_bus(24) == SMCFX_909 && smcfx_bus(28) == SMCFX_BRK && smcfx_bus(27) < 0,
           "303B select is not one of the four effect buttons");
}

int main(void)
{
    test_wire();
    test_cutoff_close();
    test_gate_inside_buffer();
    test_two_buttons();
    test_highpass_returns();
    test_sends();
    test_lowpass_returns();
    test_map();
    if (failures) {
        fprintf(stderr, "%d failed\n", failures);
        return 1;
    }
    puts("PASS smc fx: wire cutoff, smooth close, in-buffer gate, two buses, HP/LP return, sends, four-per-bus pages");
    return 0;
}
