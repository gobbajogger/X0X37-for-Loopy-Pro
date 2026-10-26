/* SPDX-License-Identifier: GPL-3.0-only
 * Offline native303 render for Chicago AU scenario battery (C-SND-1).
 * Reads a line-oriented control file from stdin; writes stereo float32 LE to stdout.
 *
 * Usage: chicago_au_render < control.txt > out.f32
 *
 * Control file (one directive per line, # comments):
 *   frames N          total samples per channel timeline @ 48 kHz host clock
 *   note_on F         sample index for note-on
 *   note_off F        sample index for note-off
 *   midi_note N       0..127
 *   velocity N        1..127
 *   pot I V           bass303 pot index and 0..127 value
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/bass303.h"

#define HOST_SR 48000.0f
#define RENDER_SR 44100.0f
#define RATIO (RENDER_SR / HOST_SR)

static int read_ctl(bass303_t *b, int *frames, int *note_on, int *note_off, int *midi, int *vel)
{
    char line[256];
    *frames = 48000;
    *note_on = 4800;
    *note_off = 24000;
    *midi = 48;
    *vel = 80;
    bass303_init(b);
    while (fgets(line, sizeof line, stdin)) {
        int a, c;
        float f;
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
        if (sscanf(line, "pot %d %d", &a, &c) == 2) {
            if (a >= 0 && a < BASS303_NPARAMS)
                bass303_set(b, a, c);
            continue;
        }
        fprintf(stderr, "chicago_au_render: unknown line: %s", line);
        return -1;
    }
    return 0;
}

int main(void)
{
    bass303_t b;
    int host_frames, note_on, note_off, midi, vel, host_pos, note_down;
    float out[512];
    int i;

    if (read_ctl(&b, &host_frames, &note_on, &note_off, &midi, &vel) < 0)
        return 2;
    note_down = 0;
    for (host_pos = 0; host_pos < host_frames; host_pos++) {
        int render_pos = (int)((float)host_pos * RATIO);
        int next_render = (int)((float)(host_pos + 1) * RATIO);
        int n = next_render - render_pos;
        if (n < 1)
            continue;
        if (!note_down && host_pos >= note_on) {
            bass303_note_on_velocity(&b, midi, 0, 0, vel);
            note_down = 1;
        }
        if (note_down && host_pos >= note_off) {
            bass303_note_off(&b);
            note_down = 0;
        }
        while (n > 0) {
            int m = n > 256 ? 256 : n;
            bass303_render(&b, out, m);
            for (i = 0; i < m; i++) {
                float s = out[i];
                if (fwrite(&s, sizeof(float), 1, stdout) != 1)
                    return 1;
                if (fwrite(&s, sizeof(float), 1, stdout) != 1)
                    return 1;
            }
            n -= m;
        }
    }
    return 0;
}
