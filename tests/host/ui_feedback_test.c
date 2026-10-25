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
    puts("PASS: help hides pending readout, read-only rows reject edits, MIX WAVE scope draws");
    return 0;
}
