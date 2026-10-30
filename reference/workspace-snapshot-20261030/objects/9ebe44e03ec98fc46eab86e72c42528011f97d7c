/* SPDX-License-Identifier: GPL-3.0-only */
/* Untagged legacy bass bytes must not index a new engine's enum names out of bounds. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
int main(void)
{
    char num[16], unit[8];
    boot();
    ui.part = PART_303A; ui.view = V_PART; ui.overlay = O_NONE;
    ui.held_step = -1;
    proj.sound.v[T_303][0][W_CARRIER_WAVE] = 48; /* preserved historical Chicago cutoff */
    pref_value_of(PR(R_ENG, T_303, 0, W_CARRIER_WAVE), 48, num, unit);
    assert(!strcmp(num, "INVALID"));
    assert(proj.sound.v[T_303][0][W_CARRIER_WAVE] == 48);
    build_pages(); draw_header(); draw_main(); draw_knobs();
    assert(proj.sound.v[T_303][0][W_CARRIER_WAVE] == 48);
    /* Ordinary valid names and explicit subsequent user edits still work. */
    pref_value_of(PR(R_ENG, T_303, 0, W_CARRIER_WAVE), 1, num, unit);
    assert(!strcmp(num, engine_param(T_303, 0, W_CARRIER_WAVE)->names[1]));
    pref_value_of(PR(R_ENG, T_303, 0, W_CARRIER_WAVE), -1, num, unit);
    assert(!strcmp(num, "INVALID"));
    pref_set(PR(R_ENG, T_303, 0, W_CARRIER_WAVE), 0);
    assert(proj.sound.v[T_303][0][W_CARRIER_WAVE] == 0);
    puts("PASS actual legacy bass enum rendering refuses OOB display without changing stored bytes");
    return 0;
}
