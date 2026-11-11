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
/* Storage-only fixture: engine/parameter policy has separate real-DSP tests.
 * Keep the revision-13 project.c symbols wired without linking audio engines. */
int engine_nvoices(int t) { (void)t; return 0; }
int engine_bass_sound_bank(const sound_t *s, int voice) { (void)s; (void)voice; return 0; }
int engine_bass_sound_validate(const sound_t *s) { (void)s; return 0; }
int engine_bass_preset_pending(int voice) { (void)voice; return 0; }
/* 0.0.25 stream 14: pattern preset bounds and note-lock records */
unsigned original_sound_factories(unsigned kind) { return kind ? 30u : 81u; }
int engine_bass_note_validate(const lane_t *lane) { (void)lane; return 0; }
const x0x_param_t *engine_bass_bank_param(int bank, int param)
{
    static const x0x_param_t unused;
    (void)bank; (void)param; return &unused;
}
const x0x_param_t *breaks_param(int param)
{
    static const x0x_param_t unused = {.max = 127};
    (void)param; return &unused;
}
int engine_nparams(int t, int v) { (void)t; (void)v; return 0; }
const x0x_param_t *engine_param(int t, int v, int i)
{
    static const x0x_param_t unused;
    (void)t; (void)v; (void)i;
    return &unused;
}

/* BF-021: different full-size sound objects can share a legacy 32-bit FNV-1a
 * digest. The second must reach flash; only byte-identical saves may be skipped. */
static uint32_t legacy_fnv32(const uint8_t *p, uint32_t n)
{
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ *p++) * 16777619u;
    return h;
}
static int test_sound_hash_collision(void)
{
    static const uint8_t a[6] = {0x0a, 0x3d, 0x7c, 0x1a, 0x77, 0x03};
    static const uint8_t b[6] = {0x31, 0x54, 0x49, 0x43, 0x66, 0x1a};
    const uint32_t n = (uint32_t)sizeof(sound_t) + 4u;
    const uint32_t before = writes;
    sound_t restored;
    uint32_t first;
    /* Both objects have the actual serialized sound length and format prefix. */
    memset(io_buf, 0, n);
    put32(io_buf, PROJ_FORMAT);
    memcpy(io_buf + 4, a, sizeof a);
    first = legacy_fnv32(io_buf, n);
    stored_known[OBJ_SOUND] = 0;
    if (save_buf(OBJ_SOUND, n) || writes != before + 1u) {
        puts("sound collision fixture first write failed");
        return 1;
    }
    memcpy(io_buf + 4, b, sizeof b);
    if (first != legacy_fnv32(io_buf, n) || !memcmp(a, b, sizeof a)) {
        puts("sound collision fixture does not collide in legacy FNV32");
        return 1;
    }
    if (save_buf(OBJ_SOUND, n) || writes != before + 2u ||
        flash_len[OBJ_SOUND] != n ||
        memcmp(flash_obj[OBJ_SOUND] + 4, b, sizeof b)) {
        puts("distinct sound payload discarded by cached hash collision");
        return 1;
    }
    if (save_buf(OBJ_SOUND, n) || writes != before + 2u) {
        puts("byte-identical sound payload needlessly rewritten");
        return 1;
    }
    if (load_sound(&restored) || memcmp(&restored, b, sizeof b)) {
        puts("saved collision variant failed sound-object readback");
        return 1;
    }
    puts("sound collision: different bytes persisted and read back; identical save skipped");
    return 0;
}

/* Exact-size settings are required; truncated objects cannot partly replace dst. */
static int test_settings_guards(void)
{
    settings_t loaded, before;
    project_defaults();
    if (project_validate_for(&proj)) {
        puts("settings fixture is invalid before ck_x2 mutation");
        return 1;
    }
    proj.set.ck_x2 = 0xFF;
    if (project_validate_for(&proj) != -1) {
        puts("invalid ck_x2 accepted");
        return 1;
    }
    project_defaults();
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set)) return 1;
    memset(&loaded, 0xA5, sizeof loaded);
    before = loaded;
    --flash_len[OBJ_SET];
    if (load_settings(&loaded) != -9 || memcmp(&loaded, &before, sizeof loaded)) {
        puts("short settings object accepted or partially copied");
        return 1;
    }
    ++flash_len[OBJ_SET];
    if (load_settings(&loaded) || memcmp(&loaded, &proj.set, sizeof loaded)) {
        puts("exact-size settings object did not load");
        return 1;
    }
    puts("settings guards: short object -9, exact size loads, ck_x2=255 rejected");
    return 0;
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
    /* Keep the current stream schema intact; arbitrary flags/steps now fail
     * revision-13 layout validation before reaching the capacity preflight. */
    for (int pat = 0; pat < NPAT; ++pat) {
        proj.pat[pat].brk.slice_mode = 0;
        memset(proj.pat[pat].brk.slice, 0, sizeof proj.pat[pat].brk.slice);
        for (int bass = 0; bass < NBASS; ++bass)
            for (int step = 0; step < NSTEPS; ++step)
                proj.pat[pat].bass[bass].step[step].flags = G_NOTE;
    }
    for (int lane = 0; lane < NLANE; ++lane)
        proj.arr.lane[lane].used = 0;
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
    if (test_settings_guards()) return 1;
    return test_sound_hash_collision();
}
