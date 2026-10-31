/* Main-screen feedback regressions. Uses the production UI through the host simulator. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main

#include <assert.h>

static uint32_t main_hash(void)
{
    draw_main();
    return cv_hash();
}

/* Tallest MASTER hist column (band 0, left of GR). Screen y = MAIN_Y + canvas y. */
static int hist_tallest_col(void)
{
    int best = -1, best_n = -1, col;
    for (col = 0; col < 98; col++) {
        int sx = 4 + col * 2, n = 0, y;
        for (y = MAIN_Y; y < MAIN_Y + 69; y++) {
            if (sx < 200 && fb[y * 240 + (unsigned)sx] != 0)
                n++;
        }
        if (n > best_n) {
            best_n = n;
            best = col;
        }
    }
    return best;
}

/* Scrolling hist moved a spike left every sample and dirtied the full main blit (tear + knob lag).
 * Write-head must keep the spike's column fixed across later quiet samples. */
static void master_hist_write_head_stable(void)
{
    int i, col0, col1, lit;

    memset(&ui, 0, sizeof ui);
    boot();
    set_view(V_MIX);
    assert(pg.n >= 2);
    assert(!strcmp(pg.title[pg.n - 2], "MASTER"));     /* WAVE is last; MASTER is its sibling */
    assert(!strcmp(pg.title[pg.n - 1], "WAVE"));
    *page_of(V_MIX) = (uint8_t)(pg.n - 2);
    build_pages();
    now_ms = 1000;
    for (i = 0; i < NPARTS; i++)
        eng_peak[i] = 0;
    for (i = 0; i < 100; i++) {
        now_ms += 20;
        (void)main_hash();
    }
    eng_peak[0] = 30000;
    now_ms += 20;
    (void)main_hash();
    col0 = hist_tallest_col();
    assert(col0 >= 0 && col0 < 98);
    eng_peak[0] = 0;
    for (i = 0; i < 10; i++) {
        now_ms += 20;
        (void)main_hash();
    }
    col1 = hist_tallest_col();
    assert(col1 == col0);

    /* Past the old uint8_t write wrap: still a full-width hist (not a collapsed strip). */
    for (i = 0; i < 300; i++) {
        eng_peak[0] = (i & 1) ? 22000 : 4000;
        now_ms += 20;
        (void)main_hash();
    }
    lit = 0;
    for (i = 0; i < 98; i++) {
        int sx = 4 + i * 2, y, n = 0;
        for (y = MAIN_Y + 4; y < MAIN_Y + 66; y++)
            if (fb[y * 240 + (unsigned)sx] != 0)
                n++;
        if (n > 2)
            lit++;
    }
    assert(lit >= 90);
}

static void readout_does_not_cover_help(void)
{
    uint32_t help_only, help_with_pending_readout, normal, readout;

    memset(&ui, 0, sizeof ui);
    boot();
    ui.view = V_PART;
    ui.help = B_SAVE + 1;
    ui.touched = -1;
    help_only = main_hash();

    ui.touched = 0;
    ui.touch_until = now_ms + 900u;
    help_with_pending_readout = main_hash();
    assert(help_with_pending_readout == help_only);

    ui.help = 0;
    ui.touched = -1;
    normal = main_hash();
    ui.touched = 0;
    ui.touch_until = now_ms + 900u;
    readout = main_hash();
    assert(readout != normal);
}

static void read_only_rows_do_not_edit(void)
{
    static const pref_t rows[] = {
        PR(R_BATT, 0, 0, 0), PR(R_BTSTAT, 0, 0, 0), PR(R_BTNAME, 0, 0, 0)
    };
    project_t before;
    unsigned i;

    memset(&ui, 0, sizeof ui);
    boot();
    ui.dirty = 0;
    before = proj;
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        char num[16], unit[8];
        pref_value(rows[i], num, unit);
        assert(num[0] != 0);
        turn_ref(rows[i], 1, EN_ALGO);
        turn_ref(rows[i], -1, EN_ALGO);
        assert(!memcmp(&proj, &before, sizeof proj));
        assert(ui.dirty == 0);
    }

    {
        pref_t editable = PR(R_AUTOSAVE, 0, 0, 0);
        int old = pref_get(editable);
        turn_ref(editable, old ? -1 : 1, EN_ALGO);
        assert(pref_get(editable) != old);
        assert(ui.dirty == 1);
    }
}

/* MIX WAVE sibling: silent eng_scope is flat; a latched shape changes the main hash; MASTER
 * page index stays 3; WAVE is page 4 with the same master knobs. */
/* HOME pattern glyphs: two used patterns with different 909 hits must not paint the same fill. */
static void home_pattern_glyph_content(void)
{
    int lit0, lit1, sx0, sx1, y, i;

    memset(&ui, 0, sizeof ui);
    boot();
    pattern_init(&proj.pat[0], 1u, 2u);
    pattern_init(&proj.pat[1], 3u, 4u);
    sm_set(proj.pat[0].drum[PART_909].hit[0], 0);
    sm_set(proj.pat[0].drum[PART_909].hit[0], 8);
    sm_set(proj.pat[1].drum[PART_909].hit[0], 1);
    sm_set(proj.pat[1].drum[PART_909].hit[0], 2);
    sm_set(proj.pat[1].drum[PART_909].hit[0], 3);
    set_view(V_HOME);
    ui.pbank = 0;
    now_ms = 1000;
    (void)main_hash();
    sx0 = 4 + 2;
    sx1 = 4 + 14 + 2;
    y = MAIN_Y + 5;
    lit0 = lit1 = 0;
    for (i = 0; i < 8; i++) {
        if (fb[y * 240 + (unsigned)(sx0 + i)] != 0)
            lit0++;
        if (fb[y * 240 + (unsigned)(sx1 + i)] != 0)
            lit1++;
    }
    assert(lit0 == 2);
    assert(lit1 == 2);
    assert(fb[y * 240 + (unsigned)(sx0 + 4)] != 0);
    assert(fb[y * 240 + (unsigned)(sx1 + 1)] != 0);
    assert(fb[y * 240 + (unsigned)(sx0 + 1)] == 0);
    assert(fb[y * 240 + (unsigned)(sx1 + 4)] == 0);
}

static void mix_wave_sibling_scope(void)
{
    uint32_t silent, moving, master_hash;
    unsigned i;

    memset(&ui, 0, sizeof ui);
    boot();
    set_view(V_MIX);
    assert(pg.n == 5);
    assert(!strcmp(pg.title[3], "MASTER"));
    assert(!strcmp(pg.title[4], "WAVE"));
    assert(pg.r[3][1].c == MST_COMP1 && pg.r[4][1].c == MST_COMP1);

    *page_of(V_MIX) = 4;
    build_pages();
    for (i = 0; i < SCOPE_N; i++)
        eng_scope[i] = 0;
    eng_scope_w = 0;
    now_ms = 1000;
    silent = main_hash();

    for (i = 0; i < SCOPE_N; i++)
        eng_scope[i] = (int16_t)((i & 1u) ? 22000 : -22000);
    eng_scope_w = SCOPE_N;
    now_ms += 100;
    moving = main_hash();
    assert(moving != silent);

    *page_of(V_MIX) = 3;
    build_pages();
    now_ms += 100;
    master_hash = main_hash();
    *page_of(V_MIX) = 4;
    build_pages();
    now_ms += 100;
    assert(main_hash() != master_hash);
}

int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "readout"))
        readout_does_not_cover_help();
    if (argc == 1 || !strcmp(argv[1], "read-only"))
        read_only_rows_do_not_edit();
    if (argc == 1 || !strcmp(argv[1], "wave"))
        mix_wave_sibling_scope();
    if (argc == 1 || !strcmp(argv[1], "master-hist"))
        master_hist_write_head_stable();
    if (argc == 1 || !strcmp(argv[1], "pat-glyph"))
        home_pattern_glyph_content();
    puts("PASS: help hides pending readout, read-only rows reject edits, MIX WAVE scope draws, MASTER hist write-head stable, HOME pattern glyphs");
    return 0;
}
