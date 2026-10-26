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
 *   fx_type T         Chicago Fx Type 0..11 (Crush/Ring/EQ/Cho/Ens/Phaser/SDly/BPM/Hall/GateRev insert when T matches)
 *   fx_edit1 V        Edit1 0..127 (Crush / Ring / EQ / Cho / Ens / Phaser / SDly / BPM / Hall / GateRev wet)
 *   fx_edit2 V        Edit2 0..127 (wet / depth / feedback / BPM division / Hall decay / Gate hold)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/bass303.h"
#include "../../firmware/src/app/chicago_fx.h"
#include "../../firmware/src/app/chicago_crush.h"
#include "../../firmware/src/app/chicago_ring.h"
#include "../../firmware/src/app/chicago_eq.h"
#include "../../firmware/src/app/chicago_cho.h"
#include "../../firmware/src/app/chicago_ens.h"
#include "../../firmware/src/app/chicago_phaser.h"
#include "../../firmware/src/app/chicago_sdly.h"
#include "../../firmware/src/app/chicago_bpmdly.h"
#include "../../firmware/src/app/chicago_hall.h"
#include "../../firmware/src/app/chicago_gaterev.h"

static int read_ctl(bass303_t *b, chi_crush_t *crush, chi_ring_t *ring, chi_eq_t *eq, chi_cho_t *cho,
                    chi_ens_t *ens, chi_phaser_t *phaser, chi_sdly_t *sdly, chi_bpmdly_t *bpmdly,
                    chi_hall_t *hall, chi_gaterev_t *grev, int *frames, int *note_on, int *note_off,
                    int *midi, int *vel, uint8_t *fx_type)
{
    char line[256];
    uint8_t e1 = 64, e2 = 64;
    *frames = 48000;
    *note_on = 4800;
    *note_off = 24000;
    *midi = 48;
    *vel = 80;
    *fx_type = CHI_FX_OFF;
    bass303_init(b);
    chi_crush_init(crush);
    chi_ring_init(ring);
    chi_eq_init(eq);
    chi_cho_init(cho);
    chi_ens_init(ens);
    chi_phaser_init(phaser);
    chi_sdly_init(sdly);
    chi_bpmdly_init(bpmdly);
    chi_hall_init(hall);
    chi_gaterev_init(grev);
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
        if (sscanf(line, "fx_type %d", &a) == 1) {
            if (a < 0)
                a = 0;
            if (a >= CHI_FX_NTYPES)
                a = CHI_FX_OFF;
            *fx_type = (uint8_t)a;
            continue;
        }
        if (sscanf(line, "fx_edit1 %d", &a) == 1) {
            if (a < 0)
                a = 0;
            if (a > 127)
                a = 127;
            e1 = (uint8_t)a;
            continue;
        }
        if (sscanf(line, "fx_edit2 %d", &a) == 1) {
            if (a < 0)
                a = 0;
            if (a > 127)
                a = 127;
            e2 = (uint8_t)a;
            continue;
        }
        fprintf(stderr, "chicago_au_render: unknown line: %s", line);
        return -1;
    }
    if (chi_fx_is_crush_insert(*fx_type))
        chi_crush_set(crush, e1, e2);
    else if (chi_fx_is_ring_insert(*fx_type))
        chi_ring_set(ring, e1, e2, BASS303_SR);
    else if (chi_fx_is_eq_insert(*fx_type))
        chi_eq_set(eq, e1, e2, BASS303_SR);
    else if (chi_fx_is_cho_insert(*fx_type))
        chi_cho_set(cho, e1, e2, BASS303_SR);
    else if (chi_fx_is_ens_insert(*fx_type))
        chi_ens_set(ens, e1, e2, BASS303_SR);
    else if (chi_fx_is_phaser_insert(*fx_type))
        chi_phaser_set(phaser, e1, e2, BASS303_SR);
    else if (chi_fx_is_sdly_insert(*fx_type))
        chi_sdly_set(sdly, e1, e2, BASS303_SR);
    else if (chi_fx_is_bpmdly_insert(*fx_type))
        chi_bpmdly_set(bpmdly, e1, e2, BASS303_SR);
    else if (chi_fx_is_hall_insert(*fx_type))
        chi_hall_set(hall, e1, e2, BASS303_SR);
    else if (chi_fx_is_gaterev_insert(*fx_type))
        chi_gaterev_set(grev, e1, e2, BASS303_SR);
    return 0;
}

int main(void)
{
    bass303_t b;
    chi_crush_t crush;
    chi_ring_t ring;
    chi_eq_t eq;
    chi_cho_t cho;
    chi_ens_t ens;
    chi_phaser_t phaser;
    chi_sdly_t sdly;
    chi_bpmdly_t bpmdly;
    chi_hall_t hall;
    chi_gaterev_t grev;
    int frames, note_on, note_off, midi, vel, pos, note_down;
    uint8_t fx_type;
    float out[512];

    if (read_ctl(&b, &crush, &ring, &eq, &cho, &ens, &phaser, &sdly, &bpmdly, &hall, &grev, &frames,
                 &note_on, &note_off, &midi, &vel, &fx_type) < 0)
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
        if (chi_fx_is_crush_insert(fx_type))
            chi_crush_process(&crush, out, 1);
        else if (chi_fx_is_ring_insert(fx_type))
            chi_ring_process(&ring, out, 1);
        else if (chi_fx_is_eq_insert(fx_type))
            chi_eq_process(&eq, out, 1);
        else if (chi_fx_is_cho_insert(fx_type))
            chi_cho_process(&cho, out, 1);
        else if (chi_fx_is_ens_insert(fx_type))
            chi_ens_process(&ens, out, 1);
        else if (chi_fx_is_phaser_insert(fx_type))
            chi_phaser_process(&phaser, out, 1);
        else if (chi_fx_is_sdly_insert(fx_type))
            chi_sdly_process(&sdly, out, 1);
        else if (chi_fx_is_bpmdly_insert(fx_type))
            chi_bpmdly_process(&bpmdly, out, 1);
        else if (chi_fx_is_hall_insert(fx_type))
            chi_hall_process(&hall, out, 1);
        else if (chi_fx_is_gaterev_insert(fx_type))
            chi_gaterev_process(&grev, out, 1);
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
