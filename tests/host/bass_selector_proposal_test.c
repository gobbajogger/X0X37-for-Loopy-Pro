/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual app/input/renderer proposal; no display or device is operated. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
static sound_t saved_sound;
static uint8_t saved_patterns[NPARTS];
static int same_patterns(void)
{
    for (int i = 0; i < NPARTS; i++)
        if (saved_patterns[i] != seq.ppat[i]) return 0;
    return 1;
}
static void turn(int role, int n)
{
    enc_acc[role] = n;
    input();
}
static void shot(const char *folder, const char *name)
{
    char path[512];
    memset(blit_hash, 0, sizeof blit_hash);
    ui.msg_until = 0; ui.touch_until = 0;
    draw_header(); draw_main(); draw_knobs();
    snprintf(path, sizeof path, "%s/%s.png", folder, name);
    png_write(path);
}
int main(int argc, char **argv)
{
    int32_t audio[512];
    CHECK(argc == 2);
    boot(); engine_guard_enable(0); engine_render(audio, 256);
    ui.view = V_PART; ui.part = PART_303A; ui.overlay = O_NONE;
    ui.held_step = -1; ui.page[V_PART][PART_303A] = 0;
    build_pages();
    CHECK(pg.r[0][0].kind == R_BVOICE && pg.r[0][1].kind == R_BINIT);
    CHECK(engine_bass_is_wobble(0)); CHECK(!engine_bass_is_wobble(1));
    CHECK(!strcmp(pref_desc(pg.r[0][0])->names[0], "MIAMI"));
    CHECK(!strcmp(pref_desc(pg.r[0][0])->names[1], "CHICAGO"));
    CHECK(bass_native_init(0));
    shot(argv[1], "miami-native");
    saved_sound = proj.sound; for (int i = 0; i < NPARTS; i++) saved_patterns[i] = seq.ppat[i];
    ui.dirty = 0;
    turn(EN_K1, 1);
    CHECK(ui.part == PART_303B); CHECK(cur_page() == 0);
    CHECK(!memcmp(&saved_sound, &proj.sound, sizeof saved_sound));
    CHECK(same_patterns());
    CHECK(!ui.dirty); CHECK(!bass_native_init(1));
    shot(argv[1], "chicago-custom");
    undo_start();
    turn(EN_K2, 1); engine_render(audio, 256);
    CHECK(bass_native_init(1));
    for (int i = 0; i < engine_nparams(T_303, 1); i++)
        CHECK(proj.sound.v[T_303][1][i] == engine_param(T_303, 1, i)->def);
    CHECK(!memcmp(saved_sound.v[T_303][0], proj.sound.v[T_303][0], NPARAMS_MAX));
    CHECK(same_patterns());
    shot(argv[1], "chicago-native");
    now_ms += UNDO_QUIET + 1; ui.frame &= ~3u; undo_tick();
    do_undo(0); engine_render(audio, 256);
    CHECK(!memcmp(&saved_sound, &proj.sound, sizeof saved_sound));
    do_undo(1); engine_render(audio, 256); CHECK(bass_native_init(1));
    /* Existing sound pages/labels remain: selector moves by the ordinary SELECT. */
    turn(EN_SELECT, 1);
    CHECK(cur_page() == 1); CHECK(pg.r[1][0].c == BASS303_CUTOFF);
    CHECK(!strcmp(pref_name(pg.r[1][0]), engine_param(T_303, 1, BASS303_CUTOFF)->name));
    shot(argv[1], "chicago-tone");
    turn(EN_K1, 1); CHECK(!bass_native_init(1));
    turn(EN_SELECT, -1); CHECK(cur_page() == 0);
    saved_sound = proj.sound;
    seq.playing = 1; turn(EN_K2, 1); seq.playing = 0;
    CHECK(!memcmp(&saved_sound, &proj.sound, sizeof saved_sound));
    CHECK(!strcmp(ui.msg, "STOP NOTES BEFORE INIT"));
    /* A held step stays a per-parameter edit; neither selector mutates its sound. */
    ui.held_step = 0; ui.held_step_part = ui.part; ui.held_step_pat = seq.ppat[ui.part];
    turn(EN_K2, 1); turn(EN_K1, -1); turn(EN_PRESET, 1);
    CHECK(same_patterns());
    CHECK(ui.part == PART_303B); CHECK(!memcmp(&saved_sound, &proj.sound, sizeof saved_sound));
    ui.held_step = -1;
    turn(EN_K1, -1); CHECK(ui.part == PART_303A);
    /* PRESETS means the sole real native preset only on the proposal sound page. */
    proj.sound.v[T_303][0][W_CUTOFF] ^= 1;
    CHECK(!bass_native_init(0));
    saved_sound = proj.sound;
    engine_bass_on(0, 48, 0, 0); engine_render(audio, 256);
    turn(EN_PRESET, 1); CHECK(!memcmp(&saved_sound, &proj.sound, sizeof saved_sound));
    engine_bass_off(0); engine_render(audio, 256);
    turn(EN_PRESET, 1); engine_render(audio, 256);
    CHECK(bass_native_init(0)); CHECK(same_patterns());
    turn(EN_SELECT, 1); CHECK(pg.r[1][0].c == W_CARRIER_WAVE);
    shot(argv[1], "miami-carrier");
    printf("PASS %u actual UI/input/preset/routing checks; proposal only; snapshots and per-voice reassignment remain open\n", checks);
    return 0;
}
