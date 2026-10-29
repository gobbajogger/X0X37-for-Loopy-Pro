/* SPDX-License-Identifier: GPL-3.0-only */
/* Regress production SMC mapping against the keyboard's per-track mute contract.
 * Input consists of raw DAW MIDI messages. The radio/HCI path is bypassed.
 * The production project, engine, UI, queues, button map and LED map are used.
 * Hardware I/O and unused clip handlers are substitutes. No audio claim is made.
 */
#define main simulator_main
void smc_ui_cancel(void);
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static uint8_t clip_mode;
uint8_t smk_clip_mode_on(void) { return clip_mode; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{ (void)msg; (void)len; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#ifndef SMC_MIXER_SOURCE
#define SMC_MIXER_SOURCE "app/smc_mixer.c"
#endif
#include SMC_MIXER_SOURCE

static void message(int status, int a, int b)
{
    const uint8_t raw[3] = {status, a, b};
    now_ms += 100;
    smc_ble_midi(raw, sizeof raw);
    smc_poll();
    smc_frame();
    assert((smc_rx_diag() >> 24) == 0);
    if (status==0x90 && b && a>=16 && a<24) {
        const uint8_t up[3]={0x80,(uint8_t)a,0};
        smc_ble_midi(up,3);smc_poll(); /* Separate mute clicks have release edges. */
    }
}

static void setup(int mode, int bank)
{
    memset(&ui, 0, sizeof ui);
    memset(&smc, 0, sizeof smc);
    clip_mode = 0;
    boot();
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link(1);
    smc_poll();
    if (!mode) smc_view(V_HOME, 0);
    else {
        smc_view(V_MIX, mode);
        ui.sel[mode - 1] = bank * 8;
        build_pages();
        assert(drum_mix_page() == mode - 1);
    }
    seq.mute = seq.vmute = 0;
    smc.solo = 0;
    engine_smc_solo(0);
    smc_frame();
}

static void continuous_probe(int mode, int bank, int strip)
{
    setup(mode, bank);
    pref_t r = smc_target(strip);
    pref_t k = smc_knob_target(strip);
    assert(r.kind == R_NONE ? k.kind == R_NONE : !smc_same(r, k));
    if (r.kind == R_NONE) return;
    if (mode) {
        pref_t expected = find_ref(mode == 1 ? T_909 : T_808, bank * 8 + strip, "Level");
        assert(expected.kind == R_ENG && expected.a == (mode == 1 ? T_909 : T_808));
        assert(expected.b == bank * 8 + strip);
        assert(smc_same(r, expected));
    }
    const x0x_param_t *d = pref_desc(r);
    const x0x_param_t *dk = pref_desc(k);
    pref_set(r, d->max / 2);
    /* Home CUT (and any other open-at-max default) must sit below the ceiling so a
     * +1 relative detent can prove the knob moved without touching the fader. */
    if (dk && dk->max)
        pref_set(k, dk->max / 2);
    smc_frame();
    int level = pref_get(r), before = pref_get(k);
    /* DAW K1 = b0 10 01. */
    message(0xB0, 16 + strip, 1);
    int after_knob = pref_get(k);
    assert(after_knob > before && pref_get(r) == level);
    /* First acquire the current target, then move the pitch-bend fader to 0. */
    message(0xE0 + strip, 0, smc_scaled(r));
    assert(smc.picked & (1u << strip));
    message(0xE0 + strip, 0, 0);
    assert(pref_get(r) == 0);
    printf("CONTINUOUS mode=%s bank=%d strip=%d target=%u/%u/%u/%u knob=%d->%d fader=%d->0 PASS\n",
           mode == 0 ? "MAIN" : mode == 1 ? "909" : "808", bank + 1, strip + 1,
           r.kind, r.a, r.b, r.c, before, after_knob, after_knob);
}


static void mute_regression(int mode, int bank, int strip)
{
    setup(mode, bank);
    int kit = mode - 1, track = bank * 8 + strip;
    int before_fx[3];
    for (int i = 0; i < 3; i++) {
        pref_t r = PR(R_ENG, T_FX, 0, i == 0 ? FX_RV_LEVEL : i == 1 ? FX_DL_LEVEL : FX_VOLUME);
        pref_set(r, 60 + i);
        before_fx[i] = pref_get(r);
    }
    /* A drum MIX page can show a kit that is not the selected part. */
    ui.part = 3;
    ui.sel[kit] = bank * 8 + 2;
    smc_frame();
    uint32_t expected = track < NDRUM ? 1u << (kit * NDRUM + track) : 0;
    message(0x90, 16 + strip, 127);
    assert(seq.vmute == expected);
    assert(seq.mute == 0);
    assert(smc.lit[16 + strip] == (track < NDRUM));
    for (int i = 0; i < 3; i++)
        assert(pref_get(PR(R_ENG, T_FX, 0, i == 0 ? FX_RV_LEVEL : i == 1 ? FX_DL_LEVEL : FX_VOLUME)) == before_fx[i]);
    assert(ui.sel[kit] == bank * 8 + 2 && ui.part == 3);
    message(0x90, 16 + strip, 127);
    assert(seq.vmute == 0 && seq.mute == 0 && smc.lit[16 + strip] == 0);
    if (track < NDRUM) {
        mute_track(kit, track);
        smc_frame();
        assert(seq.vmute == expected && smc.lit[16 + strip] == 1);
        mute_track(kit, track);
        smc_frame();
        assert(smc.lit[16 + strip] == 0);
        message(0x90, 24 + strip, 127);
        assert(ui.sel[kit] == track && ui.view == V_MIX);
        assert(smc.lit[24 + strip] == 1);
    } else {
        assert(smc_target(strip).kind == R_NONE);
        assert(smc_knob_target(strip).kind == R_NONE);
        message(0x90, 24 + strip, 127);
        assert(ui.sel[kit] == bank * 8 + 2 && smc.lit[24 + strip] == 0);
    }
    printf("PASS: %s bank %d M%d action, light and SELECT\n", mode == 1 ? "909" : "808", bank + 1, strip + 1);
}

static void main_regression(void)
{
    for (int strip = 0; strip < 8; strip++) {
        setup(0, 0);
        pref_t fx = strip < NPARTS ? NONE : PR(R_ENG, T_FX, 0,
            strip == 5 ? FX_RV_LEVEL : strip == 6 ? FX_DL_LEVEL : FX_VOLUME);
        int before = strip < NPARTS ? 0 : pref_get(fx);
        assert(strip < NPARTS || before > 0);
        message(0x90, 16 + strip, 127);
        assert(seq.vmute == 0 && smc.lit[16 + strip] == 1);
        if (strip < NPARTS) {
            static const int expected_part[5] = {PART_909, PART_808, PART_BRK, PART_303B, PART_303A};
            assert(seq.mute == MUTE_MASK[expected_part[strip]]);
        }
        else assert(seq.mute == 0 && pref_get(fx) == 0);
        message(0x90, 16 + strip, 127);
        assert(seq.vmute == 0 && seq.mute == 0 && smc.lit[16 + strip] == 0);
        if (strip >= NPARTS) assert(pref_get(fx) == before);
    }
}

static void priority_regression(void)
{
    /* Keep the existing main-part mute behavior while menu or clip owns input. */
    for (int owner = 0; owner < 4; owner++) {
        setup(1, 0);
        if (owner == 0) ui.help = 1;
        if (owner == 1) ui.overlay = O_ASK;
        if (owner == 2) ui.overlay = O_LIST;
        if (owner == 3) clip_mode = 1;
        message(0x90, 17, 127);
        /* Help leaves drum MIX mutes working; confirmation parks the surface;
         * menu/clip fall through to whole-part mute instead. */
        assert(seq.vmute == (owner == 0 ? (1u << 1) : 0));
        assert(seq.mute == (owner >= 2 ? MUTE_MASK[1] : 0));
    }
    for (int kit = 0; kit < 2; kit++) {
        setup(kit + 1, 0);
        seq.vmute = 0xFFFFFFFFu;
        seq.mute = 0xFFFFFFFFu;
        smc_frame();
        for (int strip = 0; strip < 8; strip++) assert(smc.lit[16 + strip] == 1);
        message(0x90, 0x2F, 127);
        assert(ui.sel[kit] == 8);
        for (int strip = 0; strip < 8; strip++) {
            assert(smc.lit[16 + strip] == (strip < 3));
            assert(smc.lit[24 + strip] == (strip == 0));
        }
        message(0x90, 0x2E, 127);
        for (int strip = 0; strip < 8; strip++) assert(smc.lit[16 + strip] == 1);
        /* The owner-selected drum R/S layout follows the same visible bank. */
        setup(kit + 1, 1);
        message(0x90, 1, 127);
        assert(ui.part == kit && ui.sel[kit] == 9 && ui.rec);
        message(0x90, 9, 127);
        assert(smc_project_active() && smc.lit[9] == 1);
        message(0x80, 9, 0);
        assert(!smc_project_active() && smc.lit[9] == 0);
    }
}

int main(void)
{
    puts("Simulated raw DAW MIDI; production UI, engine, SMC queues and feedback; no radio/HCI or device claim.");
    main_regression();
    for (int strip = 0; strip < 8; strip++) continuous_probe(0, 0, strip);
    for (int mode = 1; mode <= 2; mode++)
        for (int bank = 0; bank < 2; bank++)
            for (int strip = 0; strip < 8; strip++) {
                mute_regression(mode, bank, strip);
                continuous_probe(mode, bank, strip);
            }
    priority_regression();
    puts("PASS: 22 active drum mute/light destinations, 10 inactive positions, 30 knob/fader writes, main mute/FX and ownership priorities.");
    return 0;
}
