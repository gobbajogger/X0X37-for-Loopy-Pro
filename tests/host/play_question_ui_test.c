/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise production question, transport, undo and SMC MIDI dispatch. */
#define SMC_UI_MAIN smc_ui_fixture_main
#include "smc_ui_test.c"

static void question_play(int controller, int playing, int global)
{
    setup(global ? V_GLO : V_HOME, PART_909);
    seq.playing = playing;
    seq.req_start = seq.req_stop = seq.req_cont = 0;
    ui.overlay = O_LIST;
    ui.list_sel = 2;
    ask(ACT_CLEAR_PAT, 0, "CLEAR THIS PATTERN?", "ALL FIVE PARTS");
    pattern_t before = proj.pat[0];
    if (controller) message(0x90, 0x5E, 127);
    else button_tap(B_PLAY);
    CHECK(ui.overlay == O_LIST);
    CHECK(ui.list_sel == 2);
    CHECK(!strcmp(ui.msg, "CANCELLED"));
    CHECK(!memcmp(&before, &proj.pat[0], sizeof before));
    CHECK(playing ? seq.req_stop && !seq.req_start : seq.req_start && !seq.req_stop);
    if (controller) { /* A held repeat must not toggle back. */
        message(0x90, 0x5E, 127);
        CHECK(playing ? seq.req_stop && !seq.req_start : seq.req_start && !seq.req_stop);
        message(0x90, 0x5E, 0);
    }
}
static void question_redo(int controller)
{
    setup(V_HOME, PART_909);
    uint8_t *cell = &proj.sound.v[T_MIX][0][MX_PAN];
    int old = *cell;
    undo_start();
    *cell = old == 44 ? 45 : 44;
    int edited = *cell;
    CHECK(undo_commit_stable() > 0);
    do_undo(0);
    CHECK(*cell == old);
    seq.playing = seq.req_start = seq.req_stop = seq.req_cont = 0;
    if (controller) message(0x90, 6, 127); /* HOME held before the question opens. */
    ask(ACT_CLEAR_PAT, 0, "CLEAR THIS PATTERN?", 0);
    if (controller) {
        message(0x90, 0x5E, 127);
    } else {
        ui.btn = 1u << B_HOME;
        button_tap(B_PLAY);
        CHECK(ui.btn_used & (1u << B_HOME));
    }
    if (*cell != edited) fprintf(stderr, "redo controller=%d old=%d edited=%d actual=%d cur=%d n=%d msg=%s\n", controller, old, edited, *cell, undo.cur, undo.n, ui.msg);
    CHECK(*cell == edited);
    CHECK(ui.overlay == O_ASK);
    CHECK(!seq.req_start && !seq.req_stop && !seq.req_cont);
}
int main(void)
{
    for (int c = 0; c < 2; ++c) {
        for (int p = 0; p < 2; ++p)
            for (int g = 0; g < 2; ++g) question_play(c, p, g);
        question_redo(c);
    }
    printf("play-question-ui: PASS (%u checks)\n", checks);
    return 0;
}
