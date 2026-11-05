/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole-engine capture of every Chicago factory on both synth parts: silence
 * before any note, a played pattern, a long tail into silence, then a restart.
 * Run on a baseline and a candidate build; the traces must be byte-identical.
 * Also prints host CPU time of the silent tail (host only, not target CPU). */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "wb");
    if (!f) return 2;
    boot();
    engine_guard_enable(0);
    pattern_t *p = &proj.pat[0];
    for (int b = 0; b < NBASS; ++b)
        for (int s = 0; s < 16; ++s) {
            p->bass[b].step[s].note = 36 + b * 12 + s % 7;
            p->bass[b].step[s].flags = G_NOTE | (s % 3 == 0 ? BS_ACCENT : 0);
            p->bass[b].step[s].velocity = 60 + s * 4;
        }
    master = 3072;
    unsigned count = original_sound_factories(FACTORY_BANK_CHICAGO);
    double tail = 0;
    uint32_t blk = 0;
    for (unsigned preset = 0; preset < count; ++preset) {
        for (int v = 0; v < NBASS; ++v)
            assert(engine_bass_select_preset(v, FACTORY_BANK_CHICAGO, (int)preset, &proj.sound) >= 0);
        for (int block = 0; block < 1400; ++block, ++blk) {
            int32_t lr[512];
            now_ms = (uint32_t)((uint64_t)blk * 256u * 1000u / 44100u);
            if (block == 120 || block == 1300) seq_start(&seq);
            if (block == 420) seq_stop(&seq);
            clock_t t0 = clock();
            engine_render(lr, 256);
            if (block >= 700 && block < 1300) tail += (double)(clock() - t0) / CLOCKS_PER_SEC;
            fwrite(lr, sizeof lr, 1, f);
        }
        seq_stop(&seq);
    }
    fprintf(stderr, "presets %u silent-tail host cpu %.3f s\n", count, tail);
    return fclose(f) != 0;
}
