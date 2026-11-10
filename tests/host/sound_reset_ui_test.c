/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN smc_ui_fixture_main
#include "smc_ui_test.c"

static void audio_block(void)
{
    int32_t block[512];
    engine_render(block, 256);
}

static void part_span(int part, int *t, int *v, int *n)
{
    *v = 0; *n = 1;
    if (part < NKIT) { *t = part == PART_909 ? T_909 : T_808; *n = NVOICES_MAX; }
    else if ((part == PART_303A || part == PART_303B)) { *t = T_303; *v = part - PART_303A; }
    else *t = T_BRK;
}
static void open_reset(int part)
{
    message(0x90, 6, 127);
    message(0x90, 24 + part_position(part), 127);
    message(0x90, 24 + part_position(part), 0);
    message(0x90, 6, 0);
}
static void reset_part_flow(int part, int cancel)
{
    static project_t before, expected;
    sound_t factory;
    setup(V_HOME, PART_BRK); /* keyboard and SMC may select different parts */
    engine_sound_defaults(&factory);
    int t, v, n;
    part_span(part, &t, &v, &n);
    pref_set(PR(R_ENG, t, v, 0), 20);
    for (int i = 0; i < 20; ++i) audio_block();
    undo_start();
    proj.sound.v[T_MIX][part][MX_LEVEL] = 32;
    proj.sound.v[T_MIX][part][MX_PAN] = 55;
    proj.sound.v[T_MIX][part][MX_REV] = 21;
    proj.sound.v[T_MIX][part][MX_DLY] = 37;
    seq.mute = 0x15; seq.vmute = 0x123;
    before = proj;
    int keyboard_part = ui.part;
    open_reset(part);
    CHECK(ui.overlay == O_ASK);
    CHECK(!strcmp(ui.ask_q[0], "RESET SOUND"));
    CHECK(!strcmp(ui.ask_q[1], PART_N[part]));
    CHECK(ui.ask_arg == part);
    CHECK(ui.part == keyboard_part);
    ui_frame();
    if (part == PART_909 && !cancel) shot("reset-sound-909-confirm");
    if (cancel) {
        message(0x90, 6, 127);
        CHECK(ui.overlay == O_NONE);
        CHECK(!memcmp(&before, &proj, sizeof proj));
        return;
    }
    /* Only explicit R6 confirms; PLAY is covered separately by M8. */
    message(0x90, 5, 127);
    CHECK(ui.overlay == O_NONE);
    char notice[40]; snprintf(notice, sizeof notice, "RESET SOUND %s", PART_N[part]);
    CHECK(!strcmp(ui.msg, notice));
    expected = before;
    for (int i = 0; i < n; ++i)
        memcpy(expected.sound.v[t][v+i], factory.v[t][v+i], NPARAMS_MAX);
    if ((part == PART_303A || part == PART_303B)) expected.sound.bass[v] = factory.bass[v];
    CHECK(!memcmp(&expected, &proj, sizeof proj));
    CHECK(seq.mute == 0x15 && seq.vmute == 0x123);
    do_undo(0);
    CHECK(!memcmp(&before, &proj, sizeof proj)); /* mixer edits are a separate gesture */
    do_undo(1);
    CHECK(!memcmp(&expected, &proj, sizeof proj));
}
static void empty_part(int part)
{
    setup(V_HOME, PART_909);
    int t, v, n;
    part_span(part, &t, &v, &n);
    memset(proj.sound.v[t][v], 0, n * NPARAMS_MAX);
    if ((part == PART_303A || part == PART_303B)) memset(&proj.sound.bass[v], 0, sizeof proj.sound.bass[v]);
    undo_start();
    static project_t before;
    proj.sound.v[T_MIX][part][MX_PAN] = 47; /* empty reset must not commit pending mixer edits */
    before = proj;
    open_reset(part);
    CHECK(ui.overlay == O_NONE);
    CHECK(!strncmp(ui.msg, "NO SAVED SOUND", 14));
    CHECK(!memcmp(&before, &proj, sizeof proj));
    CHECK(undo.n == 0);
}
static void reset_priority(void)
{
    /* Drum bank SELECT and the existing HOME+SELECT FX action retain priority. */
    setup(V_MIX, PART_909);
    *page_of(V_MIX) = 1; build_pages(); smc_page_set(1);
    open_reset(PART_909);
    CHECK(ui.overlay != O_ASK);
    setup(V_FX, PART_909);
    /* On this base the controller HOME press navigates immediately. Hold
     * the keyboard HOME button to exercise the existing FX assignment. */
    held_btn = 1u << B_HOME;
    message(0x90, 24 + part_position(PART_909), 127);
    message(0x90, 24 + part_position(PART_909), 0);
    held_btn = 0;
    CHECK(ui.overlay != O_ASK);
    setup(V_HOME, PART_909);
    CHECK(smc_lock_toggle());
    open_reset(PART_909);
    CHECK(ui.overlay != O_ASK);
    setup(V_HOME, PART_909);
    clip_grid_mode = 1;
    open_reset(PART_909);
    CHECK(ui.overlay != O_ASK);
}
static void busy_bass(void)
{
    setup(V_HOME, PART_909);
    pref_set(PR(R_ENG, T_303, 0, 0), 20);
    for (int i = 0; i < 20; ++i) audio_block();
    undo_start();
    sound_t before = proj.sound;
    open_reset(PART_303A);
    engine_bass_on_velocity(0, 48, 0, 0, 120); /* queued input: cannot adopt now */
    message(0x90, 5, 127);
    CHECK(ui.overlay == O_ASK);
    CHECK(!memcmp(&before, &proj.sound, sizeof before));
    CHECK(undo.n == 0);
    message(0x90, 6, 127); /* cancel still works during the wait */
    CHECK(ui.overlay == O_NONE);
}
static void recording_boundary(void)
{
    setup(V_HOME, PART_909);
    pref_set(PR(R_ENG, T_909, 0, 0), 20);
    for (int i = 0; i < 20; ++i) audio_block();
    undo_start();
    seq.playing = ui.rec = 1;
    seq.rec_mask = seq.rec_ext_mask = 1;
    seq_rec_bass(&seq, 0, 48, 100, 1, 0, 0);
    CHECK(seq.rec_bass[0].active);
    seq.ext_now += 1000;
    unsigned epoch = seq.rec_epoch;
    open_reset(PART_909);
    message(0x90, 5, 127);
    CHECK(!seq.rec_bass[0].active && !seq.rec_mask && !seq.rec_ext_mask);
    CHECK(seq.rec_epoch != epoch);
    CHECK(undo.n == 2); /* finalized recording then sound-only reset */
    static project_t after;
    after = proj;
    do_undo(0);
    CHECK(proj.sound.v[T_909][0][0] == 20);
    CHECK(!memcmp(proj.pat, after.pat, sizeof proj.pat));
    CHECK(!memcmp(&proj.arr, &after.arr, sizeof proj.arr));
}
int main(void)
{
    recording_boundary();
    for (int part = 0; part < NPARTS; ++part) {
        reset_part_flow(part, 1);
        reset_part_flow(part, 0);
        empty_part(part);
    }
    reset_priority();
    busy_bass();
    printf("sound-reset-ui: PASS (%u checks)\n", checks);
    return 0;
}
