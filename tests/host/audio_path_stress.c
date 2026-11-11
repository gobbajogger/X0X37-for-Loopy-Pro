/* SPDX-License-Identifier: GPL-3.0-only */
/* Reproducible CPU stress using production engine, SMC and UI.
 * Host MIDI queues substitute transport delivery; no hardware ISR/IRQ claim. */
#define AUDIO_PATH_IDENTITY_LIBRARY
#include "audio_path_identity.c"

uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *msg, uint8_t len)
{ (void)slot; return plat_ble_smc_send(msg, len); }
#include "../../firmware/src/app/smc_mixer.c"

static void packet(unsigned src, unsigned status, unsigned a, unsigned b)
{
    assert(mi_w - mi_r < MQ);
    min_q[mi_w++ % MQ] = (status >> 4) | src << 4 | status << 8 | a << 16 | b << 24;
}
int main(int argc, char **argv)
{
    if (argc != 7) return 2;
    const char *dir = argv[1];
    int chicago = atoi(argv[3]), miami = atoi(argv[4]), blocks = atoi(argv[5]);
    int sweep = atoi(argv[6]);
    if (blocks < 1) return 2;
    boot();
    engine_guard_enable(0); /* no load shedding may make a stress result look cheaper */
    assert(load_slot(0, 0, argv[2]) == 0);
    fill_pattern(1);
    seq.bpm = 300.0f;
    seq.ch[PART_303A] = 2;
    seq.ch[PART_303B] = 3;
    /* A=Chicago, B=Miami, at the profile's fixed maximum voice/unison counts. */
    assert(engine_bass_select_preset(0, FACTORY_BANK_CHICAGO, chicago, &proj.sound) >= 0);
    assert(engine_bass_select_preset(1, FACTORY_BANK_MIAMI, miami, &proj.sound) >= 0);
    int32_t lr[1024];
    for (int warm = 0; warm < 4; ++warm) engine_render(lr, 256);
    assert(engine_bass_bank(0) == FACTORY_BANK_CHICAGO);
    assert(engine_bass_bank(1) == FACTORY_BANK_MIAMI);
    engine_brk_loops();
    for (int i = 0; i < engine_nparams(T_FX, 0); ++i) {
        const x0x_param_t *p = engine_param(T_FX, 0, i);
        if (p) engine_set(T_FX, 0, i, p->max > 1 ? p->max * 3 / 4 : p->max);
    }
    engine_set(T_MST, 0, MST_COMP1, 80);
    engine_set(T_MST, 0, MST_MODE, 1);
    engine_set(T_MST, 0, MST_CUTOFF, 72);
    engine_set(T_MST, 0, MST_RESO, 100);
    engine_usb_full(1);
    master = 3072;
    proj.set.bt |= PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    smc_ble_link(1);
    smc_poll();
    seq_start(&seq);
    FILE *audio = open_trace(dir, "audio.i32");
    FILE *trace = open_trace(dir, "peak-scope.bin");
    double total = 0, peak = 0, foreground_peak = 0;
    double synth_sum[2] = {0}, synth_peak[2] = {0};
    unsigned late = 0, above_60 = 0, queued_peak = 0;
    uint32_t blits0 = blits;
    for (int block = 0; block < blocks; ++block) {
        now_ms = (uint32_t)((uint64_t)block * 512u * 1000u / 44100u);
        double fg0 = now_ns();
        for (int i = 0; i < 8; ++i) {
            uint8_t fader[3] = {0xB0, (uint8_t)(40 + i), (uint8_t)(80 + (block + i) % 48)};
            uint8_t knob[3] = {0xB0, (uint8_t)(16 + i), (uint8_t)(64 + (block * 7 + i) % 64)};
            smc_ble_midi(fader, 3);
            smc_ble_midi(knob, 3);
        }
        smc_poll();
        ui.view = block & 1 ? V_HOME : V_MIX;
        ui_frame();
        smc_frame();
        double foreground = now_ns() - fg0;
        if (foreground > foreground_peak) foreground_peak = foreground;
        if (sweep && block % 8 == 0) {
            engine_set(T_MST, 0, MST_MODE, 1 + (block / 8) % 3);
            engine_set(T_MST, 0, MST_CUTOFF, (block * 13) % 128);
        }
        master = block & 64 ? 4096 : 1536;
        uint32_t prof0[2] = {eng_prof[PR_303A], eng_prof[PR_303B]};
        double elapsed = 0;
        /* Match main_fm1.c: each 512-frame DMA half is two engine calls of 256. */
        for (int half = 0; half < 2; ++half) {
            for (unsigned src = 0; src < 2; ++src)
                for (unsigned b = 0; b < 2; ++b)
                    for (unsigned note = 0; note < 8; ++note) {
                        unsigned old = 36 + (block + 11) % 12 + note;
                        unsigned key = 36 + block % 12 + note;
                        packet(src, 0x80u | (2u + b), old, 0);
                        packet(src, 0x90u | (2u + b), key, 70 + (block + note) % 58);
                    }
            unsigned queued = mi_w - mi_r;
            if (queued > queued_peak) queued_peak = queued;
            double t0 = now_ns();
            engine_render(lr + half * 512, 256);
            engine_volume(lr + half * 512, 256);
            elapsed += now_ns() - t0;
        }
        total += elapsed;
        if (elapsed > peak) peak = elapsed;
        if (elapsed > 11600000.0) late++;
        if (elapsed > 6960000.0) above_60++;
        for (int b = 0; b < 2; ++b) {
            double dt = (uint32_t)(eng_prof[PR_303A + b] - prof0[b]);
            synth_sum[b] += dt;
            if (dt > synth_peak[b]) synth_peak[b] = dt;
        }
        put(audio, lr, sizeof lr);
        for (int p = 0; p < NPARTS; ++p) {
            uint16_t value = eng_peak[p];
            put(trace, &value, sizeof value);
            eng_peak[p] = (uint16_t)(value * 15u / 16u);
        }
        put(trace, eng_scope, sizeof eng_scope);
        uint32_t w = eng_scope_w;
        put(trace, &w, sizeof w);
        assert(engine_bass_status(0) > 0 && engine_bass_status(1) > 0);
        assert(!plat_midi_pending());
    }
    assert(blits > blits0);
    if (fclose(audio) || fclose(trace)) return 2;
    printf("{\"chicago_preset\":%d,\"miami_preset\":%d,\"blocks\":%d,"
           "\"budget_ns\":11600000,\"peak_ns\":%.0f,\"average_ns\":%.0f,"
           "\"margin_ns\":%.0f,\"peak_pct\":%.6f,\"average_pct\":%.6f,"
           "\"deadline_misses\":%u,\"blocks_above_60pct\":%u,"
           "\"chicago_peak_ns\":%.0f,\"chicago_average_ns\":%.0f,"
           "\"miami_peak_ns\":%.0f,\"miami_average_ns\":%.0f,"
           "\"foreground_peak_ns\":%.0f,\"midi_packets\":%u,\"midi_queue_peak\":%u,"
           "\"smc_messages\":%u,\"blits\":%u}\n",
           chicago, miami, blocks, peak, total / blocks, 11600000.0 - peak,
           peak / 116000.0, total / blocks / 116000.0, late, above_60,
           synth_peak[0], synth_sum[0] / blocks, synth_peak[1], synth_sum[1] / blocks,
           foreground_peak, (unsigned)blocks * 128u, queued_peak,
           (unsigned)blocks * 16u, blits - blits0);
    return 0;
}
