/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline audio/timing probe. Compare matched source builds with the same inputs.
 * stdout is timing metadata; argv[1] receives raw native float samples. */
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "bass303.h"

static int emit(FILE *f, bass303_t *b, int n)
{
    float out[128];
    while (n) {
        int i, m = n > 128 ? 128 : n;
        bass303_render(b, out, m);
        for (i = 0; i < m; i++)
            if (!isfinite(out[i]))
                return 1;
        if (fwrite(out, sizeof(*out), (size_t)m, f) != (size_t)m)
            return 1;
        n -= m;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int lite, mode, drive, wave, failed = 0;
    clock_t start;
    FILE *f;
    if (argc != 2 || !(f = fopen(argv[1], "wb")))
        return 2;
    start = clock();
    for (lite = 0; lite < 2; lite++)
        for (mode = 0; mode < 2; mode++)
            for (drive = 0; drive < 3; drive++)
                for (wave = 0; wave < 2; wave++) {
                    bass303_t b;
                    bass303_init(&b);
                    bass303_set_lite(&b, lite);
                    bass303_set(&b, BASS303_FENV, mode);
                    bass303_set(&b, BASS303_FSUSTAIN, 64);
                    bass303_set(&b, BASS303_FATTACK, 12);
                    bass303_set(&b, BASS303_FRELEASE, 20);
                    bass303_set(&b, BASS303_DECAY, 0);
                    bass303_set(&b, BASS303_DRVTYPE, drive);
                    bass303_set(&b, BASS303_DRIVE, 80);
                    bass303_set(&b, BASS303_WAVE, wave);
                    bass303_note_on_velocity(&b, 45, 0, 0, 72);
                    failed |= emit(f, &b, 44100);
                    bass303_note_on_velocity(&b, 57, 1, 1, 120);
                    failed |= emit(f, &b, 22050);
                    bass303_note_off(&b);
                    failed |= emit(f, &b, 44100);
                    bass303_note_on(&b, 36, 0, 0);
                    failed |= emit(f, &b, 22050);
                }
    printf("cases=24 frames=3175200 cpu_seconds=%.6f finite=%s\n",
           (double)(clock() - start) / (double)CLOCKS_PER_SEC, failed ? "FAIL" : "PASS");
    if (fclose(f))
        failed = 1;
    return failed;
}
