/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual SMK wheel scanner + UI/SMC + original engines: compare navigation to
 * an otherwise identical control run. No legacy bass303 DSP structs or mocks. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include X0X_HOST_SOURCE
#undef main
#include <assert.h>
#include <stdint.h>
#include <string.h>

#if X0X_ORIGINAL_SYNTH != 1
#error The wheel test requires the shipping Chicago/Miami engines
#endif

static unsigned commands;
static int last_kind, last_target, last_voice, last_param, last_value;
void wheel_real_engine_set(int, int, int, int);
void wheel_real_engine_set_base(int, int, int);
void wheel_test_engine_set(int t, int v, int i, int value)
{
    ++commands; last_kind = 1; last_target = t; last_voice = v;
    last_param = i; last_value = value;
    wheel_real_engine_set(t, v, i, value);
}
/* 8401fc23 (BAS-11): the mod wheel is the engine's mod controller, an absolute
 * CUTOFF / WOBBLE DEPTH value (rest = 0), not a sound-parameter write. */
void wheel_real_engine_bass_mod(int, int);
int wheel_real_engine_bass_mod_param(int);
void wheel_test_engine_bass_mod(int v, int value)
{
    ++commands; last_kind = 3; last_target = T_303; last_voice = v;
    last_param = wheel_real_engine_bass_mod_param(v); last_value = value;
    wheel_real_engine_bass_mod(v, value);
}
void wheel_test_engine_set_base(int t, int v, int i)
{
    ++commands; last_kind = 2; last_target = t; last_voice = v;
    last_param = i; last_value = -1;
    wheel_real_engine_set_base(t, v, i);
}

static uint8_t clip_grid_mode;
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
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include SMC_MIXER_SOURCE

static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
#include "smk_wheels.inc"

static int mod_param(int voice)
{
    return engine_bass_bank(voice) == ORIGINAL_SOUND_MIAMI ?
        ORIGINAL_MIAMI_WOBBLE_DEPTH : ORIGINAL_CHICAGO_CUTOFF;
}
static int bend_param(int voice)
{
    return engine_bass_bank(voice) == ORIGINAL_SOUND_MIAMI ?
        ORIGINAL_MIAMI_PITCH_BEND : ORIGINAL_CHICAGO_PITCH_BEND;
}
static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}
static uint32_t edited_bits(int voice, int param, int control)
{
    float value;
    assert(original_sound_control_value((unsigned)engine_bass_bank(voice),
        (unsigned)param, (unsigned)control, &value));
    return float_bits(value);
}
static void set_base(int voice, int control)
{
    const int param = mod_param(voice);
    assert(original_sound_set(proj.sound.v[T_303][voice], (unsigned)param, (unsigned)control));
    engine_bass_sound_refresh(&proj.sound, voice);
    engine_set(T_303, voice, param, control);
}
static int32_t peak_sample;
static uint64_t hash = UINT64_C(1469598103934665603);
static void render(void)
{
    int32_t out[512];
    engine_render(out, 256);
    for (unsigned i = 0; i < 512; ++i) {
        if (out[i] > peak_sample) peak_sample = out[i];
        hash ^= (uint32_t)out[i];
        hash *= UINT64_C(1099511628211);
    }
}
static void select_part(int part, int smc_path)
{
    if (smc_path) {
        /* SELECT notes 24..28 are FX holds, not navigation. The SMC's real
         * part selector is still used by REC and the bank/channel route. */
        smc_select_part(part);
        smc_poll(); smc_frame();
        assert(ui.part == part);
    } else {
        bass_step_cancel(); drum_step_cancel(); ui.part = (uint8_t)part;
        build_pages();
    }
}
int main(int argc, char **argv)
{
    assert(argc == 7);
    const int nav = atoi(argv[1]), playing = atoi(argv[2]), locks = atoi(argv[3]);
    const int smc_path = atoi(argv[4]), part = atoi(argv[5]), gesture = atoi(argv[6]);
    boot(); engine_guard_enable(0); render();
    assert(engine_bass_bank(0) == ORIGINAL_SOUND_MIAMI);
    assert(engine_bass_bank(1) == ORIGINAL_SOUND_CHICAGO);
    assert(engine_bass_sound_validate(&proj.sound) == 0);
    for (int voice = 0; voice < NBASS; ++voice) set_base(voice, 30 + 10 * voice);
    render();
    assert(engine_bass_sound_validate(&proj.sound) == 0);
    if (smc_path) {
        proj.set.bt = PROJ_BT_SMC_CENTRAL;
        smc_ble_link(1); smc_poll(); smc_frame();
    }
    select_part(PART_303A + part, smc_path); set_view(V_PART);
    fm1_in.frames = 400; fm1_in.wheel[0] = 550; fm1_in.wheel[1] = 615;
    smk_wheels(); render();
    const uint32_t base_bits = edited_bits(part, mod_param(part), 30 + 10 * part);
    assert(engine_original_value_bits(part, (unsigned)mod_param(part)) == base_bits);

    fm1_in.wheel[1] = (gesture & 1) ? 550 : 615;
    fm1_in.wheel[0] = (gesture & 2) ? 580 : 550;
    smk_wheels(); render();
    if (gesture & 1) {
        assert(last_kind == 3 && last_target == T_303 && last_voice == part);
        assert(last_param == mod_param(part) && last_value == 57);
        assert(engine_original_value_bits(part, (unsigned)mod_param(part)) ==
            edited_bits(part, mod_param(part), 57));
    }
    if (locks) {
        const int lane = motion_alloc(proj.arr.lane, 0, PART_303A + part,
            T_303, part, mod_param(part));
        assert(lane >= 0);
        for (int i = 0; i < NSTEPS; ++i)
            proj.arr.lane[lane].val[i] = (uint8_t)((locks == 2 && (i & 1)) ? 95 : 77);
        engine_motion_reset();
    }
    seq.mute = seq.vmute = 0;
    bpart_t *bp = &proj.pat[0].bass[part]; bp->len = 4;
    for (int i = 0; i < 4; ++i) bp->step[i] = (bstep_t){48, G_NOTE, 100};
    if (playing) { seq_start(&seq); render(); }
    else { engine_bass_on(part, 48, 0, 0); render(); }
    const unsigned before = commands;
    const uint32_t initial_bits = engine_original_value_bits(part, (unsigned)mod_param(part));
    assert(initial_bits != UINT32_MAX);
    for (int i = 0; i < 160; ++i) {
        /* Navigation must not retarget a held wheel or enqueue a parameter. */
        if (nav && i == 5) select_part(PART_909, smc_path);
        if (nav && i == 15) select_part(PART_303A + (1 - part), smc_path);
        if (nav && i == 25) select_part(PART_303A + part, smc_path);
        if (nav && i == 35) set_view(V_SEQ);
        if (nav && i == 45) set_view(V_PART);
        smk_wheels();
        assert(commands == before);
        render();
        const uint32_t live = engine_original_value_bits(part, (unsigned)mod_param(part));
        assert(live != UINT32_MAX);
        if (!locks || !playing) assert(live == initial_bits);
        hash ^= live; hash *= UINT64_C(1099511628211);
        assert(pref_get(PR(R_ENG, T_303, part, mod_param(part))) == 30 + 10 * part);
        assert(proj.sound.v[T_303][part][mod_param(part)] == 30 + 10 * part);
    }
    assert(peak_sample > 0);
    printf("audio=%016llx live=%08x base=%d commands=%u\n",
        (unsigned long long)hash, initial_bits,
        proj.sound.v[T_303][part][mod_param(part)], commands - before);

    /* Offscreen release belongs to the original gesture owner. */
    select_part(PART_909, smc_path);
    fm1_in.wheel[1] = 615; fm1_in.wheel[0] = 550; smk_wheels();
    assert(commands == before + !!(gesture & 1) + !!(gesture & 2));
    render();
    if (!playing && (gesture & 1))      /* the wheel at rest is mod 0 (BAS-11) */
        assert(engine_original_value_bits(part, (unsigned)mod_param(part)) ==
            edited_bits(part, mod_param(part), 0));

    /* A physical knob edit is authoritative while a stationary wheel is held. */
    seq_stop(&seq); render();
    select_part(PART_303A + part, smc_path);
    fm1_in.wheel[1] = 550; smk_wheels(); render();
    set_base(part, 51); render();
    unsigned after_edit = commands;
    select_part(PART_909, smc_path); smk_wheels(); render();
    select_part(PART_303A + part, smc_path); smk_wheels(); render();
    assert(commands == after_edit);
    assert(engine_original_value_bits(part, (unsigned)mod_param(part)) ==
        edited_bits(part, mod_param(part), 51));
    fm1_in.wheel[1] = 615; smk_wheels(); render();

    /* Simultaneous gestures retain independent voices across part changes. */
    fm1_in.wheel[1] = 550; smk_wheels(); render();
    select_part(PART_303A + (1 - part), smc_path);
    fm1_in.wheel[0] = 580; smk_wheels(); render();
    assert(last_kind == 1 && last_target == T_303);
    assert(last_param == bend_param(1 - part) && last_voice == 1 - part);
    assert(last_value != 64);
    unsigned edge = commands;
    fm1_in.wheel[1] = 615; smk_wheels();
    assert(commands == edge + 1 && last_kind == 3 && last_value == 0);
    assert(last_voice == part && last_param == mod_param(part));
    fm1_in.wheel[0] = 550; smk_wheels();
    assert(commands == edge + 2 && last_kind == 2);
    assert(last_voice == 1 - part && last_param == bend_param(1 - part));
    render();
    assert(engine_original_value_bits(1 - part, (unsigned)bend_param(1 - part)) ==
        float_bits(0.5f));
    return 0;
}
