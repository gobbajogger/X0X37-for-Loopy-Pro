/* SPDX-License-Identifier: GPL-3.0-only */
/* Count real primitive calls, then compare production and frozen old RGB565 paths. */
static void *measured_function;
static unsigned measured_calls;
void __cyg_profile_func_enter(void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *fn, void *caller)
{ (void)caller; if (fn == measured_function) ++measured_calls; }
void __cyg_profile_func_exit(void *fn, void *caller) { (void)fn; (void)caller; }
#define SMC_UI_MAIN smc_ui_fixture_main
#include "smc_ui_test.c"
static uint16_t expected_pixels[240 * MAIN_ROWS];
/* Independent model panel: the old raster writes its complete header here,
 * never into candidate fb or tile/snapshot metadata. */
static void reference_header_commit(int region, uint32_t x, uint32_t y)
{ (void)region; (void)x; (void)y; memcpy(expected_pixels, cv_px, 240 * HDR_H * 2); }
#define cv_commit reference_header_commit
#include "reference/ui_perf_reference.inc"
#undef cv_commit

static void compare_header(void)
{
    ui_t original_ui = ui;
    draw_header_reference();
    ui = original_ui; /* expiry and other mutations must see the same input */
    draw_header();
    CHECK(!memcmp(expected_pixels, fb, 240 * HDR_H * 2));
}

static void pattern_badges(void)
{
    setup(V_HOME, PART_909);
    seq.playing = seq.song_on = seq.chain_a = seq.chain_b = 0;
    for (int p = NPAT - 2; p < NPAT; ++p) pattern_init(&proj.pat[p], 1, 2);
    proj.set.accent_q7 = 127;
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0], 0);
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0], 8);
    sm_set(proj.pat[NPAT-2].drum[PART_909].hit[0], 2);
    seq.ppat[PART_909] = NPAT - 1;
    seq.pcue[PART_909] = NPAT - 2;
    static project_t before;
    before = proj;
    for (int theme = 0; theme <= 5; theme += 5) {
        palette_set(theme); ui.msg_until = 0; draw_header();
        /* Fixed expectations independent of the shared badge renderer:
         * current steps {0,8} map to columns {0,4}; queued {2} to {1}. */
        for (int i = 0; i < 8; ++i) for (int y = 8; y <= 10; ++y) {
            uint16_t current = (0x11u & (1u << i)) ? dim(PART_COL[PART_909],16) : SOUND_FACE;
            uint16_t queued = (0x02u & (1u << i)) ? dim(PART_COL[PART_909],9) : SOUND_FACE;
            CHECK(fb[y*240+133+i] == swap16(current));
            CHECK(fb[y*240+165+i] == swap16(queued));
        }
        memcpy(expected_pixels, fb, 240 * HDR_H * 2);
        now_ms += 1000; ui.frame += 32; draw_header();
        CHECK(!memcmp(expected_pixels, fb, 240 * HDR_H * 2));
        shot(theme ? "header-content-light" : "header-content-dark");
    }
    CHECK(!memcmp(&before, &proj, sizeof proj));
    puts("PASS pattern badges: fixed saved/queued content columns, dark/light, stopped stability, read-only");
}

static void arcs(void)
{
    for (int radius = 7; radius <= 9; ++radius)
    for (int theme = 0; theme < 6; ++theme)
    for (int bipolar = 0; bipolar < 2; ++bipolar)
    for (int value = -1; value <= 128; ++value) {
        palette_set(theme);
        int x = value % 3 == 0 ? 0 : value % 3 == 1 ? 120 : 239;
        cv_begin(240, 69, C_LINE);
        measured_function = (void *)fm_sqrtf; measured_calls = 0;
        arc_reference(x, 35, radius, value / 127.0f, C_GRAY, C_HI, bipolar);
        unsigned old_calls = measured_calls;
        memcpy(expected_pixels, cv_px, 240 * 69 * 2);
        cv_begin(240, 69, C_LINE);
        measured_calls = 0;
        arc(x, 35, radius, value / 127.0f, C_GRAY, C_HI, bipolar);
        unsigned new_calls = measured_calls;
        CHECK(!memcmp(expected_pixels, cv_px, 240 * 69 * 2));
        CHECK(radius == 8 ? new_calls + 361 == old_calls : new_calls == old_calls);
    }
    puts("PASS arc: 4680 states, exact canvas bytes; radius 8 removes 361 square roots, generic fallback identical");
}
static void headers(void)
{
    for (int theme = 0; theme < 6; ++theme)
    for (int part = 0; part < NPARTS; ++part) {
        setup(V_PART, part); palette_set(theme);
        for (int state = 0; state < 32; ++state) {
            ui.rec = state & 1; seq.playing = (state >> 1) & 1; seq.metro_view = (uint8_t)(state % 9);
            ui.dirty = !!(state & 4); ui.save_failed = !!(state & 8);
            *page_of(V_PART) = (uint8_t)(state % pg.n); build_pages();
            now_ms += 100;
            ui.msg_until = 0; ui.help = 0; ui.overlay = O_NONE;
            seq.pcue[part] = 0xff;
            compare_header();
            if (part != PART_BRK && !division_pad_owner()) {
                measured_function = (void *)cv_begin; measured_calls = 0;
                draw_header();
                CHECK(measured_calls == 0); /* unchanged snapshot skips rebuild/hash */
                CHECK(!memcmp(expected_pixels, fb, 240 * HDR_H * 2));
            }
            /* Change one input at a time while candidate reuse stays live. The
             * reference neither preloads its panel nor invalidates its cache. */
            ui.rec ^= 1; compare_header();
            seq.playing ^= 1; compare_header();
            seq.metro_view = (uint8_t)((seq.metro_view + 1) % 9); compare_header();
            ui.save_failed ^= 1; compare_header();
            ui.dirty ^= 1; compare_header();
            *page_of(V_PART) = (uint8_t)((cur_page() + 1) % pg.n); build_pages(); compare_header();
            palette_set((theme + 1) % 6); compare_header();
            palette_set(theme); compare_header();
            /* Another header owner must invalidate reuse even if inputs match. */
            cv_begin(240, HDR_H, C_BLACK); cv_rect(0, 0, 240, HDR_H, C_AMB);
            cv_commit(0, 0, 0); compare_header();
            /* Other header owners, invalidation, help, questions and exact toast expiry. */
            for (int mode = 0; mode < 6; ++mode) {
                if (mode == 0) blit_hash[0] = 0;
                if (mode == 1) { ui.help = 1; }
                if (mode == 2) { ui.help = 0; ui.overlay = O_ASK; }
                if (mode == 3) { ui.overlay = O_NONE; say("RESET SOUND", 0); }
                if (mode == 4) now_ms = ui.msg_until;
                if (mode == 5) { seq.pcue[part] = 1; ui.frame ^= 16; }
                compare_header();
            }
        }
    }
    puts("PASS header: independent donor-aligned raster states and metro-phase transitions, byte equality, live cache input/owner/expiry transitions");
}
static void waves(void)
{
    setup(V_MIX, PART_909);
    for (int state = 0; state < 240; ++state) {
        palette_set(state % 6); ui.part = (uint8_t)(state % NPARTS);
        eng_scope_w = (uint32_t)(state * 7) & (SCOPE_N - 1u);
        for (unsigned i = 0; i < SCOPE_N; ++i)
            eng_scope[i] = state % 3 == 0 ? 0 : state % 3 == 1 ? (int16_t)(i * 997 + state) : 32000;
        now_ms += state % 2 ? 100 : 99; /* both sides of the unchanged latch cadence */
        cv_begin(240, MAIN_ROWS, C_BLACK); cv_band(0, BAND_H);
        measured_function = (void *)cv_line; measured_calls = 0;
        draw_master_wave_reference(); unsigned old_calls = measured_calls;
        memcpy(expected_pixels, cv_px, sizeof expected_pixels);
        cv_begin(240, MAIN_ROWS, C_BLACK); cv_band(0, BAND_H);
        measured_calls = 0; draw_master_wave();
        CHECK(!memcmp(expected_pixels, cv_px, sizeof expected_pixels));
        CHECK(measured_calls == old_calls || measured_calls + 186 == old_calls);
        if (state == 0) CHECK(measured_calls + 186 == old_calls);
    }
    puts("PASS WAVE: 240 states, exact canvas/latch cadence; silent plot collapses 187 line calls to one");
}
int main(void)
{
    const char *which = getenv("UI_PERF_CASE");
    if (!which || !strcmp(which, "badges")) pattern_badges();
    if (!which || !strcmp(which, "arc")) arcs();
    if (!which || !strcmp(which, "header")) headers();
    if (!which || !strcmp(which, "wave")) waves();
    printf("ui-perf-identity: PASS (%u checks)\n", checks);
    return 0;
}
