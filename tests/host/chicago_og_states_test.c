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
    for (int p = 0; p < 8; p++) {
        settle(PART_303B, p);
        now_ms += 16; ui_frame();               /* first paint may fill the cache */
        g_restore_chi = g_chi_init = 0;
        for (int f = 0; f < 8; f++) { now_ms += 16; ui_frame(); }
        printf("CHICAGO p=%d '%s' steady8_restore=%u steady8_init=%u\n",
               p, pg.title[p], g_restore_chi, g_chi_init);
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
