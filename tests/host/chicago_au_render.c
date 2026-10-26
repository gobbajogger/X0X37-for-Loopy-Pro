/* SPDX-License-Identifier: GPL-3.0-only
 * Offline native303 render for Chicago AU scenario battery (C-SND-1).
 * Reads a line-oriented control file from stdin; writes stereo float32 LE to stdout.
 *
 * Built at BASS303_SR=48000 (see run_chicago_au_render.sh) so PCM matches AU refs.
 *
 * Usage: chicago_au_render < control.txt > out.f32
 *
 * Control file (one directive per line, # comments):
 *   frames N          total samples per channel @ scenario sampleRate (48 kHz)
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
    int frames, note_on, note_off, midi, vel, pos, note_down;
    float out[512];

    if (read_ctl(&b, &frames, &note_on, &note_off, &midi, &vel) < 0)
        return 2;
    note_down = 0;
    for (pos = 0; pos < frames; pos++) {
        if (!note_down && pos >= note_on) {
            bass303_note_on_velocity(&b, midi, 0, 0, vel);
            note_down = 1;
        }
        if (note_down && pos >= note_off) {
            bass303_note_off(&b);
            note_down = 0;
        }
        bass303_render(&b, out, 1);
        {
            float s = out[0];
            if (fwrite(&s, sizeof(float), 1, stdout) != 1)
                return 1;
            if (fwrite(&s, sizeof(float), 1, stdout) != 1)
                return 1;
        }
    }
    return 0;
}
