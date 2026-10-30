/* SPDX-License-Identifier: GPL-3.0-only */
/* Drum part-page glyphs: every visible 909/808 control has a glyph role that
 * matches what it does, BRK and other views keep theirs, and a hit animates the
 * cell from the real voice envelope, then the cell goes back to its idle image. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void drain_ms(int ms) { int32_t a[512]; for (int i = 0; i < ms; i += 6) engine_render(a, 256); audio_due_ms = now_ms; run_ms(ms); }
static int role_of(int p, int k, const char *want)
{
    pref_t r = pg.r[p][k];
    if (r.kind == R_NONE) return 0;
    if (want) { const char *n = matrix_name(r); size_t i = 0;
        for (; want[i] && n[i]; i++) if ((n[i] | 32) != (want[i] | 32)) { fprintf(stderr, "name %s != %s\n", n, want); CHECK(0); }
        if (want[i] || n[i]) fprintf(stderr, "name %s != %s\n", n, want);
        CHECK(!want[i] && !n[i]); }
    return drum_strip_role(r);
}
static uint32_t cell_hash(int k)
{   /* page row 0, control k: the 26x19 glyph box */
    uint32_t h = 2166136261u;
    for (int y = 40; y < 64; y++) for (int x = k * 60 + 2; x < k * 60 + 30; x++) h = (h ^ fb[240 * y + x]) * 16777619u;
    return h;
}
static void test_dispatch(void)
{
    setup(V_PART, PART_909); ui.sel[PART_909] = 0; *page_of(V_PART) = 0; build_pages();
    CHECK(role_of(0, 0, "Tune") == DG_BDSWEEP); CHECK(role_of(0, 1, "Attack") == DG_ATTACK);
    CHECK(role_of(0, 2, "Decay") == DG_DECAY);
    ui.sel[PART_909] = 1; build_pages();
    CHECK(role_of(0, 0, "Tune") == DG_TUNE); CHECK(role_of(0, 1, "Tone") == DG_NTAIL);
    CHECK(role_of(0, 3, "Snappy") == DG_SNAPPY);
    setup(V_PART, PART_808); ui.sel[PART_808] = 0; *page_of(V_PART) = 0; build_pages();
    CHECK(role_of(0, 1, "Tone") == DG_LPF);
    ui.sel[PART_808] = 1; build_pages(); CHECK(role_of(0, 1, "Tone") == DG_SHELLS);
    ui.sel[PART_808] = 2; build_pages(); int found = 0; for (int k = 0; k < 4; k++) if (pg.r[0][k].kind != R_NONE && !strcmp(matrix_name(pg.r[0][k]), "SOUND")) { found = 1; CHECK(drum_strip_role(pg.r[0][k]) == DG_SOUND); } CHECK(found);
    /* every drawn control on every 909/808 voice page has a glyph */
    for (int part = PART_909; part <= PART_808; part++) for (int v = 0; v < 11; v++) {
        setup(V_PART, part); ui.sel[part] = (uint8_t)v; *page_of(V_PART) = 0; build_pages();
        for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++)
            if (pg.r[p][k].kind != R_NONE && strcmp(pg.title[p], "KNOBS 5-8")) CHECK(role_of(p, k, NULL) || orig_strip_slot(pg.r[p][k]));
    }
    setup(V_PART, PART_BRK); build_pages();
    for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++) CHECK(!role_of(p, k, NULL));
    setup(V_FX, PART_909); build_pages();
    for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++) CHECK(!role_of(p, k, NULL));
    puts("PASS dispatch: 909/808 roles by control, BRK and other views untouched");
}
static void test_motion(int part)
{
    setup(V_PART, part); ui.sel[part] = 0; *page_of(V_PART) = 0; build_pages();
    drain_ms(300); ui_frame(); uint32_t idle = cell_hash(3), d0 = cell_hash(part == PART_909 ? 2 : 3);
    uint32_t a; float e; CHECK(!engine_drum_view(part, 0, &a, &e));
    engine_drum(part, 0, 1.0f); drain_ms(20); now_ms++; ui_frame();
    CHECK(engine_drum_view(part, 0, &a, &e)); float e1 = e; CHECK(e1 > 0.2f);
    uint32_t h1 = cell_hash(part == PART_909 ? 2 : 3); CHECK(h1 != d0);
    drain_ms(150); now_ms++; ui_frame();
    CHECK(engine_drum_view(part, 0, &a, &e)); CHECK(e < e1);                 /* the tail shrinks */
    CHECK(cell_hash(part == PART_909 ? 2 : 3) != h1);                         /* the playhead moves */
    drain_ms(6000); now_ms++; ui_frame();
    CHECK(!engine_drum_view(part, 0, &a, &e)); CHECK(cell_hash(3) == idle);   /* idle again */
    printf("PASS motion %s: hit animates the decay cell, then it rests\n", part == PART_909 ? "909" : "808");
}
int main(void) { test_dispatch(); test_motion(PART_909); test_motion(PART_808); printf("PASS ui-drum-glyph (%u checks)\n", checks); return 0; }
