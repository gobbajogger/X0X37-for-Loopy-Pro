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

/* Overwrite the packed stream with a deterministic incompressible payload, then
 * repair only what stream_layout_ok() inspects: bass gate nibbles and used-lane
 * types 0xfe/0xfd. A raw xorshift fill is rejected as -9 (layout) before the
 * capacity measure; the repaired fixture must still exceed STREAM_CAP. */
static void fill_incompressible_raw(void)
{
    uint8_t *raw = (uint8_t *)&proj + RAW_OFF;
    uint32_t rng = 1;
    int i;

    for (i = 0; i < (int)RAW_LEN; i++) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        raw[i] = (uint8_t)rng;
    }
}

static void make_raw_layout_valid(void)
{
    int p, b, k;

    for (p = 0; p < NPAT; p++)
        for (b = 0; b < NBASS; b++)
            for (k = 0; k < NSTEPS; k++) {
                uint8_t f = proj.pat[p].bass[b].step[k].flags;
                /* Force a non-rest gate so leftover end bits stay legal. */
                proj.pat[p].bass[b].step[k].flags = (uint8_t)((f & ~BS_GATE_MASK) | G_NOTE);
            }
    for (k = 0; k < NLANE; k++) {
        lane_t *l = &proj.arr.lane[k];
        if (!l->used)
            continue;
        if (l->t == 0xfeu || l->t == 0xfdu)
            l->t = 0;
    }
}

int main(void)
{
    uint32_t meas_len = 0;
    int rc, meas_rc;

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

    fill_incompressible_raw();
    make_raw_layout_valid();
    proj.set.palette = 2;
    proj.sound.v[T_303][0][0] = 99;
    seq.bpm = 140.0f;

    if (!stream_layout_ok(&proj, PROJ_STREAM)) {
        puts("memory-full fixture failed stream_layout_ok");
        return 1;
    }
    meas_rc = measure_project(&proj, &meas_len);
    if (meas_rc != -7) {
        printf("memory-full fixture measure_project rc=%d len=%u (expected -7)\n",
               meas_rc, (unsigned)meas_len);
        return 1;
    }

    writes = 0;
    rc = project_save();
    if (rc != -7 || writes) {
        printf("memory-full save rc=%d writes=%u (expected -7 and 0)\n",
               rc, (unsigned)writes);
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
    return test_sound_hash_collision();
}
