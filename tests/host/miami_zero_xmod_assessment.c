/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused host assessment. No production arithmetic changes. */
#include "original_miami_backend.h"
#include "binary32_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int kernel_fn(original_miami_ms20 *, original_voice_gain *, original_oscillators *,
    const original_prepared_oscillators *, float, float *, float *, uint32_t);
extern kernel_fn original_miami_voice_render_add, fixture_miami_voice_render_add;
static unsigned calls;
static unsigned long long pairs, zero_pairs, carrier_pairs, zero_carrier_pairs, audible_pairs;

/* Called only by the generated counter copy, at the original gain-read point.
 * "carrier" means the even/master oscillator's CURRENT gain is nonzero.
 * "audible pair" means at least one current gain is nonzero; PCM is checked
 * separately. Neither term asserts perceptual audibility. */
void assessment_count(float carrier, float slave, float xmod)
{
    ++pairs;
    zero_pairs += xmod == 0.0f;
    carrier_pairs += carrier != 0.0f;
    zero_carrier_pairs += xmod == 0.0f && carrier != 0.0f;
    audible_pairs += carrier != 0.0f || slave != 0.0f;
}

int fixture_dispatch(original_miami_ms20 *f, original_voice_gain *g, original_oscillators *p,
    const original_prepared_oscillators *c, float vco, float *l, float *r, uint32_t n)
{
    original_miami_ms20 rf = *f;
    original_voice_gain rg = *g;
    original_oscillators rp = *p;
    float rl[66], rr[66];
    if (n > 64) { fputs("invalid kernel span\n", stderr); exit(2); }
    memcpy(rl, l, n * sizeof(float)); memcpy(rr, r, n * sizeof(float));
    rl[n] = rr[n] = 123.25f;
    int a = original_miami_voice_render_add(&rf, &rg, &rp, c, vco, rl, rr, n);
    int b = fixture_miami_voice_render_add(f, g, p, c, vco, l, r, n);
    ++calls;
    if (memcmp(&rp, p, sizeof rp)) {
        fprintf(stderr, "PHASE_MISMATCH call=%u frames=%u reference=%08x actual=%08x\n",
            calls, n, rp.phase[1], p->phase[1]); exit(23);
    }
    if (a != b || !a || memcmp(&rf, f, sizeof rf) || memcmp(&rg, g, sizeof rg) ||
        memcmp(rl, l, n * sizeof(float)) || memcmp(rr, r, n * sizeof(float)) ||
        rl[n] != 123.25f || rr[n] != 123.25f) {
        fprintf(stderr, "KERNEL_MISMATCH call=%u frames=%u\n", calls, n); exit(24);
    }
    return b;
}

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "0") && strcmp(argv[1], "15"))) return 2;
    unsigned factory = (unsigned)strtoul(argv[1], NULL, 10);
    static original_miami_backend backend;
    uint32_t l[258], r[258];
    int32_t dl[256], dr[256]; int16_t sl[256], sr[256];
    original_miami_fx_workspace w = {dl, dr, sl, sr, 256};
    if (!original_miami_backend_init(&backend, factory, 120.0f) ||
        !original_miami_backend_note_on(&backend, 60, 100)) return 3;
    FILE *pcm = fopen(argv[2], "wb");
    if (!pcm) return 4;
    unsigned long long nonzero = 0;
    for (unsigned block = 0; block < 128; ++block) {
        l[256] = r[256] = UINT32_C(0x12345678);
        if (!original_miami_backend_render_words(&backend, &w, l, r, 256)) return 5;
        if (l[256] != UINT32_C(0x12345678) || r[256] != UINT32_C(0x12345678)) return 6;
        for (unsigned j = 0; j < 256; ++j) {
            if ((l[j] & UINT32_C(0x7fffffff)) || (r[j] & UINT32_C(0x7fffffff))) ++nonzero;
            /* Explicit interleaved LE32 representation, independent of host byte order. */
            uint32_t samples[2] = {l[j], r[j]};
            for (unsigned ch = 0; ch < 2; ++ch) for (unsigned byte = 0; byte < 4; ++byte)
                if (fputc((int)((samples[ch] >> (8 * byte)) & 255u), pcm) == EOF) return 7;
        }
    }
    if (fclose(pcm) || !calls || !nonzero) return 8;
    printf("{\"factory\":%u,\"xmod_bits\":\"%08x\",\"frames\":32768,"
        "\"nonzero_pcm_frames\":%llu,\"kernel_calls\":%u,\"pair_samples\":%llu,"
        "\"zero_xmod_pairs\":%llu,\"nonzero_carrier_pairs\":%llu,"
        "\"zero_xmod_nonzero_carrier_pairs\":%llu,\"nonzero_gain_pairs\":%llu}\n",
        factory, original_math_bits(backend.controls.xmod), nonzero, calls, pairs,
        zero_pairs, carrier_pairs, zero_carrier_pairs, audible_pairs);
    return 0;
}
