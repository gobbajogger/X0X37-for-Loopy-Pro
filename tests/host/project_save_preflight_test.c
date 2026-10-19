/* SPDX-License-Identifier: GPL-3.0-only */
/* A full stream must not update even settings or sound before MEMORY FULL. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/project.c"

seq_t seq;
static uint8_t flash_obj[OBJ_NOBJ][PLAT_STORE_MAX];
static uint32_t flash_len[OBJ_NOBJ], writes;
static project_t saved_project;

int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    if (obj >= OBJ_NOBJ || len > PLAT_STORE_MAX)
        return -1;
    memcpy(flash_obj[obj], src, len);
    flash_len[obj] = len;
    writes++;
    return 0;
}

int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    if (obj >= OBJ_NOBJ || !flash_len[obj] || flash_len[obj] > max)
        return -1;
    memcpy(dst, flash_obj[obj], flash_len[obj]);
    return (int)flash_len[obj];
}

void engine_sound_defaults(sound_t *s) { memset(s, 0, sizeof *s); }
void engine_sound_centre_drum_pans(sound_t *s) { (void)s; }
int engine_nparams(int t, int v) { (void)t; (void)v; return 0; }
const x0x_param_t *engine_param(int t, int v, int i)
{
    static const x0x_param_t unused;
    (void)t; (void)v; (void)i;
    return &unused;
}
void pattern_init(pattern_t *p, uint32_t a, uint32_t b)
{
    (void)a; (void)b;
    memset(p, 0, sizeof *p);
}

int main(void)
{
    uint8_t *raw = (uint8_t *)&proj + RAW_OFF;
    uint32_t rng = 1;
    int i;

    project_defaults();
    proj.set.palette = 1;
    proj.sound.v[T_303][0][0] = 42;
    proj.pat[0].bass[0].step[0].note = 48;
    seq.bpm = 125.0f;
    seq.accent_q7 = 88;
    if (project_save() || !writes) {
        puts("initial project save failed");
        return 1;
    }
    memcpy(&saved_project, &proj, sizeof proj);

    /* Deterministic incompressible content exceeds the four stream objects. */
    for (i = 0; i < (int)RAW_LEN; i++) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        raw[i] = (uint8_t)rng;
    }
    proj.set.palette = 2;
    proj.sound.v[T_303][0][0] = 99;
    seq.bpm = 140.0f;
    writes = 0;
    if (project_save() != -7 || writes) {
        printf("memory-full save changed %u objects\n", (unsigned)writes);
        return 1;
    }

    /* Model power cycling: only flash contents remain. */
    memset(&proj, 0, sizeof proj);
    memset(stored_known, 0, sizeof stored_known);
    memset(stored_hash, 0, sizeof stored_hash);
    packed_len = 0;
    if (project_load() || memcmp(&proj, &saved_project, sizeof proj)) {
        puts("power cycle did not recover the last saved project");
        return 1;
    }
    puts("memory-full preflight: no writes; prior project survives reboot");
    return 0;
}
