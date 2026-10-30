/* SPDX-License-Identifier: GPL-3.0-only */
/* Real queued DAW/MMC input -> production SMC/UI/engine. Pad strikes use the
 * production SMK pad mapper and UI input(), with host hardware substitutes.
 * Pattern writes, released-pad lights, emitted SMC lights and rendered audio
 * are observed. No radio/HCI, target timing or physical-device claim is made. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include X0X_HOST_SOURCE
#undef main
#include <assert.h>
#include "dsp/drum909.h"
#include "dsp/drum808.h"

static drum909_t *observed_909;
void smc_test_drum909_set(drum909_t *d, int voice, int param, int value)
{
    observed_909 = d;
    drum909_set(d, voice, param, value);
}

static uint8_t clip_grid_mode, smk_pad_piano, smk_pad_grid = 1;
uint8_t smk_clip_mode_on(void) { return clip_grid_mode; }
int smk_ck_smc_turn(int n, int d) { (void)n; (void)d; return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }

static uint8_t wire_led[128];
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    assert(len == 3);
    if (msg[0] == 0x90) wire_led[msg[1]] = msg[2];
    return 1;
}
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include SMC_MIXER_SOURCE

#define SMK_KEY_F3 5
#define SMK_WHITE_OF_PAD(p) (smk_pad_grid ? (p) : (p) < 8u ? (p) + 8u : (p) - 8u)
static const uint8_t SMK_WHITE_NOTE[16] = {0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const int8_t SMK_PIANO_NOTE[16] = {7,9,11,12,14,16,18,19,8,10,-1,13,15,17,-1,20};
static uint8_t smk_pad_note[16];
static struct {
    uint32_t pnotes, knotes, notes, notes_pressed;
    uint8_t key_vel[37], pad_vel[16];
} fm1_in;
#include "smk_pad_route.inc"

static unsigned checks, failures, destinations, inactive, pad_writes, audio_cases;
static const char *context;
static int strip_view = V_MIX;
#define CHECK(x) do { checks++; if (!(x)) { failures++; \
    fprintf(stderr, "FAIL %s line %d: %s\n", context, __LINE__, #x); } } while (0)

static void render(int blocks)
{
    int32_t out[512];
    while (blocks-- > 0) engine_render(out, 256);
}

static int32_t peak(int blocks)
{
    int32_t out[512], maximum = 0;
    while (blocks-- > 0) {
        engine_render(out, 256);
        for (int i = 0; i < 512; i++) {
            int32_t a = out[i] < 0 ? -out[i] : out[i];
            if (a > maximum) maximum = a;
        }
    }
    return maximum;
}

static void poll(void) { now_ms += 20; smc_poll(); smc_frame(); }
static void message(int status, int note, int value)
{
    const uint8_t raw[3] = {status, note, value};
    smc_ble_midi(raw, sizeof raw);
    poll();
}
static void press(int note) { message(0x90, note, 127); }
static void release(int note) { message(0x80, note, 0); }
static void tap(int note) { press(note); release(note); }
static void mmc(int note, int host)
{
    int cmd;
    for (cmd = 1; cmd <= 43 && smc_mmc_note(cmd) != note; cmd++) {}
    assert(cmd <= 43);
    if (host >= 0) smc_mmc_host((uint8_t)cmd, (uint8_t)host);
    else {
        const uint8_t raw[6] = {0xF0, 0x7F, 0x7F, 6, (uint8_t)cmd, 0xF7};
        smc_ble_midi(raw, sizeof raw);
    }
    poll();
}

static void view(int kit, int bank)
{
    if (strip_view == V_SEQ) ui.part = (uint8_t)kit;
    smc_view(strip_view, strip_view == V_MIX ? kit + 1 : 0);
    ui.sel[kit] = (uint8_t)(bank * 8);
    build_pages();
    smc_frame();
}
static void setup(int kit, int bank)
{
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    memset(&smc, 0, sizeof smc);
    memset(&fm1_in, 0, sizeof fm1_in);
    memset(smk_pad_note, 0, sizeof smk_pad_note);
    memset(wire_led, 0xFF, sizeof wire_led);
    clip_grid_mode = smk_pad_piano = 0;
    boot();
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link(1);
    poll();
    seq.mute = seq.vmute = 0;
    engine_smc_solo(0);
    for (int p = 0; p < NPARTS; p++) seq.ppat[p] = (uint8_t)(p + 1);
    for (int p = 0; p < NPAT; p++) for (int k = 0; k < NKIT; k++) {
        memset(proj.pat[p].drum[k].hit, 0, sizeof proj.pat[p].drum[k].hit);
        proj.pat[p].drum[k].len = 32;
    }
    pref_set(PR(R_ENG, T_FX, 0, FX_RV_LEVEL), 0);
    pref_set(PR(R_ENG, T_FX, 0, FX_DL_LEVEL), 0);
    if (kit >= 0) view(kit, bank);
    else smc_view(V_HOME, 0);
    render(2);
    smc_frame();
}

static void pad(int physical, int down)
{
    smk_grid();
    smk__padq((uint32_t)physical, down ? 100u : 0u);
    held_keys = fm1_in.notes;
    now_ms += 20;
    input();
}
static void pad_tap(int physical) { pad(physical, 1); pad(physical, 0); }

static void only_hit(int kit, int track, int step)
{
    for (int p = 0; p < NPAT; p++) for (int k = 0; k < NKIT; k++)
        for (int t = 0; t < NDRUM; t++) for (int w = 0; w < 2; w++) {
            uint32_t expected = p == seq.ppat[kit] && k == kit && t == track && w == step / 32
                ? 1u << (step % 32) : 0;
            CHECK(proj.pat[p].drum[k].hit[t][w] == expected);
        }
}

static void destination(int kit, int bank, int strip)
{
    char label[96];
    int track = bank * 8 + strip;
    snprintf(label, sizeof label, "%s %s bank%d strip%d", strip_view == V_MIX ? "MIX" : "SEQ",
             PART_N[kit], bank + 1, strip + 1);
    context = label;
    setup(kit, bank);
    if (strip_view == V_MIX) ui.part = PART_303B; /* visible kit deliberately differs from selected part */
    int initial_part = ui.part;
    smc_frame();
    tap(strip);
    if (track >= NDRUM) {
        CHECK(ui.part == initial_part && !ui.rec && ui.sel[kit] == 8 && ui.view == strip_view);
        CHECK(ui.overlay == O_NONE);
        for (int row = 0; row < 4; row++) {
            tap(row * 8 + strip);
            CHECK(!wire_led[row * 8 + strip]);
        }
        CHECK(!smc_project_active() && !seq.mute && !seq.vmute);
        CHECK(ui.part == initial_part && !ui.rec && ui.sel[kit] == 8 && ui.view == strip_view);
        sound_t sound = proj.sound;
        message(0xB0, 16 + strip, 1);
        message(0xE0 + strip, 0, 127);
        CHECK(!memcmp(&sound, &proj.sound, sizeof sound));
        inactive++;
        return;
    }
    destinations++;
    CHECK(ui.part == kit && ui.sel[kit] == track && ui.rec);
    CHECK(ui.view == strip_view && (strip_view != V_MIX || drum_mix_page() == kit) && ui.overlay == O_NONE);
    CHECK(wire_led[strip] == 127 && wire_led[24 + strip] == 127 && wire_led[0x5F] == 127);
    if (ui.part != kit || ui.sel[kit] != track || ui.view != strip_view) return;
    ui.spage = (uint8_t)(track & 1);
    int physical = (track * 3) % 16, step = ui.spage * 16 + physical;
    pad_tap(physical);
    only_hit(kit, track, step);
    leds();
    CHECK(lit_keys & (1u << WHITE_KEY[physical]));
    pad_writes++;
    pad_tap(physical);              /* tapping an existing step removes it */
    CHECK(!sm_any(proj.pat[seq.ppat[kit]].drum[kit].hit[track]));
    leds();
    CHECK(!(lit_keys & (1u << WHITE_KEY[physical])));
    tap(strip);
    CHECK(!ui.rec && ui.part == kit && ui.sel[kit] == track && !wire_led[strip]);

    /* SELECT also focuses the real kit; pad sequencing works with REC off. */
    if (strip_view == V_MIX) ui.part = PART_303A;
    ui.sel[kit] = (uint8_t)(bank * 8 + (strip + 1) % (bank ? 3 : 8));
    tap(24 + strip);
    CHECK(ui.part == kit && ui.sel[kit] == track && !ui.rec && ui.view == strip_view);
    pad_tap(physical);
    only_hit(kit, track, step);
    pad_writes++;

    seq.mute = 0x80u;
    seq.vmute = 0x400u;
    press(8 + strip);
    CHECK(smc_project_active() && wire_led[8 + strip] == 127);
    CHECK(seq.mute == 0x80u && seq.vmute == 0x400u);
    message(0x90, 8 + strip, 0);     /* note-on velocity zero is a release, too */
    CHECK(!smc_project_active() && !wire_led[8 + strip]);
    CHECK(seq.mute == 0x80u && seq.vmute == 0x400u);

    if (strip_view == V_SEQ) {
        /* SEQ owns this drum: level fader, independent pan knob, and MUTE. */
        pref_t r = find_ref(kit ? T_808 : T_909, track, "Level");
        CHECK(smc_same(r, smc_target(strip)));
        pref_t pan = find_ref(kit ? T_808 : T_909, track, "Pan");
        CHECK(smc_same(pan, smc_knob_target(strip)));
        const x0x_param_t *d = pref_desc(r);
        pref_set(r, d->max / 2);
        smc_frame();
        int level = pref_get(r), before = pref_get(pan);
        message(0xB0, 16 + strip, 1);
        CHECK(pref_get(pan) > before && pref_get(r) == level);
        message(0xE0 + strip, 0, smc_scaled(r));
        message(0xE0 + strip, 0, 0);
        CHECK(pref_get(r) == 0);
        seq.mute = seq.vmute = 0;
        tap(16 + strip);
        CHECK(seq.vmute == (1u << (kit * NDRUM + track)) && !seq.mute);
        CHECK(wire_led[16 + strip] == 127);
        tap(16 + strip);
        CHECK(!seq.vmute && !wire_led[16 + strip]);
    }
}

static void banks_and_owners(void)
{
    context = "MIX/SEQ bank and channel navigation retain the selected kit";
    for (int mode = 0; mode < 2; mode++) for (int kit = 0; kit < NKIT; kit++) {
        strip_view = mode ? V_SEQ : V_MIX;
        setup(kit, 0);
        tap(0x2F);
        CHECK(ui.part == kit && ui.sel[kit] == 8 && ui.view == strip_view);
        CHECK(smc_target(0).b == 8 && smc_target(3).kind == R_NONE);
        tap(0x31);
        CHECK(ui.sel[kit] == 9);
        tap(0x30);
        CHECK(ui.sel[kit] == 8);
        tap(0x2E);
        CHECK(ui.sel[kit] == 0 && smc_target(7).b == 7);
        ui.help = 1;
        tap(0);
        /* SMC-02: help is a card, not an input owner. REC still toggles. */
        CHECK(ui.help && ui.rec && ui.sel[kit] == 0);
        tap(0);
        CHECK(ui.help && !ui.rec && ui.sel[kit] == 0);
        tap(6);                       /* R7 alone closes the help card. */
        CHECK(!ui.help && !ui.rec && ui.sel[kit] == 0);
        ui.overlay = O_ASK;
        tap(0);
        CHECK(!ui.rec && ui.sel[kit] == 0);
    }
    strip_view = V_MIX;
    context = "303 SEQ retains the main mixer map";
    setup(-1, 0);
    ui.part = PART_303A;
    smc_view(V_SEQ, 0);
    for (int strip = 0; strip < 8; strip++) CHECK(smc_same(smc_target(strip), smc_home_target(strip)));
    tap(1);
    CHECK(ui.part == PART_808 && ui.rec); /* unchanged main-map Record before entering a drum context */
}

static void solo_transitions(void)
{
    context = "held Solo target across bank/kit/page changes";
    setup(0, 0);
    press(8); press(9);
    CHECK(wire_led[8] == 127 && wire_led[9] == 127);
    tap(0x2F);
    CHECK(!wire_led[8] && !wire_led[9] && smc_project_active());
    press(8);                       /* repeated key-down must not retarget a held key */
    view(1, 1);
    press(10);                      /* hold an 808 bank2 drum beside the original 909s */
    CHECK(wire_led[10] == 127);
    release(8); release(9);
    CHECK(smc_project_active() && wire_led[10] == 127);
    smc_view(V_HOME, 0);
    release(10);
    CHECK(!smc_project_active());
    view(0, 0);
    for (int i = 8; i < 16; i++) CHECK(!wire_led[i]);

    context = "main part and drum Solo union";
    setup(-1, 0);
    press(8 + PART_303A);
    view(0, 0); press(8);
    release(8 + PART_303A);
    CHECK(smc_project_active() && wire_led[8] == 127);
    release(8);
    CHECK(!smc_project_active());

    for (int strip = 5; strip < 8; strip++) {
        context = "held S6-S8 repeat cannot become navigation on another page";
        setup(0, 0);
        press(8 + strip);
        smc_view(V_PART, 0);
        press(8 + strip);
        CHECK(ui.view == V_PART && smc_project_active());
        release(8 + strip);
        CHECK(!smc_project_active());
    }

    for (int route = -1; route < 2; route++) {
        context = "User MMC drum Solo latch and transport routes";
        setup(1, 1);
        proj.set.bt |= PROJ_BT_SMC_USER;
        mmc(10, route);
        CHECK(smc_project_active() && wire_led[10] == 127);
        mmc(10, route);
        CHECK(!smc_project_active() && !wire_led[10]);
        mmc(2, route);
        CHECK(ui.part == 1 && ui.sel[1] == 10 && ui.rec);
    }
}

static void cleanup(void)
{
    static const char *const names[] = {"disconnect", "coalesced reconnect", "help", "confirmation",
                                       "performance owner", "project reset"};
    for (int kind = 0; kind < 6; kind++) {
        context = names[kind];
        setup(1, 0);
        press(15);                 /* former PART shortcut is the visible drum's Solo */
        CHECK(smc_project_active());
        seq.mute = 0x80u; seq.vmute = 0x400u;
        if (kind == 0) smc_ble_link(0);
        if (kind == 1) { smc_ble_link(0); smc_ble_link(1); }
        if (kind == 2) ui.help = 1;
        if (kind == 3) ui.overlay = O_ASK;
        if (kind == 4) perf_test_start();
        if (kind == 5) {
            int rc = ui_project_begin();
            CHECK(rc == UI_PROJECT_BUSY);
            if (!rc) ui_project_end(0); /* negative control must not leave the engine held */
            smc_project_reset();
        }
        poll();
        /* Help keeps live controls/Solo; the other owners clear it. */
        CHECK(kind == 2 ? smc_project_active() : !smc_project_active());
        CHECK(kind == 4 || (seq.mute == 0x80u && seq.vmute == 0x400u));
        if (kind == 4) {
            button_tap(B_HOME);
            render(1);
            perf_test_tick();
            CHECK(!perf_testing());
        }
        ui.help = 0; ui.overlay = O_NONE;
        if (kind == 0) { smc_ble_link(1); poll(); }
        release(15);
        CHECK(!smc_project_active());
    }
}

static void pad_release_context(void)
{
    for (int mode = 0; mode < 3; mode++) {
        context = "held pad release cannot edit the newly selected drum";
        setup(0, 0);
        ui.sel[0] = 1; build_pages(); /* select a different sound, not the new return-to-main gesture */
        tap(24);
        uint32_t *old = proj.pat[seq.ppat[0]].drum[0].hit[0];
        uint32_t *next = proj.pat[seq.ppat[0]].drum[0].hit[mode == 2 ? 8 : 1];
        sm_set(old, 0); sm_set(next, 0);
        pad(0, 1);
        tap(mode == 0 ? 1 : mode == 1 ? 25 : 0x2F);
        pad(0, 0);
        CHECK(sm_get(next, 0));
        CHECK(ui.view == V_MIX && !held_keys);
    }
    context = "main MIX and FX pads retain no-edit ownership";
    for (int v = 0; v < 2; v++) {
        setup(-1, 0);
        smc_view(v ? V_FX : V_MIX, 0);
        pad_tap(0);
        CHECK(!sm_any(proj.pat[seq.ppat[0]].drum[0].hit[0]));
    }
}

static void held_edit_context(void)
{
    static lane_t before[NLANE];
    static const int actions[] = {1, 25, 0x2F, 0x31};
    context = "drum target change cancels a held pad before probability edits";
    strip_view = V_SEQ;
    for (int kit = 0; kit < NKIT; kit++) for (int action = 0; action < 4; action++) {
        setup(kit, 0);
        sm_set(pat_of(kit)->drum[kit].hit[0], 3);
        pad(3, 1);
        tap(actions[action]);
        CHECK(ui.sel[kit] == (action == 2 ? 8 : 1));
        memcpy(before, proj.arr.lane, sizeof before);
        enc_acc[EN_SELECT] = -1;       /* 100 -> 95 would create a wrong-track probability lock */
        now_ms += 20;
        input();
        CHECK(!memcmp(before, proj.arr.lane, sizeof before));
        pad(3, 0);
    }
    strip_view = V_MIX;

    context = "pattern cue during held p-lock restores the original drum sound";
    setup(0, 0);
    ui.part = PART_909;
    ui.sel[PART_909] = ui.spage = 0;
    smc_view(V_PART, 0);
    pref_t r = pg.r[cur_page()][0];
    CHECK(r.kind == R_ENG && r.a == T_909);
    if (r.kind != R_ENG || r.a != T_909) return;
    pref_set(r, pref_desc(r)->max / 2);
    render(2);
    int base = pref_get(r), old_pattern = seq.ppat[PART_909];
    CHECK(observed_909 && drum909_get(observed_909, r.b, r.c) == base);
    sm_set(pat_of(PART_909)->drum[PART_909].hit[0], 3);
    pad(3, 1);
    enc_acc[EN_K1] = 1;
    now_ms += 20;
    input();
    render(2);
    CHECK(ui.hold_used && drum909_get(observed_909, r.b, r.c) != base);
    tap(0x5C);
    render(2);
    CHECK(seq.ppat[PART_909] != old_pattern);
    pad(3, 0);
    render(2);
    CHECK(drum909_get(observed_909, r.b, r.c) == base);
}

static void audio_isolation(void)
{
    char label[96];
    for (int kit = 0; kit < NKIT; kit++) for (int track = 0; track < NDRUM; track++) {
        snprintf(label, sizeof label, "%s track%d audible Solo destination", PART_N[kit], track + 1);
        context = label;
        setup(kit, track / 8);
        press(8 + track % 8);
        engine_drum(kit, track, 1.0f);
        CHECK(peak(2) > 0);
        audio_cases++;
        setup(kit, track / 8);
        press(8 + track % 8);
        engine_drum(kit, (track + 1) % NDRUM, 1.0f);
        CHECK(peak(2) == 0);         /* same part, wrong drum must be silent */
        audio_cases++;
        setup(kit, track / 8);
        press(8 + track % 8);
        engine_drum(kit ^ 1, 0, 1.0f);
        CHECK(peak(2) == 0);         /* the other drum machine is outside the solo union */
        audio_cases++;
    }
    context = "drum Solo suppresses 303 and release restores it";
    setup(0, 0);
    press(8);
    engine_bass_on(0, 48, 0, 0);
    CHECK(peak(2) == 0);
    release(8);
    CHECK(peak(2) > 0);
    engine_bass_off(0);
    audio_cases += 2;
    for (int kit = 0; kit < NKIT; kit++) {
        context = "main kit Solo plus drum Solo permits all of that kit";
        setup(-1, 0);
        press(8 + kit);
        view(kit, 0);
        press(10);                 /* another physical key holds drum 3 */
        engine_drum(kit, 4, 1.0f);
        CHECK(peak(2) > 0);
        audio_cases++;
    }
}

static float bus_peak(const float *bus)
{
    float maximum = 0.0f;
    for (int i = 0; i < 256; i++) {
        float a = bus[i] < 0 ? -bus[i] : bus[i];
        if (a > maximum) maximum = a;
    }
    return maximum;
}

static void drum_render_contract(void)
{
    static drum909_t d9, copy9;
    static drum808_t d8, copy8;
    float output[4][256], ordinary[4][256];
    uint32_t checksum[2] = {2166136261u, 2166136261u};
    for (int kit = 0; kit < NKIT; kit++) {
        context = kit ? "808 dry/send and ongoing-tail render contract" : "909 dry/send and ongoing-tail render contract";
        drum909_init(&d9); drum808_init(&d8);
        for (int track = 0; track < NDRUM; track++) {
            int n = kit ? drum808_nparams(track) : drum909_nparams(track);
            for (int p = 0; p < n; p++) {
                const x0x_param_t *d = kit ? drum808_param(track, p) : drum909_param(track, p);
                if (!strcmp(d->name, "Rev") || !strcmp(d->name, "Dly")) {
                    if (kit) drum808_set(&d8, track, p, 100);
                    else drum909_set(&d9, track, p, 100);
                }
            }
            if (kit) drum808_trigger(&d8, track, 1.0f);
            else drum909_trigger(&d9, track, 1.0f);
        }
        copy9 = d9; copy8 = d8;
        memset(ordinary, 0, sizeof ordinary);
        if (kit) drum808_render_st(&copy8, ordinary[0], ordinary[1], ordinary[2], ordinary[3], 256);
        else drum909_render_st(&copy9, ordinary[0], ordinary[1], ordinary[2], ordinary[3], 256);
        for (int bus = 0; bus < 4; bus++) {
            CHECK(bus_peak(ordinary[bus]) > 0);
            for (int i = 0; i < 256; i++) {
                uint32_t word;
                memcpy(&word, &ordinary[bus][i], sizeof word);
                checksum[kit] = (checksum[kit] ^ word) * 16777619u;
            }
        }
#ifdef SMC_MASKED_AUDIO
        memset(output, 0, sizeof output);
        if (kit) drum808_render_st_masked(&d8, output[0], output[1], output[2], output[3], 256, 0x7FFu);
        else drum909_render_st_masked(&d9, output[0], output[1], output[2], output[3], 256, 0x7FFu);
        CHECK(!memcmp(output, ordinary, sizeof output));
        /* Voices are already sounding. A zero mask gates both dry buses and
         * both voice sends, then releasing the mask restores the living tail. */
        memset(output, 0, sizeof output);
        if (kit) drum808_render_st_masked(&d8, output[0], output[1], output[2], output[3], 256, 0);
        else drum909_render_st_masked(&d9, output[0], output[1], output[2], output[3], 256, 0);
        for (int bus = 0; bus < 4; bus++) CHECK(bus_peak(output[bus]) == 0);
        memset(output, 0, sizeof output);
        if (kit) drum808_render_st_masked(&d8, output[0], output[1], output[2], output[3], 256, 0x7FFu);
        else drum909_render_st_masked(&d9, output[0], output[1], output[2], output[3], 256, 0x7FFu);
        for (int bus = 0; bus < 4; bus++) CHECK(bus_peak(output[bus]) > 0);
        audio_cases += 9;
#else
        (void)output;  /* the baseline has no masked renderer to call */
#endif
    }
    printf("Normal stereo audio fingerprints: 909=%08x 808=%08x\n", checksum[0], checksum[1]);
}

int main(void)
{
    puts("Simulated DAW/MMC, production SMC/UI/SMK pad mapping/engine; no physical-device result.");
    for (int mode = 0; mode < 2; mode++) {
        strip_view = mode ? V_SEQ : V_MIX;
        for (int kit = 0; kit < NKIT; kit++) for (int bank = 0; bank < 2; bank++)
            for (int strip = 0; strip < 8; strip++) destination(kit, bank, strip);
    }
    banks_and_owners();
    solo_transitions();
    cleanup();
    pad_release_context();
    held_edit_context();
    audio_isolation();
    drum_render_contract();
    printf("%s: %u active drum destinations, %u inactive positions, %u pad writes, %u audio cases; %u checks, %u failures\n",
           failures ? "FAIL" : "PASS", destinations, inactive, pad_writes, audio_cases, checks, failures);
    return failures ? 1 : 0;
}
