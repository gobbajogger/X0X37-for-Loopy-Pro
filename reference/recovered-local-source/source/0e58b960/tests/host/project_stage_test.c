/* SPDX-License-Identifier: GPL-3.0-only */
/* Project staging against real settings, descriptors, compression and A/B NOR
 * storage. Audio/UI ownership is covered by the runtime tests; no DSP or
 * project defaults are reimplemented here. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/app/plat.h"

static uint8_t nor[0x100000], protected_flash[sizeof nor], before_flash[sizeof nor];
static unsigned checks, failures, erases, programs, saves;
static const char *test_name;

static void expect(int ok, const char *what, int line)
{
    checks++;
    if (!ok) {
        fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, line, test_name, what);
        failures++;
    }
}
#define CHECK(x) expect(!!(x), #x, __LINE__)
#define REQUIRE(x) do { int required_ok = !!(x); expect(required_ok, #x, __LINE__); if (!required_ok) exit(2); } while (0)

/* The six project objects are writable. The panel object, owner tag, BT
 * identity/calibration, all sample slots and every other byte are protected. */
static int project_range(uint32_t off, uint32_t n)
{
    return (off >= 0x9D000u && off <= 0xA5000u && n <= 0xA5000u - off) ||
           (off >= 0xA7000u && off <= 0xAB000u && n <= 0xAB000u - off);
}
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    if (off > sizeof nor || n > sizeof nor - off)
        return -1;
    memcpy(dst, nor + off, n);
    return 0;
}
static int st_erase(uint32_t off)
{
    REQUIRE(!(off & 4095u) && project_range(off, 4096u));
    erases++;
    memset(nor + off, 0xFF, 4096u);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *p = src;
    uint32_t i;
    REQUIRE((off & 255u) + n <= 256u && project_range(off, n));
    programs++;
    for (i = 0; i < n; i++)
        nor[off + i] &= p[i];
    return 0;
}
#include "../../firmware/src/storage.c"
#define SMP_USER_XIP(k) (nor + SMP_USER_OFF(k))
#include "../../firmware/src/app/slots.c"
/* Device alias: project objects are built in the store buffer (x0x.c). */
#define PROJECT_IO_BUFFER st_buf
#include "../../firmware/src/app/project.c"

int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    REQUIRE(obj < OBJ_NOBJ && obj != OBJ_PANEL);
    return st_load(obj, dst, max);
}
int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    REQUIRE(obj < OBJ_NOBJ && obj != OBJ_PANEL);
    saves++;
    return st_save(obj, src, len);
}
void plat_midi_out(uint32_t pkt)
{
    (void)pkt;
    REQUIRE(0);                         /* project storage must not send MIDI */
}

static project_t before_project, saved_project;
static seq_t before_seq;
static uint32_t before_length;
static unsigned before_erases, before_programs, before_saves;

static void snapshot(void)
{
    memcpy(&before_project, &proj, sizeof proj);
    memcpy(&before_seq, &seq, sizeof seq);
    memcpy(before_flash, nor, sizeof nor);
    before_length = packed_len;
    before_erases = erases;
    before_programs = programs;
    before_saves = saves;
}
static void check_protected_flash(void)
{
    unsigned k;
    CHECK(!memcmp(nor, protected_flash, 0x9D000u));
    CHECK(!memcmp(nor + 0xA5000u, protected_flash + 0xA5000u, 0x2000u));
    CHECK(!memcmp(nor + 0xAB000u, protected_flash + 0xAB000u, sizeof nor - 0xAB000u));
    for (k = 0; k < SMP_USER_SLOTS; k++) {
        smp_user_scan(k);
        CHECK(usr_nz[k] == k + 2u);
        CHECK(!memcmp(smp_user_xip(k), protected_flash + SMP_USER_OFF(k), SMP_USER_SIZE(k)));
    }
}
static void check_no_writes(void)
{
    CHECK(erases == before_erases && programs == before_programs && saves == before_saves);
    CHECK(!memcmp(nor, before_flash, sizeof nor));
    check_protected_flash();
}
static void check_live_unchanged(void)
{
    CHECK(!memcmp(&proj, &before_project, sizeof proj));
    CHECK(!memcmp(&seq, &before_seq, sizeof seq));
    CHECK(packed_len == before_length);
    CHECK(project_mem_pct() == (int)((before_length * 100u + STREAM_CAP - 1u) / STREAM_CAP));
    check_no_writes();
}
static void set_runtime(void)
{
    static const uint8_t ch[NPARTS] = {9, 10, 1, 2, 3};
    unsigned p;
    memset(&seq, 0, sizeof seq);
    for (p = 0; p < NPARTS; p++)
        seq.ch[p] = ch[p];
    seq.bpm = 125.0f;
    seq.accent_q7 = 88;
    seq.send_clock = 1;
    seq.clk_src = SEQ_CLK_AUTO;
}
static void fixture(const char *name)
{
    uint32_t i, k;
    test_name = name;
    memset(nor, 0xFF, sizeof nor);
    /* Distinct retained data, including bytes beyond each sample's used data. */
    for (i = 0; i < sizeof nor; i++)
        if (!project_range(i, 1))
            nor[i] = (uint8_t)(i * 37u + (i >> 8) * 13u + 11u);
    for (k = 0; k < SMP_USER_SLOTS; k++) {
        smp_user_hdr_t h;
        memset(&h, 0, sizeof h);
        h.magic = SMP_USER_MAGIC;
        h.version = 1;
        h.nz = (uint8_t)(k + 2u);
        memcpy(h.name, "KEEP001", 8);
        h.name[6] = (char)('1' + k);
        h.data_len = 8192u;
        h.crc = st_crc32(nor + SMP_USER_OFF(k) + SMP_USER_DATA, h.data_len);
        for (i = 0; i < h.nz; i++) {
            h.zone[i].off = i * 1024u;
            h.zone[i].n = 2048u;
            h.zone[i].le = 2048u;
            h.zone[i].rate = (k + 1u) * 32768u;
            h.zone[i].root16 = (int16_t)(48u * 16u + k);
            h.zone[i].pred = (int16_t)(i * 113u);
            h.zone[i].idx = (uint8_t)(i + 3u);
            h.zone[i].hi = 127u;
            h.zone[i].looped = 1;
        }
        memcpy(nor + SMP_USER_OFF(k), &h, sizeof h);
    }
    memcpy(protected_flash, nor, sizeof nor);
    erases = programs = saves = 0;
    memset(stored_known, 0, sizeof stored_known);
    memset(stored_hash, 0, sizeof stored_hash);
    memset(&proj, 0, sizeof proj);
    proj.set.bt = PROJ_DEVICE_BT | PROJ_BT_CLK_IN;
    project_defaults();
    set_runtime();
    memset(&project_stage, 0xA5, sizeof project_stage);
    packed_len = 1234u;
    check_protected_flash();
}

static void test_defaults(void)
{
    sound_t factory_sound;
    unsigned p, b, s, k;
    static const uint8_t clip[12] = {2, 0, 0, 0, 0, 0, 9, 0, 4, 0, 0, 0};
    static const uint8_t break_defaults[BRK_NSET] = {25, 60, 20, 50, 10, 0, 5, 0, 2, 0, 2, 2};
    fixture("complete reset defaults");
    memset(&proj, 0xA5, sizeof proj);
    proj.set.bt = PROJ_DEVICE_BT | PROJ_BT_CLK_IN | (SEQ_CLK_EXT << 1);
    snapshot();
    project_reset_stage();
    check_live_unchanged();
    CHECK(project_validate_stage() == 0);
    CHECK(project_stage.set.magic == PROJ_MAGIC && project_stage.set.format == PROJ_FORMAT);
    CHECK(project_stage.set.palette == 1 && project_stage.set.keyled == 1);
    CHECK(project_stage.set.bpm_x10 == 1250 && project_stage.set.accent_q7 == 88);
    CHECK(project_stage.set.clk_out == 1 && !project_stage.set.notes_out);
    CHECK(project_stage.set.bt == (PROJ_DEVICE_BT | PROJ_BT_CLK_IN));
    CHECK(project_stage.set.stereo == PROJ_STEREO && project_stage.set.snd_rev == PROJ_SND_REV);
    CHECK(!project_stage.set.keysound && !project_stage.set.autosave_off &&
          !project_stage.set.out_loud && !project_stage.set.usb_full && !project_stage.set.rsv_bright);
    CHECK(!memcmp(&project_stage.set.ck_ch, clip, sizeof clip));
    for (p = 0; p < NPARTS; p++)
        CHECK(!project_stage.set.midi_ch[p]);
    engine_sound_defaults(&factory_sound);
    CHECK(!memcmp(&project_stage.sound, &factory_sound, sizeof factory_sound));
    for (p = 0; p < NPAT; p++) {
        const pattern_t *pat = &project_stage.pat[p];
        CHECK(!pat->swing);
        for (b = 0; b < NKIT; b++) {
            CHECK(pat->drum[b].len == 16 && pat->drum[b].rate == RATE_16);
            for (k = 0; k < NDRUM; k++)
                CHECK(!sm_any(pat->drum[b].hit[k]));
            CHECK(!sm_any(pat->drum[b].accent));
        }
        for (b = 0; b < NBASS; b++) {
            const bpart_t *bp = &pat->bass[b];
            CHECK(bp->len == 16 && bp->rate == RATE_16 && bp->dir == DIR_FWD && bp->transpose == 24);
            CHECK(bp->gen.density == 70 && bp->gen.accent == 40 && bp->gen.slide == 25);
            CHECK(bp->gen.oct_range == 2 && bp->gen.root == 9 && bp->gen.scale == 0);
            CHECK(bp->gen.base_oct == b + 1u && !bp->gen.mutate_bars);
            CHECK(bp->gen.seed == (b ? 0x5A1u + p * 613u : 0x3B0u + p * 977u));
            for (s = 0; s < NSTEPS; s++) {
                CHECK(bp->step[s].note == (b ? 45 : 33));
                CHECK(bp->step[s].flags == G_REST && !bp->step[s].velocity);
            }
        }
        CHECK(!pat->brk.steps && !pat->brk.slot_a && pat->brk.slot_b == 1);
        for (k = 0; k < BRK_NSET; k++)
            CHECK(pat->brk.set[k] == break_defaults[k]);
        for (s = 0; s < 16; s++)
            CHECK(!pat->brk.slice[s]);
    }
    for (p = 0; p < sizeof project_stage.arr; p++)
        CHECK(!((const uint8_t *)&project_stage.arr)[p]);
    project_commit_stage();
    CHECK(proj.set.bt == (PROJ_DEVICE_BT | PROJ_BT_CLK_IN));
    CHECK(!memcmp(&proj, &project_stage, sizeof proj));
    CHECK(!memcmp(&seq, &before_seq, sizeof seq));
    CHECK(project_mem_pct() > 0 && project_mem_pct() < 100);
    check_no_writes();
}

static void musical_fixture(void)
{
    unsigned p;
    proj.set.palette = 4;
    proj.set.ck_ch = 15;
    proj.sound.v[T_303][1][0] = 1;
    for (p = 0; p < NPAT; p++) {
        proj.pat[p].drum[1].len = NSTEPS;
        sm_set(proj.pat[p].drum[1].hit[10], (int)((p * 7u) % NSTEPS));
        proj.pat[p].bass[1].step[63].note = (uint8_t)(40u + p);
        proj.pat[p].bass[1].step[63].flags = G_NOTE | BS_ACCENT | BS_SLIDE;
        proj.pat[p].bass[1].step[63].velocity = (uint8_t)(96u + p);
        proj.pat[p].brk.slot_a = 2;
        proj.pat[p].brk.slot_b = 7;
        proj.pat[p].brk.slice[15] = 8;
    }
    proj.arr.song.len = NSONG;
    proj.arr.song.bar[NSONG - 1].pat[PART_303B] = NPAT - 1;
    proj.arr.song.bar[NSONG - 1].mute = 0xFF;
    proj.arr.lane[NLANE - 1].used = 1;
    proj.arr.lane[NLANE - 1].pat = NPAT - 1;
    proj.arr.lane[NLANE - 1].part = PART_303B;
    proj.arr.lane[NLANE - 1].t = T_303;
    proj.arr.lane[NLANE - 1].v = 1;
    proj.arr.lane[NLANE - 1].i = 0;
    memset(proj.arr.lane[NLANE - 1].val, MOT_NONE, NSTEPS);
    proj.arr.lane[NLANE - 1].val[NSTEPS - 1] = engine_param(T_303, 1, 0)->max;
}

static void test_save_export_load(void)
{
    uint8_t h[PROJECT_FILE_HDR];
    unsigned n;
    fixture("save/export/load preserve samples and live data");
    musical_fixture();
    seq.bpm = 173.2f;
    seq.accent_q7 = 127;
    seq.send_clock = 0;
    seq.send_notes = 1;
    proj.set.bt = (uint8_t)((proj.set.bt & ~PROJ_BT_CLK_MASK) | (SEQ_CLK_EXT << 1));
    seq.clk_src = SEQ_CLK_AUTO;        /* clip mode's temporary override must not replace the stored preference */
    seq.ch[0] = SEQ_CH_OFF;
    seq.ch[1] = 15;
    snapshot();
    project_capture_stage();
    project_file_header(h);
    CHECK(project_file_validate(h) == 0);
    check_live_unchanged();
    CHECK(project_stage.set.bpm_x10 == 1732 && project_stage.set.accent_q7 == 127);
    CHECK(project_stage.set.midi_ch[0] == 17 && project_stage.set.midi_ch[1] == 16);
    CHECK(!project_stage.set.clk_out && project_stage.set.notes_out);
    CHECK(PROJ_CLK_SRC(project_stage.set.bt) == SEQ_CLK_EXT);
    CHECK(!memcmp(&project_stage.sound, &proj.sound, sizeof proj - offsetof(project_t, sound)));
    CHECK(project_save() == 0);
    CHECK(!memcmp(&seq, &before_seq, sizeof seq));
    CHECK(!memcmp(&proj.sound, &before_project.sound, sizeof proj - offsetof(project_t, sound)));
    CHECK(!memcmp(&proj, &project_stage, sizeof proj));
    check_protected_flash();
    memcpy(&saved_project, &proj, sizeof proj);
    n = saves;
    snapshot();
    CHECK(project_save() == 0);
    CHECK(saves == n);
    check_live_unchanged();

    /* Staging a stored project is read-only even when the live project has
     * diverged. Applying it preserves this keyboard's Bluetooth role bits. */
    proj.set.palette = 0;
    proj.set.bt &= (uint8_t)~PROJ_DEVICE_BT;
    proj.pat[0].bass[0].step[0].note = 111;
    seq.bpm = 99.9f;
    packed_len = 2031u;
    snapshot();
    CHECK(project_load_stage() == 0);
    CHECK(!memcmp(&project_stage, &saved_project, sizeof saved_project));
    check_live_unchanged();
    project_commit_stage();
    saved_project.set.bt &= (uint8_t)~PROJ_DEVICE_BT;
    CHECK(!memcmp(&proj, &saved_project, sizeof proj));
    CHECK(!memcmp(&seq, &before_seq, sizeof seq));
    check_no_writes();
}

static void test_file_rejection(void)
{
    uint8_t h[PROJECT_FILE_HDR], wrong[PROJECT_FILE_HDR];
    static const unsigned words[] = {0, 4, 8, 16, 20, 24, 28};
    unsigned k;
    fixture("file identity, length, revision and CRC rejection");
    musical_fixture();
    project_capture_stage();
    project_file_header(h);
    snapshot();
    CHECK(get32(h) == 0x50583058u && get32(h + 4) == 1u);
    CHECK(get32(h + 8) + PROJECT_FILE_HDR == PROJECT_FILE_SIZE);
    CHECK(project_file_validate(h) == 0);
    for (k = 0; k < sizeof words / sizeof words[0]; k++) {
        memcpy(wrong, h, sizeof wrong);
        wrong[words[k]] ^= 1;
        CHECK(project_file_validate(wrong) == -1);
        check_live_unchanged();
    }
    memcpy(wrong, h, sizeof wrong);
    wrong[12] ^= 1;
    CHECK(project_file_validate(wrong) == -4);
    project_stage.pat[NPAT - 1].bass[1].step[NSTEPS - 1].velocity ^= 1;
    CHECK(project_file_validate(h) == -4);
    check_live_unchanged();
}

typedef struct { size_t offset; unsigned bytes; uint32_t value; } invalid_t;
#define BAD8(field, value) {offsetof(project_t, field), 1, value}
#define BAD16(field, value) {offsetof(project_t, field), 2, value}
static void test_semantic_rejection(void)
{
    static const invalid_t invalid[] = {
        BAD8(set.palette, 5), BAD8(set.keyled, 3), BAD8(set.clk_out, 2), BAD8(set.notes_out, 2),
        BAD16(set.bpm_x10, 199), BAD16(set.bpm_x10, 2751), BAD8(set.accent_q7, 128),
        BAD8(set.keysound, 2), BAD8(set.autosave_off, 2), BAD8(set.out_loud, 2), BAD8(set.usb_full, 2),
        BAD8(set.bt, 0), BAD8(set.bt, 0xC8), BAD8(set.bt, 14), BAD8(set.ck_ch, 1),
        BAD8(set.ck_root, 12), BAD8(set.ck_rate, 13), BAD8(set.midi_ch[4], 18),
        BAD8(pat[31].swing, 101), BAD8(pat[31].drum[1].len, 0), BAD8(pat[31].drum[1].len, 65),
        BAD8(pat[31].drum[1].rate, NRATES), BAD8(pat[31].bass[1].len, 0),
        BAD8(pat[31].bass[1].dir, NDIRS), BAD8(pat[31].bass[1].transpose, 49),
        BAD8(pat[31].bass[1].gen.density, 101), BAD8(pat[31].bass[1].gen.oct_range, 0),
        BAD8(pat[31].bass[1].gen.scale, TB3PO_NSCALES), BAD8(pat[31].bass[1].step[63].flags, 3),
        BAD8(pat[31].bass[1].step[63].flags, 16), BAD8(pat[31].bass[1].step[63].note, 128),
        BAD8(pat[31].bass[1].step[63].velocity, 128), BAD8(pat[31].brk.set[BRK_PHRASE], 255),
        BAD8(pat[31].brk.slice[15], 33), BAD8(pat[31].brk.slice_mode, 16), BAD16(arr.song.len, NSONG + 1),
        BAD8(arr.song.bar[NSONG - 1].pat[PART_BRK], NPAT),
        BAD8(arr.lane[NLANE - 1].used, 2), BAD8(arr.lane[NLANE - 1].pat, NPAT),
        BAD8(arr.lane[NLANE - 1].part, NPARTS), BAD8(arr.lane[NLANE - 1].t, T_VEL + 1),
        BAD8(arr.lane[NLANE - 1].v, NVOICES_MAX), BAD8(arr.lane[NLANE - 1].i, NPARAMS_MAX),
        BAD8(arr.lane[NLANE - 1].val[NSTEPS - 1], 254), BAD8(sound.v[T_303][1][0], 254),
        BAD8(sound.v[T_FX][1][0], 128),
    };
    uint8_t h[PROJECT_FILE_HDR];
    unsigned k, b;
    fixture("CRC-valid unsafe values rejected before commit");
    musical_fixture();
    project_capture_stage();
    memcpy(&saved_project, &project_stage, sizeof saved_project);
    snapshot();
    CHECK(project_validate_stage() == 0);
    for (k = 0; k < sizeof invalid / sizeof invalid[0]; k++) {
        memcpy(&project_stage, &saved_project, sizeof project_stage);
        for (b = 0; b < invalid[k].bytes; b++)
            ((uint8_t *)&project_stage)[invalid[k].offset + b] = (uint8_t)(invalid[k].value >> (b * 8u));
        project_file_header(h);       /* correct CRC: this must be semantic rejection */
        int invalid_rc = project_file_validate(h);
        if (invalid_rc != -1 && invalid_rc != -9) {
            fprintf(stderr, "unsafe case %u (offset %zu) was accepted\n", k, invalid[k].offset);
            CHECK(0);
        }
        check_live_unchanged();
    }
    memcpy(&project_stage, &saved_project, sizeof project_stage);
    project_stage.arr.lane[0].used = 1;
    project_stage.arr.lane[0].part = PART_909;
    project_stage.arr.lane[0].t = T_PROB;
    project_stage.arr.lane[0].v = NDRUM - 1;
    memset(project_stage.arr.lane[0].val, MOT_NONE, NSTEPS);
    for (k = 0; k < PROB_NCOND; k++)
        project_stage.arr.lane[0].val[k] = (uint8_t)(PROB_COND + k);
    CHECK(project_validate_stage() == 0); /* all 1:2 through 8:8 conditions are valid */
    project_stage.arr.lane[0].val[63] = PROB_COND + PROB_NCOND;
    CHECK(project_validate_stage() == -1);
    project_stage.arr.lane[0].val[63] = MOT_NONE;
    project_stage.arr.lane[0].v = NDRUM;
    CHECK(project_validate_stage() == -1);
    /* T_VEL: drum strike velocity lanes (1..127); not for 303 parts. */
    memcpy(&project_stage, &saved_project, sizeof project_stage);
    project_stage.arr.lane[0].used = 1;
    project_stage.arr.lane[0].part = PART_909;
    project_stage.arr.lane[0].t = T_VEL;
    project_stage.arr.lane[0].v = (uint8_t)(PART_909 << 4 | (NDRUM - 1));
    project_stage.arr.lane[0].i = 0;
    memset(project_stage.arr.lane[0].val, MOT_NONE, NSTEPS);
    project_stage.arr.lane[0].val[0] = 1;
    project_stage.arr.lane[0].val[1] = 127;
    CHECK(project_validate_stage() == 0);
    project_stage.arr.lane[0].val[2] = 128;
    CHECK(project_validate_stage() == -1);
    project_stage.arr.lane[0].val[2] = MOT_NONE;
    project_stage.arr.lane[0].part = PART_303A;
    project_stage.arr.lane[0].v = (uint8_t)(PART_303A << 4);
    CHECK(project_validate_stage() == -1);
    check_live_unchanged();
}

static void test_storage_rejection(void)
{
    static const uint32_t objects[] = {OBJ_SET, OBJ_SOUND, OBJ_PAT0};
    uint8_t payload[PLAT_STORE_MAX];
    unsigned k;
    for (k = 0; k < sizeof objects / sizeof objects[0]; k++) {
        int n;
        fixture("truncated store leaves live project/settings/gauge intact");
        musical_fixture();
        REQUIRE(project_save() == 0);
        n = st_load(objects[k], payload, sizeof payload);
        REQUIRE(n > 16);
        /* A valid NOR CRC around a truncated project payload. Settings older
         * than 20 bytes are malformed; sound/stream lose their final byte. */
        REQUIRE(st_save(objects[k], payload, objects[k] == OBJ_SET ? 19u : (uint32_t)n - 1u) == 0);
        proj.set.palette = 0;
        proj.pat[NPAT - 1].bass[1].step[63].note = 19;
        seq.bpm = 231.0f;
        packed_len = 4321u;
        snapshot();
        CHECK(project_load_stage() < 0);
        check_live_unchanged();
    }
    fixture("corrupt stream payload leaves live project and samples intact");
    musical_fixture();
    REQUIRE(project_save() == 0);
    {
        int n = st_load(OBJ_PAT0, payload, sizeof payload);
        REQUIRE(n > (int)STREAM_HDR);
        payload[STREAM_HDR] ^= 0x40;
        REQUIRE(st_save(OBJ_PAT0, payload, (uint32_t)n) == 0);
    }
    snapshot();
    CHECK(project_load_stage() < 0);
    check_live_unchanged();
    fixture("empty storage leaves live project intact");
    musical_fixture();
    snapshot();
    CHECK(project_load_stage() < 0);
    check_live_unchanged();
}

/* Encode the actual pre-stream format, with valid current sound/settings so
 * these cases isolate legacy arrangement completeness rather than migrations. */
static void store_legacy_project(int arrangement)
{
    uint8_t payload[PLAT_STORE_MAX], raw[ARR1_LEN];
    pattern1_t old;
    unsigned h, p, k, v, i;
    REQUIRE(save_obj(OBJ_SET, &proj.set, sizeof proj.set) == 0);
    REQUIRE(save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound) == 0);
    for (h = 0; h < 2; h++) {
        put32(payload, PROJ_FORMAT);
        for (p = 0; p < 8; p++) {
            const pattern_t *pat = &proj.pat[h * 8 + p];
            memset(&old, 0, sizeof old);
            for (k = 0; k < NKIT; k++) {
                for (v = 0; v < NDRUM; v++)
                    old.drum[k].hit[v] = pat->drum[k].hit[v][0];
                old.drum[k].accent = pat->drum[k].accent[0];
                old.drum[k].len = pat->drum[k].len;
                old.drum[k].rate = pat->drum[k].rate;
            }
            for (k = 0; k < NBASS; k++) {
                for (i = 0; i < NSTEPS1; i++) {
                    old.bass[k].step[i].note = pat->bass[k].step[i].note;
                    old.bass[k].step[i].flags = pat->bass[k].step[i].flags;
                }
                old.bass[k].len = pat->bass[k].len;
                old.bass[k].rate = pat->bass[k].rate;
                old.bass[k].dir = pat->bass[k].dir;
                old.bass[k].transpose = pat->bass[k].transpose;
                old.bass[k].gen = pat->bass[k].gen;
            }
            old.brk.steps = pat->brk.steps;
            memcpy(old.brk.set, pat->brk.set, sizeof old.brk.set);
            old.brk.slot_a = pat->brk.slot_a;
            old.brk.slot_b = pat->brk.slot_b;
            old.swing = pat->swing;
            memcpy(payload + 4 + p * sizeof old, &old, sizeof old);
        }
        REQUIRE(st_save(h ? OBJ_PAT1 : OBJ_PAT0, payload, 4u + 8u * sizeof old) == 0);
    }
    if (!arrangement)
        return;
    memcpy(raw, &proj.arr.song, sizeof(song_t));
    for (k = 0; k < NLANE; k++) {
        lane1_t old_lane;
        const lane_t *l = &proj.arr.lane[k];
        memset(&old_lane, 0, sizeof old_lane);
        old_lane.used = l->used;
        old_lane.gen = l->gen;
        old_lane.pat = l->pat;
        old_lane.part = l->part;
        old_lane.t = l->t;
        old_lane.v = l->v;
        old_lane.i = l->i;
        memcpy(old_lane.val, l->val, NSTEPS1);
        memcpy(raw + sizeof(song_t) + k * sizeof old_lane, &old_lane, sizeof old_lane);
    }
    REQUIRE(save_obj(OBJ_SONG, raw, ARR1_A) == 0);
    REQUIRE(save_obj(OBJ_MOTION, raw + ARR1_A, ARR1_LEN - ARR1_A) == 0);
}

static void test_legacy_arrangement(void)
{
    static const uint32_t objects[] = {OBJ_SONG, OBJ_MOTION};
    uint8_t payload[PLAT_STORE_MAX];
    unsigned k, fault;
    fixture("legacy arrangement widens without changing live data or samples");
    proj.arr.song.len = 1;
    proj.arr.song.bar[0].pat[PART_303B] = 15;
    proj.arr.lane[NLANE - 1].used = 1;
    proj.arr.lane[NLANE - 1].pat = 15;
    proj.arr.lane[NLANE - 1].part = PART_303B;
    proj.arr.lane[NLANE - 1].t = T_303;
    proj.arr.lane[NLANE - 1].v = 1;
    proj.arr.lane[NLANE - 1].i = 0;
    memset(proj.arr.lane[NLANE - 1].val, MOT_NONE, NSTEPS);
    proj.arr.lane[NLANE - 1].val[NSTEPS1 - 1] = 53;
    store_legacy_project(1);
    snapshot();
    CHECK(project_load_stage() == 0);
    CHECK(project_stage.arr.song.len == 1);
    CHECK(project_stage.arr.song.bar[0].pat[PART_303B] == 15);
    CHECK(project_stage.arr.lane[NLANE - 1].val[NSTEPS1 - 1] == 53);
    for (k = NSTEPS1; k < NSTEPS; k++)
        CHECK(project_stage.arr.lane[NLANE - 1].val[k] == MOT_NONE);
    check_live_unchanged();

    fixture("pre-arrangement projects retain compatibility when both objects unavailable");
    store_legacy_project(0);
    snapshot();
    CHECK(project_load_stage() == 0);
    CHECK(project_stage.arr.song.len == 0);
    for (k = 0; k < NLANE; k++)
        CHECK(!project_stage.arr.lane[k].used);
    check_live_unchanged();

    for (k = 0; k < sizeof objects / sizeof objects[0]; k++)
        for (fault = 0; fault < 3; fault++) {
            int n;
            fixture("incomplete or malformed legacy arrangement refuses staged load");
            store_legacy_project(1);
            n = st_load(objects[k], payload, sizeof payload);
            REQUIRE(n > 4);
            if (fault == 0) {
                /* Simulate one unavailable object; both A/B copies are absent. */
                REQUIRE(st_erase(st_sector(objects[k], 0)) == 0);
                REQUIRE(st_erase(st_sector(objects[k], 1)) == 0);
            } else {
                if (fault == 2)
                    put32(payload, PROJ_FORMAT + 1u);
                REQUIRE(st_save(objects[k], payload, (uint32_t)n - (fault == 1)) == 0);
            }
            musical_fixture();
            seq.bpm = 231.0f;
            packed_len = 4321u;
            snapshot();
            CHECK(project_load_stage() < 0);
            check_live_unchanged();
        }
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
static void test_capacity_rejection(void)
{
    uint8_t h[PROJECT_FILE_HDR];
    fixture("capacity rejected before any flash write");
    musical_fixture();
    REQUIRE(project_save() == 0);
    memcpy(&saved_project, &proj, sizeof saved_project);
    project_capture_stage();
    fill_capacity(&project_stage);
    snapshot();
    CHECK(project_validate_stage() == -7);
    project_file_header(h);
    CHECK(project_file_validate(h) == -7);
    check_live_unchanged();
    memcpy(&proj, &project_stage, sizeof proj);
    snapshot();
    CHECK(project_save() == -7);
    check_live_unchanged();
    /* The previously saved project is still readable after the rejected save. */
    memset(stored_known, 0, sizeof stored_known);
    CHECK(project_load_stage() == 0);
    CHECK(!memcmp(&project_stage, &saved_project, sizeof saved_project));
    check_live_unchanged();
}


/* BF-020: stored pre-BD-send pan motion must follow the sound pan's two-byte
 * migration. Stream arrangement loads after sound; both must be in memory
 * before the migration rewrites any motion-lane target indices. */
static void test_legacy_bd_pan_motion(void)
{
    static const struct {uint8_t rev, pan_909, pan_808;} cases[] = {
        {2, 8, 7}, /* no BD sends on either 909 or 808 */
        {3, 10, 7}, /* 909 already migrated; 808 not yet migrated */
        {4, 10, 9}, /* both already migrated: do not migrate twice */
    };
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        const uint8_t pan909 = cases[c].pan_909, pan808 = cases[c].pan_808;
        lane_t *lane;
        fixture("legacy BD PAN motion migration");
        proj.set.snd_rev = cases[c].rev;
        proj.set.stereo = PROJ_STEREO;
        proj.sound.v[T_909][0][pan909] = 31;
        proj.sound.v[T_808][0][pan808] = 87;
        lane = &proj.arr.lane[0];
        lane->used = 1; lane->pat = 3; lane->part = PART_909;
        lane->t = T_909; lane->v = 0; lane->i = pan909;
        memset(lane->val, MOT_NONE, NSTEPS); lane->val[2] = 31;
        lane = &proj.arr.lane[1];
        lane->used = 1; lane->pat = 4; lane->part = PART_808;
        lane->t = T_808; lane->v = 0; lane->i = pan808;
        memset(lane->val, MOT_NONE, NSTEPS); lane->val[4] = 87;
        lane = &proj.arr.lane[2];
        lane->used = 1; lane->pat = 3; lane->part = PART_909;
        lane->t = T_909; lane->v = 1; lane->i = 2; /* unaffected different voice */
        memset(lane->val, MOT_NONE, NSTEPS); lane->val[2] = 33;
        REQUIRE(project_save() == 0);
        snapshot();
        CHECK(project_load_stage() == 0);
        CHECK(project_stage.set.snd_rev == PROJ_SND_REV);
        CHECK(project_stage.arr.lane[0].i == 10);
        CHECK(project_stage.arr.lane[1].i == 9);
        CHECK(project_stage.arr.lane[2].i == 2);
        CHECK(project_stage.arr.lane[0].val[2] == 31);
        CHECK(project_stage.arr.lane[1].val[4] == 87);
        CHECK(project_stage.arr.lane[2].val[2] == 33);
        CHECK(project_stage.sound.v[T_909][0][10] == 31);
        CHECK(project_stage.sound.v[T_808][0][9] == 87);
        check_live_unchanged(); /* stage migration never mutates active playback or flash */
    }
}

/* A revision-1 project may contain a valid master high-pass cutoff above 120.
 * The compact FILTER macro only represents 0..120 on the HP side; migration
 * must never manufacture a value outside its 0..127 parameter range. */
static void test_legacy_master_hp_migration(void)
{
    static const unsigned cutoff[] = {0, 1, 7, 8, 32, 60, 119, 120, 121, 123, 124, 125, 126, 127};
    unsigned mode, k;
    for (mode = 0; mode < 4u; mode++) {
        for (k = 0; k < sizeof cutoff / sizeof cutoff[0]; k++) {
            unsigned c = cutoff[k], expected;
            if (mode == 0u)
                expected = 64u;
            else if (mode == 3u)
                expected = 69u + (c > 120u ? 120u : c) * 58u / 120u;
            else
                expected = c < 8u ? 0u : (c - 8u) * 59u / 119u;
            fixture("legacy master filter valid after sound-revision migration");
            proj.set.snd_rev = 1;       /* the one-knob FILTER did not exist */
            proj.sound.v[T_MST][0][MST_MODE] = (uint8_t)mode;
            proj.sound.v[T_MST][0][MST_CUTOFF] = (uint8_t)c;
            REQUIRE(project_save() == 0);
            snapshot();
            CHECK(project_load_stage() == 0);
            CHECK(project_stage.set.snd_rev == PROJ_SND_REV);
            CHECK(project_stage.sound.v[T_MST][0][MST_MODE] == mode);
            CHECK(project_stage.sound.v[T_MST][0][MST_CUTOFF] == c);
            CHECK(project_stage.sound.v[T_MST][0][MST_DJF] <= 127u);
            CHECK(project_stage.sound.v[T_MST][0][MST_DJF] == expected);
            check_live_unchanged();
        }
    }
}

/* Legacy SMC profile bytes are compatibility data, not an active mode. */
static void test_smc_daw_normalization(void)
{
    fixture("legacy SMC DAW normalization");
    REQUIRE(project_save()==0);
    for(unsigned role=0;role<2;role++)for(unsigned enable=0;enable<2;enable++)for(unsigned clock=0;clock<3;clock++) {
        settings_t legacy=proj.set;
        legacy.bt=(uint8_t)(PROJ_BT_CLK_IN|(clock<<1)|(role?PROJ_BT_SMC_CENTRAL:0)|enable|PROJ_BT_SMC_USER);
        legacy.autosave_off=1;
        REQUIRE(save_obj(OBJ_SET,&legacy,sizeof legacy)==0);
        unsigned writes=saves,erase=erases,program=programs;
        memcpy(before_flash,nor,sizeof nor);
        for(int boot=0;boot<2;boot++) {
            REQUIRE(project_load()==0);
            settings_t expected=legacy;expected.bt&=(uint8_t)~PROJ_BT_SMC_USER;
            CHECK(!memcmp(&proj.set,&expected,sizeof expected));
            CHECK(saves==writes&&erases==erase&&programs==program);
            CHECK(!memcmp(before_flash,nor,sizeof nor)); /* no implicit migration save */
        }
        /* A raw stale bit after load cannot leak through ordinary export. */
        proj.set.bt|=PROJ_BT_SMC_USER;project_capture_stage();
        CHECK(!(project_stage.set.bt&PROJ_BT_SMC_USER));CHECK(proj.set.bt&PROJ_BT_SMC_USER);
        /* Existing file CRC is checked before canonical adoption. */
        project_stage.set.bt|=PROJ_BT_SMC_USER;
        uint8_t header[PROJECT_FILE_HDR];project_file_header(header);
        REQUIRE(project_file_validate(header)==0);CHECK(project_stage.set.bt&PROJ_BT_SMC_USER);
        header[12]^=1;CHECK(project_file_validate(header)!=0);header[12]^=1;
        REQUIRE(project_file_validate(header)==0);
        before_seq=seq;project_commit_stage();
        CHECK(!(proj.set.bt&PROJ_BT_SMC_USER));CHECK(!memcmp(&seq,&before_seq,sizeof seq));
        CHECK((proj.set.bt&PROJ_DEVICE_BT)==(legacy.bt&PROJ_DEVICE_BT));
        CHECK(saves==writes&&erases==erase&&programs==program);
        REQUIRE(project_save()==0); /* an ordinary requested save may write canonical bytes */
        settings_t stored={0};REQUIRE(load_settings(&stored)==0);CHECK(!(stored.bt&PROJ_BT_SMC_USER));
        check_protected_flash();
    }
    proj.set.bt|=PROJ_BT_SMC_USER;project_defaults();CHECK(!(proj.set.bt&PROJ_BT_SMC_USER));
    proj.set.bt|=PROJ_BT_SMC_USER;project_reset_stage();CHECK(!(project_stage.set.bt&PROJ_BT_SMC_USER));
}

int main(void)
{
    test_smc_daw_normalization();
    test_defaults();
    test_legacy_bd_pan_motion();
    test_save_export_load();
    test_file_rejection();
    test_semantic_rejection();
    test_storage_rejection();
    test_legacy_master_hp_migration();
    test_capacity_rejection();
    test_legacy_arrangement();
    printf("project staging: %u checks, %u failures; real storage, defaults and sample metadata\n", checks, failures);
    return failures ? 1 : 0;
}
