/* SPDX-License-Identifier: GPL-3.0-only */
/* Observe complete production engine state; audio still runs through engine_render. */
#include "../../firmware/src/app/engine.c"

static chi_fx_insert_t saved_insert;

int chicago_test_idle(int v) { return b303[v].idle; }
int chicago_test_delay(int v) { return chi_fx[v].bpmdly.delay; }
int chicago_test_position(int v) { return chi_fx[v].bpmdly.w; }
int chicago_test_type(int v) { return chi_fx_type[v]; }
int chicago_test_miami(int v)
{
#if X0X_DEV_WOBBLE
    return bass_wobble_voice(v);
#else
    (void)v;
    return 0;
#endif
}
void chicago_test_save(int v) { saved_insert = chi_fx[v]; }
int chicago_test_unchanged(int v)
{
    return memcmp(&saved_insert, &chi_fx[v], sizeof saved_insert) == 0;
}
void chicago_test_impulse(int v)
{
    float impulse[64] = {1.0f};
    chi_fx_process(v, impulse, 64);
}
double chicago_test_energy(int v)
{
    double e = 0;
    for (int i = 0; i < CHI_BPMDLY_MAX; ++i) {
        double x = chi_fx[v].bpmdly.buf[i] / 32767.0;
        e += x * x;
    }
    return e;
}
