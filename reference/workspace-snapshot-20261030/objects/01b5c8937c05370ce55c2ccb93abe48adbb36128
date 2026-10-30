/* SPDX-License-Identifier: GPL-3.0-only */
/* The 909 cymbals as IMA ADPCM (drum909.c, ima_adpcm.h, tools/gen_drum_samples.py):
 *   1. the firmware decoder gives the reference samples (tests/host/cymbal_ref.py: the approved
 *      A/B study's decode when it is there) within 1 LSB, and the SNR against the original PCM;
 *   2. d9_render_smp's forward streaming decode gives bit-identical output to the old read loop
 *      on a fully decoded int16 buffer, at pitch 0.25 ... 4 (inc up to 4: up to 4 decodes a sample),
 *      and again after a retrigger part way through;
 *   3. the cost: decodes per output sample at each pitch, and the time per decode on the host.
 *   drum909_adpcm_test REFDIR */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../firmware/src/dsp/x0x_param.c"
#include "../../firmware/src/dsp/drum909.c"

static int16_t *load(const char *dir, const char *name, const char *ext, uint32_t n)
{
    char p[512];
    snprintf(p, sizeof p, "%s/%s.%s", dir, name, ext);
    FILE *f = fopen(p, "rb");
    if (!f) { printf("FAIL cannot open %s\n", p); exit(1); }
    int16_t *b = malloc(n * 2);
    if (fread(b, 2, n, f) != n) { printf("FAIL short %s\n", p); exit(1); }
    fclose(f);
    return b;
}

/* the read loop before this change (int16 buffer), mute and envelope as 1 */
static uint32_t play_pcm(const int16_t *buf, uint32_t len, float pitch, float *out, uint32_t max)
{
    d9_smp_t s;
    memset(&s, 0, sizeof s);
    s.pitch = pitch;
    d9_smp_rate(&s);
    uint32_t k = 0;
    while (s.pos < len && k < max) {
        const float a = (float)buf[s.pos] * (1.0f / 32768.0f);
        const float b = s.pos + 1 < len ? (float)buf[s.pos + 1] * (1.0f / 32768.0f) : 0.0f;
        const float fr = (float)s.frac * (1.0f / 4294967296.0f);
        out[k++] = a + (b - a) * fr;
        const uint32_t f2 = s.frac + s.incf;
        s.pos += s.inc + (f2 < s.frac);
        s.frac = f2;
    }
    return k;
}

static void trig(d9_smp_t *s)
{
    s->pos = s->frac = 0;
    s->dk = 0;
    s->pred = s->idx = 0;
    s->h0 = s->h1 = 0;
    s->playing = 1;
    s->mute = 0x7FFFFFFF;
    d9_env_set(&s->out, 1.0f);
}

/* the firmware render (streaming ADPCM), in blocks of 32; returns samples until pos >= len */
static uint32_t play_adpcm(d9_smp_t *s, float *out, uint32_t max, uint32_t *decodes)
{
    static float dry[32];
    d9_bus_t bus;
    memset(&bus, 0, sizeof bus);
    bus.dry = dry;
    uint32_t k = 0;
    while (s->pos < s->len && k < max) {
        memset(dry, 0, sizeof dry);
        d9_render_smp(s, &bus, 32);
        for (int i = 0; i < 32 && k < max; i++)
            out[k++] = dry[i];
    }
    if (decodes) *decodes = s->dk;
    return k;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s REFDIR\n", argv[0]); return 2; }
    struct { const char *name; const uint8_t *adpcm; uint32_t len, bytes; } C[3] = {
        {"hh", x0x_smp_hh, X0X_SMP_HH_LEN, sizeof x0x_smp_hh},
        {"ride", x0x_smp_ride, X0X_SMP_RIDE_LEN, sizeof x0x_smp_ride},
        {"crash", x0x_smp_crash, X0X_SMP_CRASH_LEN, sizeof x0x_smp_crash}};
    static const float PITCH[] = {0.25f, 0.5f, 0.7937005f, 1.0f, 1.3348398f, 2.0f, 3.1748021f, 4.0f};
    int bad = 0;
    uint32_t flash = 0, pcm_bytes = 0;
    for (int c = 0; c < 3; c++) {
        const uint32_t n = C[c].len;
        int16_t *ref = load(argv[1], C[c].name, "ref", n), *pcm = load(argv[1], C[c].name, "pcm", n);
        int16_t *dec = malloc(n * 2);
        int32_t pred = 0, idx = 0, maxd = 0;
        double es = 0, ps = 0;
        for (uint32_t i = 0; i < n; i++) {
            dec[i] = (int16_t)ima_decode(C[c].adpcm, i, &pred, &idx);
            int32_t d = abs(dec[i] - ref[i]);
            if (d > maxd) maxd = d;
            es += (double)(dec[i] - pcm[i]) * (dec[i] - pcm[i]);
            ps += (double)pcm[i] * pcm[i];
        }
        flash += C[c].bytes;
        pcm_bytes += n * 2;
        const double snr = 10.0 * log10(ps / es);
        const int ok = maxd <= 1 && C[c].bytes == (n + 1) / 2 && snr > 15.0;
        printf("  %s %-5s %6u samples, %5u B ADPCM (int16 %6u B), max |decode - ref| %d LSB, SNR %.1f dB\n",
               ok ? "ok  " : "FAIL", C[c].name, n, C[c].bytes, n * 2, maxd, snr);
        bad += !ok;

        /* streaming against the full decode, every pitch */
        float *a = malloc(sizeof(float) * (4 * n + 64)), *b = malloc(sizeof(float) * (4 * n + 64));
        for (unsigned p = 0; p < sizeof PITCH / sizeof PITCH[0]; p++) {
            d9_smp_t s;
            memset(&s, 0, sizeof s);
            s.adpcm = C[c].adpcm;
            s.len = n;
            s.pitch = PITCH[p];
            d9_smp_rate(&s);
            trig(&s);
            uint32_t dk = 0;
            const uint32_t na = play_pcm(dec, n, PITCH[p], a, 4 * n + 64);
            const uint32_t nb = play_adpcm(&s, b, 4 * n + 64, &dk);
            uint32_t diff = 0;
            for (uint32_t i = 0; i < na && i < nb; i++)
                diff += memcmp(&a[i], &b[i], sizeof(float)) != 0;
            /* nb is rounded up to the block: the samples after the end are 0 */
            for (uint32_t i = na; i < nb; i++)
                diff += b[i] != 0.0f;
            const int pk = na <= nb && diff == 0 && dk <= n && dk + 4 >= n;   /* the last few are skipped at high pitch */
            if (!pk || p == 0 || PITCH[p] == 4.0f)
                printf("  %s %-5s pitch %.3f: %u samples, %u differ, %u decodes (%.2f per output sample)\n",
                       pk ? "ok  " : "FAIL", C[c].name, (double)PITCH[p], na, diff, dk, (double)dk / na);
            bad += !pk;
        }
        /* a retrigger part way through: decoding starts again from the first nibble */
        {
            d9_smp_t s;
            memset(&s, 0, sizeof s);
            s.adpcm = C[c].adpcm;
            s.len = n;
            s.pitch = 1.3348398f;
            d9_smp_rate(&s);
            trig(&s);
            static float dry[32];
            d9_bus_t bus;
            memset(&bus, 0, sizeof bus);
            bus.dry = dry;
            for (int k = 0; k < 40; k++)
                d9_render_smp(&s, &bus, 32);
            trig(&s);
            const uint32_t na = play_pcm(dec, n, s.pitch, a, 4 * n + 64);
            const uint32_t nb = play_adpcm(&s, b, 4 * n + 64, NULL);
            uint32_t diff = 0;
            for (uint32_t i = 0; i < na && i < nb; i++)
                diff += memcmp(&a[i], &b[i], sizeof(float)) != 0;
            printf("  %s %-5s retrigger at 1280 samples: %u differ\n", diff ? "FAIL" : "ok  ", C[c].name, diff);
            bad += diff != 0;
        }
        free(a); free(b); free(dec); free(ref); free(pcm);
    }
    printf("  cymbals: %u B of ADPCM in flash, %u B as int16, %u B saved\n", flash, pcm_bytes, pcm_bytes - flash);

    /* host time per decode (a rough guide only; the device estimate is in BEHAVIOR.md DRM-12) */
    {
        volatile int32_t sink = 0;
        int32_t pred = 0, idx = 0;
        const clock_t t0 = clock();
        const int R = 200;
        for (int r = 0; r < R; r++) {
            pred = idx = 0;
            for (uint32_t i = 0; i < X0X_SMP_RIDE_LEN; i++)
                sink += ima_decode(x0x_smp_ride, i, &pred, &idx);
        }
        const double ns = (double)(clock() - t0) / CLOCKS_PER_SEC * 1e9 / ((double)R * X0X_SMP_RIDE_LEN);
        printf("  host: %.2f ns per decode (%d)\n", ns, (int)(sink & 1));
    }
    puts(bad ? "DRUM909 ADPCM FAILED" : "drum909 adpcm: decode and streaming ok");
    return bad ? 1 : 0;
}
