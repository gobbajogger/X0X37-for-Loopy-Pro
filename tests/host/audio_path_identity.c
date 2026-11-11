/* SPDX-License-Identifier: GPL-3.0-only */
/* Characterize production engine output, USB tap and meter/scope state.
 * Wrong downmix gain, skipped moving DJF smoothing or USB glide changes must fail. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static void put(FILE *f, const void *p, size_t n)
{
    if (fwrite(p, 1, n, f) != n) { perror("trace write"); exit(2); }
}
static FILE *open_trace(const char *dir, const char *name)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    return f;
}
static void fill_pattern(int dense)
{
    pattern_t *p = &proj.pat[0];
    for (int k = 0; k < NKIT; ++k)
        for (int v = 0; v < NDRUM; ++v)
            for (int s = 0; s < 16; ++s)
                if (dense || s % 4 == v % 4) sm_set(p->drum[k].hit[v], s);
    for (int b = 0; b < NBASS; ++b)
        for (int s = 0; s < 16; ++s) {
            p->bass[b].step[s].note = 36 + b * 12 + s % 7;
            p->bass[b].step[s].flags = G_NOTE | (s % 3 == 0 ? BS_ACCENT : 0) |
                                     (s % 4 == 0 ? BS_SLIDE : 0);
            p->bass[b].step[s].velocity = 60 + s * 4;
        }
    p->brk.steps = 0xffffu;
    p->brk.slot_a = p->brk.slot_b = 0;
}
static int mono(const char *dir)
{
    FILE *audio = open_trace(dir, "mono.f32");
    FILE *state = open_trace(dir, "mono-state.bin");
    for (int mode = 1; mode <= 3; ++mode)
        for (int moving = 0; moving <= 1; ++moving) {
            master_t m;
            master_init(&m);
            master_set(&m, MST_MODE, mode);
            for (int block = 0; block < 256; ++block) {
                float x[256];
                if (moving && block % 16 == 0)
                    master_set(&m, MST_CUTOFF, (block * 7) % 128);
                if (block == 64) master_set(&m, MST_COMP1, 72);
                if (block == 128) master_set(&m, MST_LIMIT, 0);
                for (int i = 0; i < 256; ++i)
                    x[i] = (float)(((block * 256 + i) * 37) % 1024 - 512) / 256.0f;
                master_process(&m, x, 256, block < 96 ? 0.8f : 0.3f);
                put(audio, x, sizeof x);
                put(state, &m, sizeof m);
            }
        }
    return fclose(audio) || fclose(state);
}
#ifndef AUDIO_PATH_IDENTITY_LIBRARY
int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    const char *scenario = argv[1], *dir = argv[2];
    if (!strcmp(scenario, "mono")) return mono(dir);
    boot();
    engine_guard_enable(0);
    assert(load_slot(0, 0, argv[3]) == 0);
    engine_brk_loops();
    int idle = !strcmp(scenario, "idle");
    fill_pattern(!strcmp(scenario, "dense"));
    engine_brk_loops();
    if (!idle) seq_start(&seq);
    if (!strcmp(scenario, "comp")) engine_set(T_MST, 0, MST_COMP1, 80);
    if (!strcmp(scenario, "limiter-off")) engine_set(T_MST, 0, MST_LIMIT, 0);
    if (!strncmp(scenario, "djf", 3)) {
        engine_set(T_MST, 0, MST_MODE, scenario[3] - '0');
        engine_set(T_MST, 0, MST_CUTOFF, 72);
    }
    engine_usb_full(!strcmp(scenario, "usb-full"));
    master = 3072;
    FILE *audio = open_trace(dir, "audio.i32");
    FILE *tap = open_trace(dir, "usb.i32");
    FILE *trace = open_trace(dir, "peak-scope.bin");
    uint16_t seen[NPARTS] = {0};
    for (int block = 0; block < 512; ++block) {
        int32_t lr[512];
        now_ms = (uint32_t)((uint64_t)block * 256u * 1000u / 44100u);
        if (!strcmp(scenario, "solo-mute")) {
            if (block == 64) engine_smc_solo(1u << PART_303A);
            if (block == 128) seq.mute = seq_part_mask(PART_303B);
            if (block == 192) engine_smc_solo(0);
            if (block == 256) seq.mute = 0;
        }
        if (!strcmp(scenario, "mix-sweeps") && block % 8 == 0)
            for (int p = 0; p < NPARTS; ++p) {
                engine_set(T_MIX, p, MX_LEVEL, (block + p * 19) % 128);
                engine_set(T_MIX, p, MX_PAN, (block * 3 + p * 23) % 128);
                engine_set(T_MIX, p, MX_REV, (block + p * 7) % 128);
                engine_set(T_MIX, p, MX_DLY, (block * 5 + p * 11) % 128);
            }
        if (!strncmp(scenario, "djf", 3) && strstr(scenario, "sweep") && block % 16 == 0)
            engine_set(T_MST, 0, MST_CUTOFF, (block * 7) % 128);
        if (!strcmp(scenario, "preset-fade")) {
            if (block == 64) {
                engine_bass_off(0);
                engine_bass_off(1);
                assert(engine_bass_select_preset(0, FACTORY_BANK_MIAMI, 1, &proj.sound) >= 0);
                assert(engine_bass_select_preset(1, FACTORY_BANK_CHICAGO, 1, &proj.sound) >= 0);
            }
        }
        if (block == 128) master = 1024;
        if (block == 320) master = 4096;
        for (int p = 0; p < NPARTS; ++p) eng_peak[p] = (uint16_t)(eng_peak[p] * 15u / 16u);
        engine_render(lr, 256);
        put(tap, lr, sizeof lr);
        engine_volume(lr, 256);
        put(audio, lr, sizeof lr);
        for (int p = 0; p < NPARTS; ++p) {
            uint16_t peak = eng_peak[p];
            seen[p] |= peak;
            put(trace, &peak, sizeof peak);
        }
        put(trace, eng_scope, sizeof eng_scope);
        uint32_t w = eng_scope_w;
        put(trace, &w, sizeof w);
        assert(engine_bass_status(0) > 0 && engine_bass_status(1) > 0);
    }
    if (!strcmp(scenario, "all"))
        for (int p = 0; p < NPARTS; ++p) assert(seen[p] != 0);
    if (fclose(audio) || fclose(tap) || fclose(trace)) return 2;
    return 0;
}

#endif
