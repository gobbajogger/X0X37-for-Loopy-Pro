/* SPDX-License-Identifier: GPL-3.0-only */
/* The USB audio OUTPUT (usb.c, host -> SMK, EP4 OUT adaptive) against clocks that do not agree:
 * the host sends a 1 ms packet of the 44.1 pattern by its USB clock (as uac_out_rx stores it), the
 * SMK's I2S renders 512-frame halves (two 256-frame blocks) at RATE_IN and uac_out_mix adds the
 * ring into them. A 1 kHz sine goes in; after 20 s it must come out at 1 kHz by the host's clock
 * (the resampler took up the difference), clean, with no underrun once settled, the ring's fill
 * near UO_MID, and mixed on top of what the engines rendered (a constant offset here). */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
#define FELUCCA_UAC 1
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "usb.c"

#define SECS 40
static int32_t outL[50000 * SECS];

static int run(double rate_in)
{
    double t_in = 0, t_out = 0, ph = 0, err = 0, sig = 0, f, w;
    static int32_t blk[512];
    long nout = 0, a, i, zc = 0;
    uint32_t acc = 0, under_settled = 0, fmin = 0xFFFFFFFFu, fmax = 0;
    memset(&uo, 0, sizeof uo);
    uo_w = uo_r = 0;
    uo.alt = 1;
    a = (long)(rate_in * 20);
    while (t_in < SECS) {
        if (t_out <= t_in) {                             /* the host's 1 ms packet */
            uint32_t n = 44u, k;
            acc += 100u;
            if (acc >= 1000u) { acc -= 1000u; n++; }
            for (k = 0; k < n && uo_w - uo_r < UO_N; k++, ph += 1000.0 / 44100.0) {
                int16_t v = (int16_t)lrint(sin(2 * M_PI * ph) * 16000.0);
                uo_ring[uo_w & (UO_N - 1u)] = (uint32_t)(uint16_t)v | (uint32_t)(uint16_t)v << 16;
                uo_w++;
            }
            t_out += 0.001;
        } else {                                         /* a half: two blocks */
            int b, k;
            uint32_t u = uo.underruns, fill = uo_w - uo_r;
            uac_out_half();
            if (t_in > 10.0) {
                if (fill < fmin) fmin = fill;
                if (fill > fmax) fmax = fill;
            }
            for (b = 0; b < 2; b++) {
                for (k = 0; k < 512; k++)
                    blk[k] = 1000 << 8;                  /* the engines: a DC offset, mixed under */
                uac_out_mix(blk, 256);
                for (k = 0; k < 256 && nout < 50000L * SECS; k++)
                    outL[nout++] = (blk[2 * k] >> 8) - 1000;
            }
            if (t_in > 10.0 && uo.underruns != u)
                under_settled++;
            t_in += 512.0 / rate_in;
        }
    }
    for (i = a + 1; i < nout; i++)
        zc += (outL[i - 1] < 0) != (outL[i] < 0);
    f = zc / 2.0 * rate_in / (double)(nout - a);       /* cycles per real second (the SMK's clock) */
    w = 2 * M_PI * (zc / 2.0 / (double)(nout - a));     /* radians per output sample */
    for (i = a + 2; i < nout; i++) {
        double pred = 2 * cos(w) * outL[i - 1] - outL[i - 2];
        err += (outL[i] - pred) * (outL[i] - pred);
        sig += (double)outL[i] * outL[i];
    }
    {
        double expect = 1000.0;                       /* the host's tone, its clock matched */
        printf("  I2S %.0f Hz: tone %.3f Hz, residual %.1f dB, %u underruns after 10 s, fill %u..%u (UO_MID %d)\n",
               rate_in, f, 10 * log10(err / sig), under_settled, fmin, fmax, UO_MID);
        return fabs(f - expect) < 0.5 && 10 * log10(err / sig) < -60.0 && under_settled == 0 &&
               fmin > UO_LOW && fmax < UO_N - 2u * UA_MAXF;
    }
}

int main(void)
{
    int ok = run(44145.0) & run(44180.0) & run(44100.0) & run(44300.0) & run(43950.0);
    printf(ok ? "uac-out: ok\n" : "uac-out: FAILED\n");
    return !ok;
}
