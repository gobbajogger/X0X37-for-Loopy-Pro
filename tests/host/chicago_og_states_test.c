/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago PART pages 4-7 must not restore the program every UI frame.
 * og_states used to scan pot 0..127 through pref_value_of (386 restores
 * a frame). The glyph now uses the same cached fields as the readout. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main

static uint32_t g_restore_chi, g_chi_init;
int __real_original_sound_restore_chicago(void *out, const uint8_t *cell);
int __wrap_original_sound_restore_chicago(void *out, const uint8_t *cell)
{ g_restore_chi++; return __real_original_sound_restore_chicago(out, cell); }
int __real_original_chicago_program_init(void *p, unsigned factory);
int __wrap_original_chicago_program_init(void *p, unsigned factory)
{ g_chi_init++; return __real_original_chicago_program_init(p, factory); }

static int adopt(int voice, int bank)
{
    if (engine_bass_select_preset(voice, bank, 0, &proj.sound)) return -1;
    for (int i = 0; i < 8000 && engine_bass_preset_pending(voice); i++) {
        int32_t a[512]; engine_render(a, 256);
    }
    return (engine_bass_sound_bank(&proj.sound, voice) == bank &&
            engine_bass_bank(voice) == bank) ? 0 : -1;
}

static void settle(int part, int page)
{
    ui.view = V_PART; ui.part = (uint8_t)part; ui.overlay = O_NONE; ui.help = 0;
    ui.touched = -1; ui.touch_until = ui.msg_until = 0; ui.msg[0] = 0;
    ui.held_step = ui.hold_step = -1; ui.btn = ui.keys = ui.rec = 0;
    ui.page[V_PART][part] = (uint8_t)page;
    set_view(V_PART);
    ui.page[V_PART][part] = (uint8_t)page;
    build_pages();
    *page_of(V_PART) = (uint8_t)page;
}

/* This is the pre-fix scan, run ONLY in the host test. It is a reference
 * oracle for every displayed value, never used by the shipping UI. */
static int legacy_state_table(pref_t r, int index[128])
{
    char a[16], u[8], prev[16] = "";
    int n = 0;
    for (int v = 0; v < 128; v++) {
        pref_value_of(r, v, a, u);
        if (strcmp_s(a, prev)) { n++; put_s(prev, a); }
        index[v] = n - 1;
    }
    return n;
}

/* Reference pixels for the two actual pre-fix glyph branches. Do not compare
 * only their labels: a swapped or missing bar must fail the framebuffer check. */
static int legacy_glyph_pixels(pref_t r, int value, int n, int idx)
{
    static uint16_t actual[240 * KNOB_H];
    const int gx = 10, gy = 10, top = gy + 3;
    const int bot = gy + OG_H, mid = gy + OG_H / 2;
    unsigned state = 6, held = 0;
    float amp = 0, mod = 0;
    if (engine_bass_voice_view(r.b, &amp, &mod, &state, &held) && state != 6) {
        puts("PIXEL_TEST_NOT_IDLE");
        return 0; /* The legacy reference here intentionally excludes live pips. */
    }
    cv_begin(240, KNOB_H, C_BLACK);
    orig_strip_glyph(r, value, gx, gy, C_HI);
    memcpy(actual, cv_px, sizeof actual);

    cv_begin(240, KNOB_H, C_BLACK);
    if (r.c == 11) {
        cv_rect(gx, bot, OG_W, 1, C_LINE);
        if (idx) {
            cv_line(gx + 2, bot, gx + 2, top, C_HI);
            cv_line(gx + 2, top, gx + 22, top, C_HI);
            cv_line(gx + 22, top, gx + 22, bot, C_HI);
        } else {
            cv_line(gx + 1, bot, gx + 5, top, C_HI);
            cv_line(gx + 5, top, gx + 10, mid, C_HI);
            cv_line(gx + 10, mid, gx + 18, mid, C_HI);
            cv_line(gx + 18, mid, gx + 24, bot, C_HI);
        }
    } else {
        if (!n) return 0;
        int sw = OG_W / n;
        for (int k = 0; k < n; k++) {
            int h = 3 + k * (OG_H - 3) / (n > 1 ? n - 1 : 1);
            cv_rect(gx + k * sw, bot - h + 1, sw - 2, h, k <= idx ? C_HI : C_LINE);
        }
    }
    return !memcmp(actual, cv_px, sizeof actual);
}

static int check_states_and_pixels(void)
{
    int failures = 0;
    unsigned pixels = 0;
    for (int control = 11; control <= 15; control += 4) {
        int expected[128];
        pref_t r = PR(R_ENG, T_303, 1, control);
        original_controls.valid = 0;
        g_restore_chi = g_chi_init = 0;
        int old_n = legacy_state_table(r, expected);
        uint32_t legacy_restores = g_restore_chi, legacy_inits = g_chi_init;
        if (legacy_restores < 64 || legacy_inits < 64 || old_n != (control == 11 ? 2 : 3)) {
            printf("REFERENCE_FAIL c=%d states=%d scan_restore=%u scan_init=%u\n",
                   control, old_n, legacy_restores, legacy_inits);
            failures++;
            continue;
        }
        for (int value = 0; value < 128; value++) {
            int idx = -1;
            original_controls.valid = 0; /* Cold: one view, not 128 restores. */
            g_restore_chi = g_chi_init = 0;
            int n = og_states(r, value, &idx);
            if (n != old_n || idx != expected[value] ||
                g_restore_chi != 1 || g_chi_init != 1) {
                printf("CONTROL_FAIL c=%d value=%d old=%d/%d new=%d/%d restore=%u init=%u\n",
                       control, value, old_n, expected[value], n, idx,
                       g_restore_chi, g_chi_init);
                failures++;
                break;
            }
            g_restore_chi = g_chi_init = 0;
            if (!legacy_glyph_pixels(r, value, old_n, expected[value]) ||
                g_restore_chi || g_chi_init) {
                printf("FRAMEBUFFER_FAIL c=%d value=%d restore=%u init=%u\n",
                       control, value, g_restore_chi, g_chi_init);
                failures++;
                break;
            }
            pixels++;
        }
        printf("CONTROL c=%d legacy_scan_restore=%u legacy_scan_init=%u "
               "states=%d\n", control, legacy_restores, legacy_inits, old_n);
    }
    printf("GLYPH_PIXEL_EQUAL %u/256 cases\n", pixels);
    return failures + (pixels != 256);
}

int main(void)
{
    int fails = 0;
    load_force = 0;
    boot();
    now_ms = 1000;
    proj.set.autosave_off = 1;
    if (adopt(0, ORIGINAL_SOUND_MIAMI) || adopt(1, ORIGINAL_SOUND_CHICAGO)) {
        printf("ADOPT_FAIL\n");
        return 1;
    }
    /* Verify that --wrap is live: an unhooked zero counter is a false PASS. */
    {
        pref_t r = PR(R_ENG, T_303, 1, 11);
        char a[16], u[8];
        original_controls.valid = 0;
        g_restore_chi = g_chi_init = 0;
        pref_value_of(r, 0, a, u);
        pref_value_of(r, 127, a, u);
        printf("WRAP_PROBE restore=%u init=%u\n", g_restore_chi, g_chi_init);
        if (!g_restore_chi || !g_chi_init) { puts("WRAP_INACTIVE"); fails++; }
    }
    fails += check_states_and_pixels();
    for (int p = 0; p < 8; p++) {
        settle(PART_303B, p);
        original_controls.valid = 0;
        g_restore_chi = g_chi_init = 0;
        now_ms += 16; ui_frame(); /* Cold redraw: check the actual page path. */
        uint32_t first_restore = g_restore_chi, first_init = g_chi_init;
        if (p >= 4 && first_restore >= 386) {
            printf("COLD_RESTORE_REGRESSION p=%d restore=%u\n", p, first_restore);
            fails++;
        }
        g_restore_chi = g_chi_init = 0;
        for (int f = 0; f < 8; f++) { now_ms += 16; ui_frame(); }
        printf("CHICAGO p=%d '%s' cold_restore=%u cold_init=%u steady8_restore=%u steady8_init=%u\n",
               p, pg.title[p], first_restore, first_init, g_restore_chi, g_chi_init);
        if (g_restore_chi || g_chi_init) {
            printf("FAIL: expected 0 restores/inits on a steady frame\n");
            fails++;
        }
    }
    /* Miami must stay at zero (untouched path). */
    for (int p = 0; p < 6; p++) {
        settle(PART_303A, p);
        now_ms += 16; ui_frame();
        g_restore_chi = g_chi_init = 0;
        for (int f = 0; f < 8; f++) { now_ms += 16; ui_frame(); }
        if (g_restore_chi || g_chi_init) {
            printf("FAIL Miami p=%d restore=%u\n", p, g_restore_chi);
            fails++;
        }
    }
    printf(fails ? "CHICAGO OG_STATES FAIL %d\n" : "CHICAGO OG_STATES PASS\n", fails);
    return fails ? 1 : 0;
}
