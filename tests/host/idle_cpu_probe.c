/* SPDX-License-Identifier: GPL-3.0-only */
/* Production host engine: ten seconds of audio-time silence and restart traces.
 * Host nanoseconds are not target ISR cycles or a target CPU certificate. */
#define AUDIO_PATH_IDENTITY_LIBRARY
#include "audio_path_identity.c"

static void setup_idle(void)
{
    boot();
    engine_guard_enable(0);
    engine_usb_full(0);
    seq_stop(&seq);
    assert(engine_bass_select_preset(0, FACTORY_BANK_CHICAGO, 0, &proj.sound) >= 0);
    assert(engine_bass_select_preset(1, FACTORY_BANK_MIAMI, 0, &proj.sound) >= 0);
    /* Adopt the programs before the measured or characterized idle interval. */
    int32_t out[512];
    /* Preset adoption takes more than one block; render until both are adopted. */
    for (int i = 0; i < 400 && (engine_bass_bank(0) != FACTORY_BANK_CHICAGO ||
                                engine_bass_bank(1) != FACTORY_BANK_MIAMI); ++i)
        engine_render(out, 256);
    assert(engine_bass_bank(0) == FACTORY_BANK_CHICAGO);
    assert(engine_bass_bank(1) == FACTORY_BANK_MIAMI);
}

static void silent_frames(uint32_t n)
{
    int32_t out[512];
    while (n) {
        uint32_t k = n > 256 ? 256 : n;
        engine_render(out, k);
        for (uint32_t i = 0; i < 2 * k; ++i) assert(out[i] == 0);
        n -= k;
    }
}

static int benchmark(void)
{
    const uint32_t frames = 10u * 44100u;
    uint64_t stage_total[ENG_PROF_N] = {0}, stage_peak[ENG_PROF_N] = {0};
    uint64_t elapsed = 0, peak = 0;
    setup_idle();
    silent_frames(512);
    for (uint32_t done = 0; done < frames;) {
        uint32_t count = frames - done;
        if (count > 512) count = 512;
        uint32_t before[ENG_PROF_N];
        for (unsigned p = 0; p < ENG_PROF_N; ++p) before[p] = eng_prof[p];
        int32_t out[1024];
        uint64_t begin = host_ns();
        /* Match the device's two <=256-frame engine calls per DMA half. */
        for (uint32_t offset = 0; offset < count;) {
            uint32_t n = count - offset;
            if (n > 256) n = 256;
            engine_render(out + 2u * offset, n);
            offset += n;
        }
        uint64_t dt = host_ns() - begin;
        /* Oracle work stays outside the measured render interval. */
        for (uint32_t i = 0; i < 2u * count; ++i) assert(out[i] == 0);
        elapsed += dt;
        if (dt > peak) peak = dt;
        for (unsigned p = 0; p < ENG_PROF_N; ++p) {
            uint32_t ticks = eng_prof[p] - before[p];
            stage_total[p] += ticks;
            if (ticks > stage_peak[p]) stage_peak[p] = ticks;
        }
        done += count;
    }
    static const char *const name[ENG_PROF_N] =
        {"909", "808", "Chicago", "Miami", "BRK", "FX", "MASTER", "SEQ"};
    printf("{\"kind\":\"host nanoseconds; target ISR/CPU NOT_RUN\",\"frames\":%u,"
           "\"audio_seconds\":10,\"render_total_ns\":%llu,\"render_peak_block_ns\":%llu,"
           "\"host_render_load_percent\":%.9f,\"stages\":[",
           frames, (unsigned long long)elapsed, (unsigned long long)peak,
           (double)elapsed / 100000000.0);
    for (unsigned p = 0; p < ENG_PROF_N; ++p)
        printf("%s{\"part\":\"%s\",\"total_ns\":%llu,\"peak_block_ns\":%llu,"
               "\"host_load_percent\":%.9f}", p ? "," : "", name[p],
               (unsigned long long)stage_total[p], (unsigned long long)stage_peak[p],
               (double)stage_total[p] / 100000000.0);
    puts("]}");
    return 0;
}

static void trigger_part(unsigned part)
{
    if (part < NKIT) {
        for (int voice = 0; voice < NDRUM; ++voice) engine_drum((int)part, voice, .8f);
    } else if (part < PART_BRK) {
        engine_bass_on((int)(part - PART_303A), 48, 1, 0);
    } else {
        engine_brk_live(0, 1);
    }
}

static int restart(unsigned part, uint32_t idle, uint32_t event_offset, const char *dir)
{
    assert(part < NPARTS && event_offset <= 512);
    setup_idle();
    silent_frames(idle);
    /* Offset splitting puts the real queued event at an interior sample, not
     * merely at a convenient whole-block boundary. The reference does the same. */
    silent_frames(event_offset);
    trigger_part(part);
    FILE *audio = open_trace(dir, "restart.i32");
    FILE *state = open_trace(dir, "peak-scope.bin");
    unsigned heard = 0;
    for (uint32_t done = 0; done < 44100;) {
        uint32_t n = 44100 - done;
        if (n > 256) n = 256;
        int32_t out[512];
        engine_render(out, n);
        put(audio, out, n * 2u * sizeof *out);
        for (uint32_t i = 0; i < 2 * n; ++i) heard |= out[i] != 0;
        for (unsigned p = 0; p < NPARTS; ++p) {
            uint16_t peak_value = eng_peak[p];
            put(state, &peak_value, sizeof peak_value);
        }
        put(state, eng_scope, sizeof eng_scope);
        uint32_t w = eng_scope_w;
        put(state, &w, sizeof w);
        done += n;
    }
    assert(heard); /* prevents a broken, permanently silent wakeup from passing */
    return fclose(audio) || fclose(state);
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "bench")) return benchmark();
    if (argc != 6 || strcmp(argv[1], "restart")) return 2;
    return restart((unsigned)strtoul(argv[2], 0, 10),
                   (uint32_t)strtoul(argv[3], 0, 10),
                   (uint32_t)strtoul(argv[4], 0, 10), argv[5]);
}
