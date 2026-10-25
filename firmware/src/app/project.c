/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X project: the settings, the sound, the 32 patterns, the song and the motion lanes.
 *
 * The settings and the sound are one storage object each, as they are. The patterns, the song and
 * the lanes (~30 KB, mostly empty steps) are one stream packed with lz.c across the four objects
 * OBJ_PAT0, OBJ_PAT1, OBJ_SONG, OBJ_MOTION (PROJ_STREAM). The first carries the raw length and a
 * CRC of the raw bytes, so a stream torn by a power cut between two objects, or one that does not
 * unpack, loads as the defaults rather than as garbage. A save packs once to measure (nothing is
 * written if it would not fit: "MEMORY FULL") and once to write; an object whose bytes did not
 * change is not written again.
 *
 * Every object carries its format number. A project saved before the stream (format 1: 16
 * patterns of 32 steps in OBJ_PAT0/1, the song and 32-step lanes in OBJ_SONG/OBJ_MOTION) is
 * converted as it loads: its steps land on steps 1-32, its patterns on 1-16. */
#include "x0x.h"
#include "../dsp/drum909.h"            /* DR_SD (the sound's revisions) */
#include "../dsp/master.h"             /* MST_* (the sound's revisions) */
#include "lz.c"

project_t proj;
/* Reuses the UI undo shadow while an exclusive project operation is active. */
project_t project_stage X0X_POOL;

#define PROJ_STREAM 4u                  /* sound snapshots use typed records in the unchanged lane pool */
#define PROJ_STREAM_3 3u                /* same raw layout, without snapshot records */
#define PROJ_STREAM_OLD 2u
static const uint8_t STREAM_OBJ[] = {OBJ_PAT0, OBJ_PAT1, OBJ_SONG, OBJ_MOTION};
#define NSTREAM ((int)sizeof STREAM_OBJ)
#define STREAM_HDR 16u                  /* in the first object: format, raw length, raw CRC, packed length */
#define STREAM_CAP (NSTREAM * (PLAT_STORE_MAX - 4u) - (STREAM_HDR - 4u))
#define RAW_OFF __builtin_offsetof(project_t, pat)
#define RAW_LEN (sizeof(project_t) - RAW_OFF)   /* the patterns and the arrangement, contiguous */
typedef char raw_fits[(RAW_LEN < 0xFFFFu) ? 1 : -1];
typedef char raw_contiguous[(__builtin_offsetof(project_t, arr) == RAW_OFF + sizeof proj.pat) ? 1 : -1];

static void arrange_defaults(project_t *pr);
static uint32_t packed_len;             /* the last save's (or load's) packed stream, bytes */

/* SMK-37 clip mode keybed settings (main_fm1.c), ck_ch .. ck_x2: channel 2, C MAJOR NEAR, MAJ root position,
 * ARP UP 1/16, 1 octave, gate 50 %, no swing, human, rhythm or strum; and the highest values. A value out of
 * range (for example an older object): all go back to the defaults. An older object without ck_x1/ck_x2 loads
 * zeros there: the defaults. */
static const uint8_t CK_DEF[12] = {2, 0, 0, 0, 0, 0, 9, 0, 4, 0, 0, 0},
                     CK_MAX[12] = {15, 11, 23, 1, 29, 5, 12, 3, 9, 3, 63, 127};
static void project_ck_check(project_t *pr, int force)
{
    uint8_t *v = &pr->set.ck_ch;
    for (int i = 0; i < 12; i++)
        force |= v[i] > CK_MAX[i];
    if (force || v[0] < 2)
        __builtin_memcpy(v, CK_DEF, 12);
}
typedef char ck_set_size[sizeof(settings_t) == 40u ? 1 : -1];   /* the new bytes use the old padding */

static void project_defaults_for(project_t *pr, uint8_t device_bt)
{
    int i;
    __builtin_memset(pr, 0, sizeof *pr);
    pr->set.magic = PROJ_MAGIC;
    pr->set.format = PROJ_FORMAT;
    pr->set.palette = 1;                /* amber */
    pr->set.keyled = 1;
    pr->set.keysound = 0;
    pr->set.autosave_off = 0;
    pr->set.clk_out = 1;
    pr->set.bt = (uint8_t)((device_bt & PROJ_DEVICE_BT) | PROJ_BT_CLK_IN);   /* CLOCK SOURCE AUTO */
    pr->set.notes_out = 0;
    pr->set.bpm_x10 = 1250;
    pr->set.accent_q7 = 88;
    pr->set.stereo = PROJ_STEREO;
    pr->set.snd_rev = PROJ_SND_REV;
    project_ck_check(pr, 1);
    engine_sound_defaults(&pr->sound);
    arrange_defaults(pr);
    /* the 303 lines start empty, like the drums: OCT+ on TB-3PO (or keyboard mode) writes one */
    for (i = 0; i < NPAT; i++)
        pattern_init(&pr->pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}

/* how full the stream's room is, in % (GLO > ABOUT) */
int project_mem_pct(void) { return (int)((packed_len * 100u + STREAM_CAP - 1u) / STREAM_CAP); }

/* object payload: format word + body */
static uint8_t io_buf[PLAT_STORE_MAX] __attribute__((aligned(4)));

/* what each object last held on the flash (a hash of its payload), so a save writes only the
 * objects that changed: an autosave usually erases one or two sectors, not six */
static uint32_t stored_hash[OBJ_NOBJ];
static uint8_t stored_known[OBJ_NOBJ];
static uint32_t hash_of(const uint8_t *p, uint32_t n)        /* FNV-1a */
{
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ *p++) * 16777619u;
    return h;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t crc32_of(const uint8_t *p, uint32_t n)       /* zlib CRC-32, 4 bits per step */
{
    static const uint32_t T[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        c = (c >> 4) ^ T[c & 15u];
        c = (c >> 4) ^ T[c & 15u];
    }
    return ~c;
}

/* the object is the n bytes of io_buf, format word included; written only if they changed */
static int save_buf(uint32_t obj, uint32_t n)
{
    uint32_t h = hash_of(io_buf, n);
    int rc;
    if (stored_known[obj] && stored_hash[obj] == h)
        return 0;                                    /* the flash already holds exactly this */
    rc = plat_store_save(obj, io_buf, n);
    stored_known[obj] = rc == 0;
    stored_hash[obj] = h;
    return rc;
}

static int load_obj(uint32_t obj, void *dst, uint32_t len)
{
    int n = plat_store_load(obj, io_buf, sizeof io_buf);
    uint32_t i;
    if (n < 0 || (uint32_t)n != len + 4u || get32(io_buf) != PROJ_FORMAT)
        return -1;
    for (i = 0; i < len; i++)
        ((uint8_t *)dst)[i] = io_buf[4u + i];
    stored_known[obj] = 1;
    stored_hash[obj] = hash_of(io_buf, len + 4u);
    return 0;
}

/* the settings may have grown since they were saved: a shorter payload of this format loads into
 * the front of dst, and the rest keeps what dst held (zeros: the new settings' defaults) */
static int load_settings(settings_t *dst)
{
    int n = plat_store_load(OBJ_SET, io_buf, sizeof io_buf);
    uint32_t i;
    if (n < 20 || (uint32_t)n > sizeof *dst + 4u || io_buf[0] != (uint8_t)PROJ_FORMAT || io_buf[1] || io_buf[2] || io_buf[3])
        return -1;
    for (i = 0; i + 4u < (uint32_t)n; i++)
        ((uint8_t *)dst)[i] = io_buf[4u + i];
    if ((uint32_t)n == sizeof *dst + 4u) {           /* exactly what a save of these settings writes */
        stored_known[OBJ_SET] = 1;
        stored_hash[OBJ_SET] = hash_of(io_buf, (uint32_t)n);
    }
    return 0;
}

static int save_obj(uint32_t obj, const void *src, uint32_t len)
{
    uint32_t i;
    if (len + 4u > sizeof io_buf)
        return -1;
    put32(io_buf, PROJ_FORMAT);
    for (i = 0; i < len; i++)
        io_buf[4u + i] = ((const uint8_t *)src)[i];
    return save_buf(obj, len + 4u);
}

typedef char sound_fits[(sizeof(sound_t) + 4u <= PLAT_STORE_MAX) ? 1 : -1];

static void arrange_defaults(project_t *pr)
{
    uint32_t i;
    uint8_t *z = (uint8_t *)&pr->arr;
    for (i = 0; i < sizeof pr->arr; i++)
        z[i] = 0;
}

/* ---- the packed stream ---------------------------------------------------------------------- */

/* the writer: fills io_buf after the object's header and writes it when full (measure: counts only) */
typedef struct {
    int measure, k, err;
    uint32_t fill, total;
} st_out_t;

static int st_flush(st_out_t *o)
{
    int rc = save_buf(STREAM_OBJ[o->k], o->fill);
    o->k++;
    o->fill = 4u;
    put32(io_buf, PROJ_STREAM);
    return rc;
}

static int st_put(void *ctx, const uint8_t *p, uint32_t n)
{
    st_out_t *o = ctx;
    o->total += n;
    if (o->measure)
        return o->total > STREAM_CAP ? -1 : 0;
    while (n) {
        if (o->fill == PLAT_STORE_MAX) {
            if (o->k + 1 >= NSTREAM || st_flush(o)) {
                o->err = 1;
                return -1;
            }
        }
        io_buf[o->fill++] = *p++;
        n--;
    }
    return 0;
}

/* Check capacity before any project object is changed, including settings and sound. */
static int measure_project(const project_t *pr, uint32_t *length)
{
    const uint8_t *raw = (const uint8_t *)pr + RAW_OFF;
    st_out_t o = {1, 0, 0, 0, 0};
    lz_sink_t sink = {st_put, &o};
    if (lz_compress(raw, RAW_LEN, &sink))
        return -7;
    *length = o.total;
    return 0;
}

static int save_stream(void)
{
    const uint8_t *raw = (const uint8_t *)&proj + RAW_OFF;
    uint32_t crc = crc32_of(raw, RAW_LEN);
    st_out_t o = {0, 0, 0, 0, 0};
    lz_sink_t sink = {st_put, &o};
    o.fill = STREAM_HDR;
    put32(io_buf, PROJ_STREAM);
    put32(io_buf + 4, RAW_LEN);
    put32(io_buf + 8, crc);
    put32(io_buf + 12, packed_len);
    if (lz_compress(raw, RAW_LEN, &sink) || o.err || st_flush(&o))
        return -3;
    while (o.k < NSTREAM)                             /* the objects past the stream: empty */
        if (st_flush(&o))
            return -3;
    return 0;
}

/* the reader: the objects in turn, from io_buf */
typedef struct {
    int k, n;
    uint32_t pos, left;
    uint32_t format;
} st_in_t;

static int st_get(void *ctx)
{
    st_in_t *s = ctx;
    if (!s->left)
        return -1;
    while (s->pos >= (uint32_t)s->n) {
        if (++s->k >= NSTREAM)
            return -1;
        s->n = plat_store_load(STREAM_OBJ[s->k], io_buf, sizeof io_buf);
        if (s->n < 4 || get32(io_buf) != s->format)
            return -1;
        stored_known[STREAM_OBJ[s->k]] = 1;
        stored_hash[STREAM_OBJ[s->k]] = hash_of(io_buf, (uint32_t)s->n);
        s->pos = 4u;
    }
    s->left--;
    return io_buf[s->pos++];
}

/* 0 = loaded; -1 = not there or not good (proj's patterns and arrangement then hold garbage) */
/* The old stream's 303 step was note+flags. Expand patterns back-to-front in
 * the already allocated new raw area; move the unchanged arrangement first. */
typedef struct { uint8_t note, flags; } bstep2_t;
typedef struct {
    bstep2_t step[NSTEPS];
    uint8_t len, rate, dir, transpose;
    tb3po_cfg_t gen;
} bpart2_t;
typedef struct {
    dpart_t drum[NKIT];
    bpart2_t bass[NBASS];
    brkpart_t brk;
    uint8_t swing, rsv[3];
} pattern2_t;
#define RAW_LEN_2 (NPAT * sizeof(pattern2_t) + sizeof(arrange_t))
static void stream_from2(project_t *pr)
{
    uint8_t *raw = (uint8_t *)pr + RAW_OFF;
    int p, b, s;
    uint32_t i;
    /* The destination is higher and overlaps the old pattern data. Copy from the
     * end so this also works with the firmware link, which has no libc memmove. */
    for (i = sizeof(arrange_t); i > 0; i--)
        raw[NPAT * sizeof(pattern_t) + i - 1u] = raw[NPAT * sizeof(pattern2_t) + i - 1u];
    for (p = NPAT - 1; p >= 0; p--) {
        pattern2_t old;
        pattern_t *dst = &pr->pat[p];
        __builtin_memcpy(&old, raw + (uint32_t)p * sizeof old, sizeof old);
        __builtin_memset(dst, 0, sizeof *dst);
        __builtin_memcpy(dst->drum, old.drum, sizeof old.drum);
        for (b = 0; b < NBASS; b++) {
            bpart_t *bp = &dst->bass[b];
            for (s = 0; s < NSTEPS; s++) {
                bp->step[s].note = old.bass[b].step[s].note;
                bp->step[s].flags = old.bass[b].step[s].flags;
            }
            bp->len = old.bass[b].len;
            bp->rate = old.bass[b].rate;
            bp->dir = old.bass[b].dir;
            bp->transpose = old.bass[b].transpose;
            bp->gen = old.bass[b].gen;
        }
        dst->brk = old.brk;
        dst->swing = old.swing;
        __builtin_memcpy(dst->rsv, old.rsv, sizeof old.rsv);
    }
    for (p = 0; p < NSTREAM; p++)
        stored_known[STREAM_OBJ[p]] = 0;             /* next save writes the current stream format */
}

static int project_has_snapshot(const project_t *pr)
{
    int k;
    for (k = 0; k < NLANE; k++)
        if (pr->arr.lane[k].used && pr->arr.lane[k].t == MOT_SNAPSHOT)
            return 1;
    return 0;
}

static int load_stream(project_t *pr, int n, uint32_t format)
{
    uint8_t *raw = (uint8_t *)pr + RAW_OFF;
    uint32_t crc = get32(io_buf + 8), plen = get32(io_buf + 12);
    uint32_t raw_len = format == PROJ_STREAM_OLD ? RAW_LEN_2 : RAW_LEN;
    st_in_t s = {0, n, STREAM_HDR, plen, format};
    lz_src_t src = {st_get, &s};
    if (n < (int)STREAM_HDR || get32(io_buf + 4) != raw_len || plen > STREAM_CAP)
        return -1;
    stored_known[STREAM_OBJ[0]] = 1;
    stored_hash[STREAM_OBJ[0]] = hash_of(io_buf, (uint32_t)n);
    if (lz_decompress(&src, raw, raw_len) || s.left || crc32_of(raw, raw_len) != crc)
        return -1;
    if (format == PROJ_STREAM_OLD)
        stream_from2(pr);
    if (format != PROJ_STREAM && project_has_snapshot(pr))
        return -1;                                  /* old formats never contained snapshots */
    packed_len = plen;
    return 0;
}

/* ---- format 1: 16 patterns of 32 steps, 32-step lanes ---------------------------------------- */

#define NSTEPS1 32
#define NPAT1 16
typedef struct {
    uint32_t hit[NDRUM];
    uint32_t accent;
    uint8_t len, rate, rsv[2];
} dpart1_t;
typedef struct {
    bstep2_t step[NSTEPS1];
    uint8_t len, rate, dir, transpose;
    tb3po_cfg_t gen;
} bpart1_t;
typedef struct {
    uint32_t steps;
    uint8_t set[BRK_NSET];
    uint8_t slot_a, slot_b, rsv[2];
} brkpart1_t;
typedef struct {
    dpart1_t drum[NKIT];
    bpart1_t bass[NBASS];
    brkpart1_t brk;
    uint8_t swing, rsv[3];
} pattern1_t;
typedef struct {
    uint8_t used, gen, pat, part, t, v, i, rsv;
    uint8_t val[NSTEPS1];
} lane1_t;
#define ARR1_LEN (sizeof(song_t) + NLANE * sizeof(lane1_t))
#define ARR1_A (PLAT_STORE_MAX - 4u)                 /* format 1's arrangement bytes in OBJ_SONG; the rest in OBJ_MOTION */
typedef char arr1_room[(ARR1_LEN <= sizeof(arrange_t)) ? 1 : -1];   /* it unpacks in proj.arr */

static void pattern_from1(pattern_t *p, const pattern1_t *o)
{
    int k, s, v;
    for (k = 0; k < NKIT; k++) {
        for (v = 0; v < NDRUM; v++)
            p->drum[k].hit[v][0] = o->drum[k].hit[v];
        p->drum[k].accent[0] = o->drum[k].accent;
        p->drum[k].len = o->drum[k].len;
        p->drum[k].rate = o->drum[k].rate;
    }
    for (k = 0; k < NBASS; k++) {
        for (s = 0; s < NSTEPS1; s++) {
            p->bass[k].step[s].note = o->bass[k].step[s].note;
            p->bass[k].step[s].flags = o->bass[k].step[s].flags;
        }
        p->bass[k].len = o->bass[k].len;
        p->bass[k].rate = o->bass[k].rate;
        p->bass[k].dir = o->bass[k].dir;
        p->bass[k].transpose = o->bass[k].transpose;
        p->bass[k].gen = o->bass[k].gen;
    }
    p->brk.steps = o->brk.steps;
    for (k = 0; k < BRK_NSET; k++)
        p->brk.set[k] = o->brk.set[k];
    p->brk.slot_a = o->brk.slot_a;
    p->brk.slot_b = o->brk.slot_b;
    p->swing = o->swing;
}

/* OBJ_PAT0 + OBJ_PAT1, format 1 (eight patterns each) onto patterns 1-16 (the defaults are in place) */
static int load_patterns1(project_t *pr)
{
    int h, i;
    for (h = 0; h < 2; h++) {
        int n = plat_store_load(h ? OBJ_PAT1 : OBJ_PAT0, io_buf, sizeof io_buf);
        if (n != (int)(8u * sizeof(pattern1_t) + 4u) || get32(io_buf) != PROJ_FORMAT)
            return -1;
        for (i = 0; i < 8; i++) {
            pattern1_t o;
            uint32_t b;
            for (b = 0; b < sizeof o; b++)
                ((uint8_t *)&o)[b] = io_buf[4u + (uint32_t)i * sizeof o + b];
            pattern_from1(&pr->pat[h * 8 + i], &o);
        }
    }
    return 0;
}

/* OBJ_SONG + OBJ_MOTION, format 1: the song as it is, the lanes widened to 64 steps (in place, last
 * first: lane k's new place never reaches below its old one) */
static int load_arrange1(project_t *pr)
{
    uint8_t *a = (uint8_t *)&pr->arr;
    int k, i;
    if (load_obj(OBJ_SONG, a, ARR1_A) || load_obj(OBJ_MOTION, a + ARR1_A, ARR1_LEN - ARR1_A))
        return -1;
    for (k = NLANE - 1; k >= 0; k--) {
        lane1_t o;
        lane_t *l = &pr->arr.lane[k];
        uint32_t b;
        for (b = 0; b < sizeof o; b++)
            ((uint8_t *)&o)[b] = a[sizeof(song_t) + (uint32_t)k * sizeof o + b];
        l->used = o.used;
        l->gen = o.gen;
        l->pat = o.pat;
        l->part = o.part;
        l->t = o.t;
        l->v = o.v;
        l->i = o.i;
        l->rsv = 0;
        for (i = 0; i < NSTEPS; i++)
            l->val[i] = i < NSTEPS1 ? o.val[i] : MOT_NONE;
    }
    stored_known[OBJ_SONG] = stored_known[OBJ_MOTION] = 0;   /* the next save writes the stream */
    return 0;
}

/* ---- load, save ----------------------------------------------------------------------------- */

/* the 909 SD gained DECAY at pot 3: what was 3.. (LEVEL, DRIVE, DIST, REV, DLY, PAN) moves up one,
 * the knobs' motion with it, and DECAY takes its default (9W9's 340 ms) */
static void sound_rev1(project_t *pr)
{
    uint8_t *sd = pr->sound.v[T_909][DR_SD];
    int i, n = engine_nparams(T_909, DR_SD);
    for (i = n - 1; i > 3; i--)
        sd[i] = sd[i - 1];
    sd[3] = engine_param(T_909, DR_SD, 3)->def;
    for (i = 0; i < NLANE; i++) {
        lane_t *l = &pr->arr.lane[i];
        if (l->used && l->t == T_909 && l->v == DR_SD && l->i >= 3)
            l->i++;
    }
}

/* the master's one-knob COMP and FILTER and the delay's TAPE arrived: each set to its nearest to what
 * the details say (the details themselves stay, and are applied after: the sound is as it was) */
static void sound_rev2(project_t *pr)
{
    uint8_t *m = pr->sound.v[T_MST][0];
    int th = m[MST_THRESH], cut = m[MST_CUTOFF];
    float thr = (float)th * (48.0f / 127.0f) - 48.0f, x = (-thr - 4.0f) / 28.0f;
    m[MST_COMP1] = (uint8_t)(!m[MST_RATIO] ? 0 : x <= 0.0f ? 1 : x >= 1.0f ? 127 : (int)(x * 127.0f + 0.5f));
    m[MST_DJF] = (uint8_t)(m[MST_MODE] == 0 ? 64 : m[MST_MODE] == 3 ? 69 + cut * 58 / 120
                           : (cut < 8 ? 0 : (cut - 8) * 59 / 119));
    pr->sound.v[T_FX][1][0] = (uint8_t)(pr->sound.v[T_FX][0][FX_DL_TYPE] ? 16 + pr->sound.v[T_FX][0][FX_DL_WEAR] * 111 / 127 : 0);
}

/* a BD gained REV and DLY before its PAN (909: 8 -> 10, 808: 7 -> 9): the pan and its motion move, the sends start
 * at none (as the kick had) */
static void sound_bd_sends(project_t *pr, int t, int pan)
{
    uint8_t *bd = pr->sound.v[t][0];
    int i;
    bd[pan + 2] = bd[pan];
    bd[pan] = bd[pan + 1] = 0;
    for (i = 0; i < NLANE; i++) {
        lane_t *l = &pr->arr.lane[i];
        if (l->used && l->t == t && l->v == 0 && l->i == pan)
            l->i = (uint8_t)(pan + 2);
    }
}

static void patterns_defaults(project_t *pr)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&pr->pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}

/* CLOCK SOURCE (MID-13) is in bt bits 1-2. An object without PROJ_BT_CLK_IN comes from a74 or older. a74 kept
 * the value 0-2 in byte 38, and byte 39 was 0. Byte 38 is ck_x1 in the clip keys (ARP SWING and HUMAN), so the
 * value moves to bt, and byte 38 becomes 0 (no SWING, no HUMAN). Other bytes there stay, and CLOCK SOURCE is AUTO. */
typedef char a74_clk_in_set[(PROJ_A74_CLK_OFF + 2u == sizeof(settings_t)) ? 1 : -1];
static void project_clk_check(project_t *pr)
{
    uint8_t *raw = (uint8_t *)&pr->set, b = pr->set.bt, v = raw[PROJ_A74_CLK_OFF];
    if (!(b & PROJ_BT_CLK_IN)) {
        b &= 1u;
        if (v <= SEQ_CLK_EXT && !raw[PROJ_A74_CLK_OFF + 1u]) {
            b |= (uint8_t)(v << 1);
            raw[PROJ_A74_CLK_OFF] = 0;
        }
        b |= PROJ_BT_CLK_IN;
    } else if (PROJ_CLK_SRC(b) > SEQ_CLK_EXT)
        b &= (uint8_t)~PROJ_BT_CLK_MASK;
    pr->set.bt = b;
}

/* each object is read straight into its place in proj: a load copies only a valid payload */
static int read_project(project_t *pr)
{
    int bad = 0, i, sound_ok, n;
    settings_t set;
    project_defaults_for(pr, proj.set.bt);
    packed_len = 0;
    __builtin_memcpy(&set, &pr->set, sizeof set);    /* (memcpy: the padding bytes 38-39 too, project_clk_check) */
    if (load_settings(&set) == 0 && set.magic == PROJ_MAGIC && set.format == PROJ_FORMAT && set.snd_rev <= PROJ_SND_REV)
        __builtin_memcpy(&pr->set, &set, sizeof set);
    else
        bad = 1;
    project_clk_check(pr);                              /* (first: it can clear the a74 byte 38) */
    project_ck_check(pr, 0);                              /* (main_fm1.c trusts the clip keys settings) */
    sound_ok = load_obj(OBJ_SOUND, &pr->sound, sizeof pr->sound) == 0;
    if (!sound_ok)
        bad = 1;
    if (sound_ok && pr->set.snd_rev < 3)             /* before the 909 BD's sends: its pan moves up two */
        sound_bd_sends(pr, T_909, 8);                     /* (first: the pans' own fixes below see the new place) */
    if (sound_ok && pr->set.snd_rev < 4)             /* the 808 BD's, the same */
        sound_bd_sends(pr, T_808, 7);
    if (pr->set.stereo < 1)                          /* saved mono: no pans there (zeros = hard left) */
        for (i = 0; i < NPARTS; i++)
            pr->sound.v[T_MIX][i][MX_PAN] = 64;
    if (pr->set.stereo < 2)                          /* saved before the drum voices had pans */
        engine_sound_centre_drum_pans(&pr->sound);
    pr->set.stereo = PROJ_STEREO;
    n = plat_store_load(OBJ_PAT0, io_buf, sizeof io_buf);
    if (n >= 4 && (get32(io_buf) == PROJ_STREAM || get32(io_buf) == PROJ_STREAM_3 || get32(io_buf) == PROJ_STREAM_OLD)) {
        if (load_stream(pr, n, get32(io_buf))) {          /* torn or broken: the defaults, not garbage */
            patterns_defaults(pr);
            arrange_defaults(pr);
            bad = 1;
        }
    } else {
        if (load_patterns1(pr)) {                       /* half a set is no set: the defaults */
            patterns_defaults(pr);
            bad = 1;
        }
        /* the song and the lanes: absent in a project saved before they existed, which is no fault */
        if (load_arrange1(pr))
            arrange_defaults(pr);
        stored_known[OBJ_PAT0] = stored_known[OBJ_PAT1] = 0;
    }
    if (pr->arr.song.len > NSONG) {
        bad = 1;
        pr->arr.song.len = NSONG;
    }
    if (sound_ok && pr->set.snd_rev < 1)             /* saved before the 909 SD's DECAY: its pots move up one */
        sound_rev1(pr);
    if (sound_ok && pr->set.snd_rev < 2)             /* before the one-knob COMP, FILTER, TAPE */
        sound_rev2(pr);
    pr->set.snd_rev = PROJ_SND_REV;
    return bad ? -1 : 0;
}

void project_defaults(void) { project_defaults_for(&proj, proj.set.bt); }
int project_load(void) { return read_project(&proj); }
void project_reset_stage(void) { project_defaults_for(&project_stage, proj.set.bt); }

/* Validate indices before the UI, sequencer or DSP can see an imported project.
 * Unused sound cells and motion lanes are opaque, just as they are in storage.
 * Break slot indices are resolved against the companion's complete sample manifest:
 * a slot may not exist here yet, because sample restoration follows this check. */
int project_validate_stage(void)
{
    const project_t *p = &project_stage;
    const settings_t *s = &p->set;
    uint32_t length;
    int t, v, i, k, b;
    if (s->magic != PROJ_MAGIC || s->format != PROJ_FORMAT || s->stereo != PROJ_STEREO ||
        s->snd_rev != PROJ_SND_REV || s->palette > 4 || s->keyled > 2 || s->clk_out > 1 ||
        s->notes_out > 1 || s->bpm_x10 < 200 || s->bpm_x10 > 2750 || s->accent_q7 > 127 ||
        s->keysound > 1 || s->autosave_off > 1 || s->out_loud > 1 || s->usb_full > 1 ||
        (s->bt & 0xC0u) || !(s->bt & PROJ_BT_CLK_IN) || PROJ_CLK_SRC(s->bt) > SEQ_CLK_EXT ||
        s->ck_ch < 2)
        return -1;
    for (i = 0; i < 12; i++)
        if ((&s->ck_ch)[i] > CK_MAX[i])
            return -1;
    for (i = 0; i < NPARTS; i++)
        if (s->midi_ch[i] > 17)
            return -1;
    if (p->sound.v[T_FX][1][0] > 127)       /* UI's derived TAPE control */
        return -1;
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t); v++)
            for (i = 0; i < engine_nparams(t, v); i++)
                if (p->sound.v[t][v][i] > engine_param(t, v, i)->max)
                    return -1;
    for (k = 0; k < NPAT; k++) {
        const pattern_t *pat = &p->pat[k];
        if (pat->swing > 100)
            return -1;
        for (b = 0; b < NKIT; b++)
            if (!pat->drum[b].len || pat->drum[b].len > NSTEPS || pat->drum[b].rate >= NRATES)
                return -1;
        for (b = 0; b < NBASS; b++) {
            const bpart_t *bp = &pat->bass[b];
            const tb3po_cfg_t *g = &bp->gen;
            if (!bp->len || bp->len > NSTEPS || bp->rate >= NRATES || bp->dir >= NDIRS || bp->transpose > 48 ||
                g->density > 100 || g->accent > 100 || g->slide > 100 || !g->oct_range || g->oct_range > 3 ||
                g->root > 11 || g->scale >= TB3PO_NSCALES || g->base_oct > 4 || g->mutate_bars > 16)
                return -1;
            for (i = 0; i < NSTEPS; i++)
                if (bp->step[i].note > 127 || bp->step[i].velocity > 127 ||
                    (bp->step[i].flags & ~15u) || bstep_gate(&bp->step[i]) > G_TIE)
                    return -1;
        }
        for (i = 0; i < BRK_NSET; i++)
            if (pat->brk.set[i] > breaks_param(i)->max)
                return -1;
        for (i = 0; i < 16; i++)
            if (pat->brk.slice[i] > 8)
                return -1;
    }
    if (p->arr.song.len > NSONG)
        return -1;
    for (k = 0; k < p->arr.song.len; k++)
        for (i = 0; i < NPARTS; i++)
            if (p->arr.song.bar[k].pat[i] >= NPAT)
                return -1;
    for (k = 0; k < NLANE; k++) {
        const lane_t *l = &p->arr.lane[k];
        int max;
        if (!l->used)
            continue;
        if (l->used != 1 || l->pat >= NPAT || l->part >= NPARTS)
            return -1;
        if (l->t == MOT_SNAPSHOT) {
            unsigned bank = l->val[NSTEPS - 1];
            if ((unsigned)(l->part - NKIT) >= NBASS || l->v >= NSTEPS ||
                l->i != MOT_SNAPSHOT_SCHEMA || (bank != 0 && bank != MOT_SNAPSHOT_PARAMS) ||
                (l->val[32 + bank / MOT_SNAPSHOT_PARAMS] != 1 && l->val[32 + bank / MOT_SNAPSHOT_PARAMS] != 2))
                return -1;
            for (i = 0; i < MOT_SNAPSHOT_PARAMS; i++)
                if (l->val[bank + (unsigned)i] >
                    (l->val[32 + bank / MOT_SNAPSHOT_PARAMS] == 1 ?
                     engine_param(T_303, l->part-NKIT, i)->max : 127))
                    return -1;
            /* Duplicate keys would make lookup order define a different sound. */
            for (i = 0; i < k; i++) {
                const lane_t *a = &p->arr.lane[i];
                if (a->used && a->t == MOT_SNAPSHOT && a->pat == l->pat &&
                    a->part == l->part && a->v == l->v)
                    return -1;
            }
            continue;
        }
        if (l->t == T_PROB) {
            if ((l->v >> 4) != l->part || (l->v & 15) >= (l->part < NKIT ? NDRUM : 1) || l->i)
                return -1;
            max = PROB_COND + PROB_NCOND - 1;
        } else {
            if (l->t >= NTARGETS || l->v >= engine_nvoices(l->t) || l->i >= engine_nparams(l->t, l->v))
                return -1;
            max = engine_param(l->t, l->v, l->i)->max;
        }
        for (i = 0; i < NSTEPS; i++)
            if (l->val[i] != MOT_NONE && l->val[i] > max)
                return -1;
    }
    return measure_project(p, &length);
}

int project_load_stage(void)
{
    uint32_t live_length = packed_len;
    int rc = read_project(&project_stage);
    packed_len = live_length;
    return rc ? rc : project_validate_stage();
}

void project_commit_stage(void)
{
    /* Device roles belong to this keyboard; loading music must not change its
     * Bluetooth connection or opt-in control surface mode. */
    project_stage.set.bt = (uint8_t)((project_stage.set.bt & ~PROJ_DEVICE_BT) | (proj.set.bt & PROJ_DEVICE_BT));
    __builtin_memcpy(&proj, &project_stage, sizeof proj);
    (void)measure_project(&proj, &packed_len);
}

/* Portable inner file, LE u32 words. The version fixes the pointer-free project_t
 * layout; no sample bytes or device calibration are stored in this payload. */
void project_file_header(uint8_t *h)
{
    put32(h, 0x50583058u);                 /* X0XP */
    put32(h + 4, 1);
    put32(h + 8, sizeof project_stage);
    put32(h + 12, crc32_of((const uint8_t *)&project_stage, sizeof project_stage));
    put32(h + 16, PROJ_FORMAT);
    put32(h + 20, PROJ_STREAM);
    put32(h + 24, PROJ_SND_REV);
    put32(h + 28, 0);
}

int project_file_validate(const uint8_t *h)
{
    if (get32(h) != 0x50583058u || get32(h + 4) != 1 || get32(h + 8) != sizeof project_stage ||
        get32(h + 16) != PROJ_FORMAT ||
        (get32(h + 20) != PROJ_STREAM && get32(h + 20) != PROJ_STREAM_3) ||
        get32(h + 24) != PROJ_SND_REV || get32(h + 28))
        return -1;
    if (get32(h + 12) != crc32_of((const uint8_t *)&project_stage, sizeof project_stage))
        return -4;
    if (get32(h + 20) != PROJ_STREAM && project_has_snapshot(&project_stage))
        return -1;
    return project_validate_stage();
}

void project_apply_io(void)
{
    static const uint8_t DEF[5] = {9, 10, 1, 2, 3};   /* 909 10, 808 11, 303s 2 / 3, break 4 */
    int p;
    seq.send_clock = proj.set.clk_out;
    seq.send_notes = proj.set.notes_out;
    seq.clk_src = PROJ_CLK_SRC(proj.set.bt);
    for (p = 0; p < 5; p++) {
        int v = proj.set.midi_ch[p];
        seq.ch[p] = (uint8_t)(!v ? DEF[p] : v > 16 ? (int)SEQ_CH_OFF : v - 1);
    }
    engine_out_loud(proj.set.out_loud);
    engine_usb_full(proj.set.usb_full);
}

static void capture_settings(settings_t *set)
{
    int p;
    for (p = 0; p < 5; p++)
        set->midi_ch[p] = (uint8_t)(seq.ch[p] == SEQ_CH_OFF ? 17 : seq.ch[p] + 1);
    set->bpm_x10 = (uint16_t)(seq.bpm * 10.0f + 0.5f);
    set->accent_q7 = seq.accent_q7;
    set->clk_out = seq.send_clock;
    set->notes_out = seq.send_notes;
    /* The UI stores CLOCK SOURCE in set->bt. Clip mode temporarily forces the
     * runtime sequencer to AUTO; saving must retain the user's stored choice. */
}

void project_capture_stage(void)
{
    __builtin_memcpy(&project_stage, &proj, sizeof proj);
    capture_settings(&project_stage.set);
}

/* 0 = saved and verified; -7 = the patterns, song and motion do not fit (nothing was written) */
int project_save(void)
{
    capture_settings(&proj.set);
    if (measure_project(&proj, &packed_len))
        return -7;
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set))
        return -1;
    if (save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound))
        return -2;
    return save_stream();
}
