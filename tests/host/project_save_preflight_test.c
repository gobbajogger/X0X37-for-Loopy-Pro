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

static uint32_t random_state = 0x1C0369E7u;
static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}
static void fill_capacity(project_t *p)
{
    unsigned k, b, v, i;
    for (k = 0; k < NPAT; k++) {
        for (b = 0; b < NKIT; b++) {
            for (v = 0; v < NDRUM; v++)
                for (i = 0; i < NSW; i++)
                    p->pat[k].drum[b].hit[v][i] = next_random();
            for (i = 0; i < NSW; i++)
                p->pat[k].drum[b].accent[i] = next_random();
        }
        for (b = 0; b < NBASS; b++)
            for (i = 0; i < NSTEPS; i++) {
                p->pat[k].bass[b].step[i].note = (uint8_t)(next_random() % 128u);
                p->pat[k].bass[b].step[i].velocity = (uint8_t)(next_random() % 128u);
                p->pat[k].bass[b].step[i].flags = (uint8_t)((next_random() % 3u) | ((next_random() & 3u) << 2));
            }
    }
    for (k = 0; k < NLANE; k++) {
        lane_t *l = &p->arr.lane[k];
        l->used = 1;
        l->pat = (uint8_t)(k % NPAT);
        l->part = (uint8_t)(k / NPAT);
        l->t = T_MIX;
        l->v = l->part;
        l->i = MX_LEVEL;
        for (i = 0; i < NSTEPS; i++)
            l->val[i] = (uint8_t)(next_random() % 128u);
    }
}
int main(void)
{
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

    /* Unsupported data admission is a separate prewrite contract from OOM. */
    uint8_t prior_flags = proj.pat[0].bass[0].step[0].flags;
    proj.pat[0].bass[0].step[0].flags = G_REST | (1u << BS_END_SHIFT);
    writes = 0;
    if (project_save() != -9 || writes) {
        puts("unsupported layout was not blocked before all object writes");
        return 1;
    }
    proj.pat[0].bass[0].step[0].flags = prior_flags;
    puts("unsupported layout preflight: rejected before all writes");

    /* Use the maintained staging fixture's valid musical capacity data, not
     * arbitrary raw bytes that hit unsupported-layout rejection first. */
    fill_capacity(&proj);
    if (!stream_layout_ok(&proj, PROJ_STREAM)) {
        puts("capacity fixture is not a supported stream layout");
        return 1;
    }
    proj.set.palette = 2;
    proj.sound.v[T_303][0][0] = 99;
    seq.bpm = 140.0f;
    writes = 0;
    int save_rc = project_save();
    if (save_rc != -7 || writes) {
        printf("memory-full save returned %d, changed %u objects\n", save_rc, (unsigned)writes);
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
