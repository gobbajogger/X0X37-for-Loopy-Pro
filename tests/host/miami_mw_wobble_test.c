/* SPDX-License-Identifier: GPL-3.0-only */
/* Miami/WBL1: mod wheel writes W_WOBBLE_DEPTH (not Chicago cutoff). */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define X0X_WOBBLE 1
#define X0X_DEV_WOBBLE 1
#define main simulator_main
#include X0X_HOST_SOURCE
#undef main
#include <assert.h>
#include <stdio.h>
#include "dsp/wobble_config.h"
#include "dsp/wobble_backend.h"
#include "../../../experiments/wobble/wobble.h"

static unsigned commands;
static int last_target, last_voice, last_param, last_value;
void real_engine_set(int, int, int, int);
void wheel_test_engine_set(int t, int v, int i, int val)
{
    commands++;
    last_target = t;
    last_voice = v;
    last_param = i;
    last_value = val;
    real_engine_set(t, v, i, val);
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
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include SMC_MIXER_SOURCE

static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
#include "smk_wheels.inc"

int main(void)
{
    int32_t out[512];
    boot();
    engine_guard_enable(0);
    engine_render(out, 256);
    assert(engine_bass_is_wobble(0) && "product Miami: 303A is WBL1");
    assert(!engine_bass_is_wobble(1) && "303B stays native303");
    assert(engine_nparams(T_303, 0) == W_PARAMETER_COUNT);
    assert(engine_nparams(T_303, 1) == CHI_AUX_END);
    assert(engine_param(T_303, 0, W_BASS_SPLIT)->def == 36);
    assert(engine_param(T_303, 0, W_PARAMETER_COUNT) == NULL);
    {
        sound_t fresh;
        engine_sound_defaults(&fresh);
        for (int i = 0; i < W_PARAMETER_COUNT; ++i) {
            const x0x_param_t *p = engine_param(T_303, 0, i);
            assert(p && p->name);
            assert(fresh.v[T_303][0][i] <= p->max);
            assert(fresh.v[T_303][0][i] == p->def);
        }
        assert(fresh.v[T_303][0][W_CARRIER_WAVE] == 0);
        assert(fresh.v[T_303][0][W_BASS_SPLIT] == 36);
        assert(fresh.v[T_303][1][BASS303_CUTOFF] == 48);
    }
    {
        wobble_t w;
        wobble_init(&w, 44100.0f);
        const unsigned toggles[] = {W_CARRIER_WAVE, W_XMOD_WAVE,
                                    W_WOBBLE_SHAPE, W_DAMPER};
        for (unsigned i = 0; i < sizeof toggles / sizeof toggles[0]; ++i) {
            wobble_foundation_set(&w, (int)toggles[i], 1);
            assert(w.p[toggles[i]] >= 0.5f);
            wobble_foundation_set(&w, (int)toggles[i], 0);
            assert(w.p[toggles[i]] == 0.0f);
        }
        wobble_foundation_set(&w, W_BASS_SPLIT, 36);
        assert(wobble_bass_split_hz(w.p[W_BASS_SPLIT]) > 80.0f);
        assert(wobble_bass_split_hz(w.p[W_BASS_SPLIT]) < 100.0f);
    }

    ui.part = PART_303A;
    fm1_in.frames = 400;
    fm1_in.wheel[0] = 550; /* pitch rest */
    fm1_in.wheel[1] = 615; /* mod rest (high ADC) */
    commands = 0;
    smk_wheels();
    fm1_in.wheel[1] = 550; /* move mod up from rest → nonzero mw */
    smk_wheels();
    assert(commands >= 1);
    assert(last_target == T_303 && last_voice == 0);
    assert(last_param == W_WOBBLE_DEPTH);
    assert(last_value > 0 && last_value <= 127);
    /* Miami bend must reach WBL1, not CHI_PBEND in dormant b303[0]. */
    fm1_in.wheel[0] = 580;
    smk_wheels();
    assert(last_target == T_303 && last_voice == 0);
    assert(last_param == W_PITCH_BEND && last_value > 64);
    ui.part = PART_909;               /* offscreen release stays on original 303 */
    fm1_in.wheel[0] = 550;
    smk_wheels();
    assert(last_target == T_303 && last_voice == 0);
    assert(last_param == W_PITCH_BEND);
    assert(last_value == proj.sound.v[T_303][0][W_PITCH_BEND]);
    ui.part = PART_303B;
    fm1_in.wheel[0] = 580;
    smk_wheels();
    assert(last_voice == 1 && last_param == CHI_PBEND && last_value > 64);
    fm1_in.wheel[0] = 550;
    smk_wheels();
    /* Miami's own damper holds a live note after release. Pattern off remains
     * unconditional; damper-up releases the deferred live gate. */
    engine_set(T_303, 0, W_DAMPER, 1);
    engine_render(out, 256);
    engine_bass_on(0, 48, 0, 0);
    engine_render(out, 256);
    assert(engine_midi_held() & 1);
    engine_bass_off(0);
    engine_render(out, 256);
    assert(engine_midi_held() & 1);
    engine_set(T_303, 0, W_DAMPER, 0);
    engine_render(out, 256);
    assert(!(engine_midi_held() & 1));
    puts("PASS Miami defaults, Bass Split, binary controls, pitch wheels and damper");
    return 0;
}
