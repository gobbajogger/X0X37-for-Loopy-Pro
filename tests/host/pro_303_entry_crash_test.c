/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production boot/UI contract for Chicago and Miami in the 303 slots. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if X0X_ORIGINAL_SYNTH != 1
#error Only the shipping Chicago/Miami engines may enter this test
#endif

static void verify_slots(void)
{
    const unsigned banks[2] = {ORIGINAL_SOUND_MIAMI, ORIGINAL_SOUND_CHICAGO};
    assert(PROJ_SND_REV == 13u && PROJ_STREAM == 13u);
    assert(PART_303A == 2 && PART_303B == 3 && NBASS == 2);
    assert(engine_bass_sound_validate(&proj.sound) == 0);
    for (int voice = 0; voice < NBASS; ++voice) {
        const uint8_t *cell = proj.sound.v[T_303][voice];
        unsigned bank = banks[voice];
        assert(engine_bass_bank(voice) == (int)bank);
        assert(engine_bass_sound_bank(&proj.sound, voice) == (int)bank);
        assert(engine_bass_is_wobble(voice) == (bank == ORIGINAL_SOUND_MIAMI));
        assert(original_sound_valid(cell) && cell[29] == bank);
        assert(cell[26] == 'O' && cell[27] == 'S' && cell[28] == ORIGINAL_SOUND_SCHEMA);
        assert(engine_nparams(T_303, voice) == (int)original_sound_count(bank));
        for (int param = 0; param < engine_nparams(T_303, voice); ++param) {
            const x0x_param_t *d = engine_param(T_303, voice, param);
            assert(d && cell[param] <= d->max);
        }
        ui.part = PART_303A + voice;
        set_view(V_PART);
        ui.touched = -1;
        ui.msg_until = 0;
        ui_frame();
    }
}

static void verify_invalid(void)
{
    for (int voice = 0; voice < NBASS; ++voice) {
        uint8_t *cell = proj.sound.v[T_303][voice];
        unsigned bank = (unsigned)engine_bass_bank(voice);
        uint8_t saved[ORIGINAL_SOUND_BYTES];
        memcpy(saved, cell, sizeof saved);
        for (unsigned param = 0; param < original_sound_count(bank); ++param) {
            /* The original patch codec must reject out-of-range control bytes
             * without changing any tagged project or active engine identity. */
            assert(!original_sound_set(cell, param, 255u));
            assert(!memcmp(cell, saved, sizeof saved));
        }
        unsigned param = bank == ORIGINAL_SOUND_CHICAGO ? ORIGINAL_CHICAGO_FX_TYPE : 0u;
        cell[param] = 255u;
        assert(!original_sound_valid(cell));
        assert(engine_bass_sound_bank(&proj.sound, voice) < 0);
        assert(engine_bass_sound_validate(&proj.sound) != 0);
        ui.part = PART_303A + voice;
        set_view(V_PART);
        ui.touched = -1;
        ui.msg_until = 0;
        ui_frame(); /* corrupt patches must not select an unchecked name table */
        assert(cell[param] == 255u);
        memcpy(cell, saved, sizeof saved);
        assert(original_sound_valid(cell));
        assert(engine_bass_sound_bank(&proj.sound, voice) == (int)bank);
    }
    assert(engine_bass_sound_validate(&proj.sound) == 0);
}

int main(int argc, char **argv)
{
    boot();
    verify_slots();
    if (argc > 1 && !strcmp(argv[1], "invalid-retained"))
        verify_invalid();
    else if (argc > 1)
        assert(!strcmp(argv[1], "check-defaults"));
    puts("PASS original Chicago/Miami 303 slots and tagged project guards");
    return 0;
}
