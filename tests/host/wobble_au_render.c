/* SPDX-License-Identifier: GPL-3.0-only
 * Offline wobble render for Miami AU factory preset battery.
 * stdin control file; stdout stereo float32 LE interleaved @ 48 kHz.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../experiments/wobble/wobble.h"

static int read_ctl(wobble_t *w, int *frames, int *note_on, int *note_off, int *midi, int *vel)
{
    char line[256];
    *frames = 48000;
    *note_on = 4800;
    *note_off = 24000;
    *midi = 48;
    *vel = 80;
    wobble_init(w, 48000.0f);
    while (fgets(line, sizeof line, stdin)) {
        int a;
        float f, v;
        if (line[0] == '#' || line[0] == '\n')
            continue;
        if (sscanf(line, "frames %d", &a) == 1) {
            *frames = a;
            continue;
        }
        if (sscanf(line, "note_on %f", &f) == 1) {
            *note_on = (int)(f + 0.5f);
            continue;
        }
        if (sscanf(line, "note_off %f", &f) == 1) {
            *note_off = (int)(f + 0.5f);
            continue;
        }
        if (sscanf(line, "midi_note %d", &a) == 1) {
            *midi = a;
            continue;
        }
        if (sscanf(line, "velocity %d", &a) == 1) {
            *vel = a;
            continue;
        }
        if (sscanf(line, "wobble %d %f", &a, &v) == 2) {
            if (a >= 0 && a < (int)W_PARAMETER_COUNT)
                wobble_set(w, (unsigned)a, v);
            continue;
        }
        fprintf(stderr, "wobble_au_render: unknown line: %s", line);
        return -1;
    }
    return 0;
}

int main(void)
{
    wobble_t w;
    int frames, note_on, note_off, midi, vel;
    if (read_ctl(&w, &frames, &note_on, &note_off, &midi, &vel) < 0)
        return 2;
    if (frames < 1 || note_on < 0 || note_off <= note_on || note_off > frames)
        return 2;
    wobble_note_on(&w, (uint8_t)midi, (uint8_t)vel);
    float l[256], r[256], lr[512];
    for (int p = 0; p < frames;) {
        if (p == note_off)
            wobble_note_off(&w, (uint8_t)midi);
        int n = frames - p;
        if (n > 256)
            n = 256;
        wobble_render(&w, l, r, (size_t)n);
        for (int i = 0; i < n; i++) {
            lr[i * 2] = l[i];
            lr[i * 2 + 1] = r[i];
        }
        if (fwrite(lr, sizeof(float) * 2, (size_t)n, stdout) != (size_t)n)
            return 1;
        p += n;
    }
    return 0;
}
