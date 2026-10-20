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

int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "readout"))
        readout_does_not_cover_help();
    if (argc == 1 || !strcmp(argv[1], "read-only"))
        read_only_rows_do_not_edit();
    puts("PASS: help hides pending readout and read-only rows reject edits");
    return 0;
}
