/* SPDX-License-Identifier: GPL-3.0-only */
/* engine_brk_outline peak store: negative IMA full-scale must not wrap to blank. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define X0X_IMA_TABLES_DEFINE
#include "../../firmware/src/dsp/ima_adpcm.h"

/* Same clamp as firmware/src/app/engine.c brk_outline_peak_u8. */
static uint8_t brk_outline_peak_u8(int32_t pk)
{
    return (uint8_t)(pk >= 32768 ? 255 : pk >> 7);
}

static int outline(const uint8_t *adpcm, uint32_t ns, uint8_t *peaks, int n)
{
    uint32_t i;
    int32_t pred = 0, idx = 0, col = 0, pk = 0;
    uint32_t per;

    if (!adpcm || !ns || n <= 0)
        return 0;
    per = (ns + (uint32_t)n - 1u) / (uint32_t)n;
    for (i = 0; i < ns; i++) {
        int32_t s = ima_decode(adpcm, i, &pred, &idx);
        int32_t c = (int32_t)(i / per);
        if (c != col) {
            peaks[col] = brk_outline_peak_u8(pk);
            col = c;
            pk = 0;
        }
        if ((s < 0 ? -s : s) > pk)
            pk = s < 0 ? -s : s;
    }
    peaks[col] = brk_outline_peak_u8(pk);
    for (col++; col < n; col++)
        peaks[col] = 0;
    return 1;
}

static int count_nz(const uint8_t *peaks, int n)
{
    int i, nz = 0;
    for (i = 0; i < n; i++)
        if (peaks[i])
            nz++;
    return nz;
}

int main(void)
{
    uint8_t adpcm[4096], peaks[232];
    int fails = 0;
    int32_t neg = -32768, pos = 32767;

    /* Unit: abs(-32768) must map to 255, not (uint8_t)256 → 0. */
    if (brk_outline_peak_u8(neg < 0 ? -neg : neg) != 255) {
        fprintf(stderr, "FAIL: abs(-32768) peak store want 255 got %u\n",
                (unsigned)brk_outline_peak_u8(neg < 0 ? -neg : neg));
        fails++;
    }
    if (brk_outline_peak_u8(pos < 0 ? -pos : pos) != 255) {
        fprintf(stderr, "FAIL: +32767 peak store want 255 got %u\n",
                (unsigned)brk_outline_peak_u8(pos < 0 ? -pos : pos));
        fails++;
    }
    if (brk_outline_peak_u8(0) != 0) {
        fprintf(stderr, "FAIL: silence peak store want 0 got %u\n",
                (unsigned)brk_outline_peak_u8(0));
        fails++;
    }

    /* Synthetic IMA: nibble 0xF saturates negative; 0x7 positive; 0x0 quiet. */
    memset(adpcm, 0xFF, sizeof adpcm);
    outline(adpcm, 7424, peaks, 232);
    if (count_nz(peaks, 232) != 232 || peaks[0] != 255 || peaks[231] != 255) {
        fprintf(stderr, "FAIL: neg full-scale outline nz=%d first=%u last=%u (want 232/255/255)\n",
                count_nz(peaks, 232), peaks[0], peaks[231]);
        fails++;
    }

    memset(adpcm, 0x77, sizeof adpcm);
    outline(adpcm, 7424, peaks, 232);
    if (count_nz(peaks, 232) != 232 || peaks[0] != 255 || peaks[231] != 255) {
        fprintf(stderr, "FAIL: pos full-scale outline nz=%d first=%u last=%u (want 232/255/255)\n",
                count_nz(peaks, 232), peaks[0], peaks[231]);
        fails++;
    }

    memset(adpcm, 0x00, sizeof adpcm);
    outline(adpcm, 7424, peaks, 232);
    if (count_nz(peaks, 232) != 0) {
        fprintf(stderr, "FAIL: silence outline nz=%d (want 0)\n", count_nz(peaks, 232));
        fails++;
    }

    if (fails) {
        fprintf(stderr, "%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS: brk outline peaks (neg/pos full-scale visible; silence blank)\n");
    return 0;
}
