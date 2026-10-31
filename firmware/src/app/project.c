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
 * Every object carries its format number. The original-engine profile accepts only
 * revision/stream 13. In the legacy profile, a project saved before the stream (format 1: 16
 * patterns of 32 steps in OBJ_PAT0/1, the song and 32-step lanes in OBJ_SONG/OBJ_MOTION) is
 * converted as it loads: its steps land on steps 1-32, its patterns on 1-16. */
#include "x0x.h"
#include "lz.c"
#include "../crc32_nibble.h"           /* one nibble table shared with storage.c */

project_t proj;
/* Reuses the UI undo shadow while an exclusive project operation is active. */
project_t project_stage X0X_POOL;

#define PROJ_STREAM 13u                 /* compact Chicago schema2; current raw patterns/arrangement */
static int stream_known(uint32_t format) { return format == PROJ_STREAM; }
#define PROJ_STREAM_PREV 3u             /* velocity steps, legacy gates */
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
static int project_validate_for(const project_t *pr);
static uint8_t project_incompatible; /* retain unsupported stored bytes until explicit staged adoption */
static uint32_t packed_len;             /* the last save's (or load's) packed stream, bytes */

/* SMK-37 clip mode keybed settings (main_fm1.c), ck_ch .. ck_x2: channel 2, C MAJOR NEAR, MAJ root position,
 * ARP UP 1/16, 1 octave, gate 50 %, no swing, human, rhythm or strum; and the highest values. A value out of
 * range (for example an older object): all go back to the defaults. An older object without ck_x1/ck_x2 loads
 * zeros there: the defaults. */
static const uint8_t CK_DEF[12] = {2, 0, 0, 0, 0, 0, 9, 0, 4, 0, 0, 0},
                     CK_MAX[12] = {15, 11, 23, 1, 29, 5, 12, 3, 9, 3, 63, 127};
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
    __builtin_memcpy(&pr->set.ck_ch, CK_DEF, sizeof CK_DEF);
    engine_sound_defaults(&pr->sound);
    arrange_defaults(pr);
    /* the 303 lines start empty, like the drums: OCT+ on TB-3PO (or keyboard mode) writes one */
    for (i = 0; i < NPAT; i++)
        pattern_init(&pr->pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}

/* how full the stream's room is, in % (GLO > ABOUT) */
int project_mem_pct(void) { return (int)((packed_len * 100u + STREAM_CAP - 1u) / STREAM_CAP); }

/* object payload: format word + body. The device aliases this to st_buf
 * (x0x.c PROJECT_IO_BUFFER). Host builds keep a private buffer. */
#ifdef PROJECT_IO_BUFFER
#define io_buf PROJECT_IO_BUFFER
#else
static uint8_t io_buf[PLAT_STORE_MAX] __attribute__((aligned(4)));
#endif

/* what each object last held on the flash (a hash of its payload), so a save writes only the
 * objects that changed: an autosave usually erases one or two sectors, not six */
static uint64_t stored_hash[OBJ_NOBJ];
static uint8_t stored_known[OBJ_NOBJ];
__attribute__((noinline)) static uint64_t hash_of(const uint8_t *p, uint32_t n)        /* FNV-1a, 64-bit */
{
    uint64_t h = UINT64_C(14695981039346656037);
    while (n--)
        h = (h ^ *p++) * UINT64_C(1099511628211);
    return h;
}

__attribute__((noinline)) static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
__attribute__((noinline)) static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

#define crc32_of x0x_crc32                               /* zlib CRC-32, 4 bits per step */

/* the object is the n bytes of io_buf, format word included; written only if they changed */
static int save_buf(uint32_t obj, uint32_t n)
{
    uint64_t h = hash_of(io_buf, n);
    int rc;
    if (stored_known[obj] && stored_hash[obj] == h)
        return 0;                                    /* the flash already holds exactly this */
    rc = plat_store_save(obj, io_buf, n);
    stored_known[obj] = rc == 0;
    stored_hash[obj] = h;
    return rc;
}

/* the settings may have grown since they were saved: a shorter payload of this format loads into
 * the front of dst, and the rest keeps what dst held (zeros: the new settings' defaults) */
static int load_settings(settings_t *dst)
{
    int n = plat_store_load(OBJ_SET, io_buf, sizeof io_buf);
    if (n < 0) return -1;
    if ((uint32_t)n != sizeof *dst + 4u || get32(io_buf) != PROJ_FORMAT ||
        io_buf[4u + __builtin_offsetof(settings_t, snd_rev)] != PROJ_SND_REV) return -9;
    if (n < 20 || (uint32_t)n > sizeof *dst + 4u || io_buf[0] != (uint8_t)PROJ_FORMAT || io_buf[1] || io_buf[2] || io_buf[3])
        return -1;
    __builtin_memcpy(dst, io_buf + 4u, (uint32_t)n - 4u);
    if ((uint32_t)n == sizeof *dst + 4u) {           /* exactly what a save of these settings writes */
        stored_known[OBJ_SET] = 1;
        stored_hash[OBJ_SET] = hash_of(io_buf, (uint32_t)n);
    }
    return 0;
}

static int save_obj(uint32_t obj, const void *src, uint32_t len)
{
    if (len + 4u > sizeof io_buf)
        return -1;
    put32(io_buf, PROJ_FORMAT);
    __builtin_memcpy(io_buf + 4u, src, len);
    return save_buf(obj, len + 4u);
}

typedef char sound_fits[(sizeof(sound_t) + 4u <= PLAT_STORE_MAX) ? 1 : -1];
#define SOUND_BYTES_V10 (NTARGETS * NVOICES_MAX * NPARAMS_MAX)
#define SOUND_CELL_V4 16u
#define SOUND_BYTES_V4 (NTARGETS * NVOICES_MAX * SOUND_CELL_V4)
#define SOUND_CELL_V7 24u
#define SOUND_BYTES_V7 (NTARGETS * NVOICES_MAX * SOUND_CELL_V7)
#define SOUND_CELL_V8 28u
#define SOUND_BYTES_V8 (NTARGETS * NVOICES_MAX * SOUND_CELL_V8)

static int load_sound(sound_t *dst)
{
    int n = plat_store_load(OBJ_SOUND, io_buf, sizeof io_buf);
    if (n < 0) return -1;
    if ((uint32_t)n != sizeof *dst + 4u || get32(io_buf) != PROJ_FORMAT) return -9;
    __builtin_memcpy(dst, io_buf + 4u, sizeof *dst);
    if (engine_bass_sound_validate(dst)) return -9;
    stored_known[OBJ_SOUND] = 1;
    stored_hash[OBJ_SOUND] = hash_of(io_buf, (uint32_t)n);
    return 0;
}

__attribute__((noinline)) static void arrange_defaults(project_t *pr)
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
    uint8_t unsupported; /* a consumed segment must retain its format classification */
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
        if (s->n < 4)
            return -1;
        uint32_t format = get32(io_buf);
        if (!stream_known(format)
            ) {
            s->unsupported = 1;
            return -1;
        }
        if (format != s->format)
            return -1;
        stored_known[STREAM_OBJ[s->k]] = 1;
        stored_hash[STREAM_OBJ[s->k]] = hash_of(io_buf, (uint32_t)s->n);
        s->pos = 4u;
    }
    s->left--;
    return io_buf[s->pos++];
}

/* 0 = loaded; -1 = not there or not good (proj's patterns and arrangement then hold garbage) */

/* No frozen snapshot layout is reinterpreted as ordinary motion or endpoints.
 * Streams 1/2/3 never assigned the upper flags nibble. */
static int stream_parts_ok(const pattern_t *pat,const arrange_t *arr,uint32_t format)
{
    int p, b, k;
    for (p = 0; p < NPAT; p++) {
        /* Validate old meanings before migration: old mode 1 is not count code 1. */
        if (pat[p].brk.slice_mode > (format < 8u ? 1u : 15u)) return 0;
        for (k = 0; k < 16; k++)
            if (pat[p].brk.slice[k] > (format < 8u ? 8u : 32u)) return 0;
    }
    for (p = 0; p < NPAT; p++)
        for (b = 0; b < NBASS; b++)
            for (k = 0; k < NSTEPS; k++) {
                unsigned f = pat[p].bass[b].step[k].flags;
                if ((f & BS_GATE_MASK) > G_TIE ||
                    (format < 6u && (f >> BS_END_SHIFT)) ||
                    ((f >> BS_END_SHIFT) && (f & BS_GATE_MASK) == G_REST))
                    return 0;
            }
    for (k = 0; k < NLANE; k++) {
        const lane_t *l=&arr->lane[k];
        if(!l->used)continue;
        if(l->t==0xfeu)return 0; /* frozen historical contract remains unsupported */
        if(l->t==0xfdu) {
#if X0X_BASS_NOTE_SOUND
            if((format!=7u && format!=9u) || engine_bass_note_validate(l) ||
               bstep_gate(&pat[l->pat].bass[l->part-PART_303A].step[l->v])!=G_NOTE)return 0;
            for(int j=0;j<k;j++)if(arr->lane[j].used && arr->lane[j].t==l->t &&
                arr->lane[j].pat==l->pat && arr->lane[j].part==l->part && arr->lane[j].v==l->v)return 0;
#else
            return 0;
#endif
        }
    }
    return 1;
}
static int stream_layout_ok(const project_t *pr,uint32_t format)
{return stream_parts_ok(pr->pat,&pr->arr,format);}

#if !X0X_ORIGINAL_SYNTH
/* The retired project profile only. Revision 13 needs no migration. */
static void migrate_break_counts(project_t *pr, uint32_t format)
{
    if (format < 8u)
        for (int p = 0; p < NPAT; p++) pr->pat[p].brk.slice_mode = 0;
}
#endif


static int load_stream(project_t *pr, int n, uint32_t format)
{
    uint8_t *raw = (uint8_t *)pr + RAW_OFF;
    uint32_t crc = get32(io_buf + 8), plen = get32(io_buf + 12);
    uint32_t raw_len = RAW_LEN;
    st_in_t s = {0, n, STREAM_HDR, plen, format, 0};
    lz_src_t src = {st_get, &s};
    if (n < (int)STREAM_HDR || get32(io_buf + 4) != raw_len || plen > STREAM_CAP)
        return -1;
    stored_known[STREAM_OBJ[0]] = 1;
    stored_hash[STREAM_OBJ[0]] = hash_of(io_buf, (uint32_t)n);
    int decode_rc = lz_decompress(&src, raw, raw_len);
    /* Only st_get encounters used continuation objects; stale unused tails are
     * deliberately irrelevant. Do not turn newer data into ordinary corruption
     * and allow the boot-default autosave path to overwrite it. */
    if (s.unsupported)
        return -9;
    if (decode_rc || s.left || crc32_of(raw, raw_len) != crc)
        return -1;
    if (!stream_layout_ok(pr, format))
        return -9;
#if !X0X_ORIGINAL_SYNTH
    migrate_break_counts(pr, format);
#endif
    if (format != PROJ_STREAM)
        for (int k = 0; k < NSTREAM; k++) stored_known[STREAM_OBJ[k]] = 0;
    packed_len = plen;
    return 0;
}


static void patterns_defaults(project_t *pr)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&pr->pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}


/* Objects are read into the candidate project; the original profile admits only
 * a completely validated candidate and leaves safe defaults on every failure. */
static int read_project(project_t *pr)
{
    uint8_t device_bt = proj.set.bt;
    project_defaults_for(pr, device_bt);
    packed_len = 0;
    settings_t set = pr->set;
    int bad = 0, rc = load_settings(&set);
    if (rc == -9) goto unsupported;
    if (!rc) {
        if (set.magic != PROJ_MAGIC || set.format != PROJ_FORMAT ||
            set.snd_rev != PROJ_SND_REV || set.stereo != PROJ_STEREO) goto unsupported;
        set.bt &= (uint8_t)~PROJ_BT_SMC_USER;   /* canonical DAW; stored hash still names original bytes */
        pr->set = set;
        device_bt = set.bt;                       /* retain saved Bluetooth role on partial-load fallback */
    } else {
        bad = 1;
#if BOARD_SMK37
        /* No stored device settings: connect an SMC on the very first boot.
         * Valid saved OFF/DEVICE settings are never overwritten. */
        if (rc == -1 && !(device_bt & PROJ_DEVICE_BT))
            device_bt = (uint8_t)(1u | PROJ_BT_SMC_CENTRAL);
#endif
    }
    rc = load_sound(&pr->sound);
    if (rc == -9) goto unsupported;
    if (rc) bad = 1;
    int n = plat_store_load(OBJ_PAT0, io_buf, sizeof io_buf);
    if (n >= 0 && (n < 4 || !stream_known(get32(io_buf)))) goto unsupported;
    if (n >= 4) {
        rc = load_stream(pr, n, get32(io_buf));
        if (rc == -9) goto unsupported;
        if (rc) {
            patterns_defaults(pr);
            arrange_defaults(pr);
            bad = 1;
        }
    } else bad = 1;
    rc = bad ? -1 : project_validate_for(pr);
    if (!rc) return 0;
    goto failed;
unsupported:
    rc = -9;
failed:
    project_defaults_for(pr, device_bt);
    packed_len = 0;
    return rc;
}

void project_defaults(void) { project_defaults_for(&proj, proj.set.bt); }
int project_load(void)
{
    int rc = read_project(&proj);
    project_incompatible = (uint8_t)(rc == -9
#if X0X_BASS_TAGGED_SOUND
        || (!rc && engine_bass_sound_validate(&proj.sound))
#endif
        );
    return rc;
}
int project_storage_unsupported(void) { return project_incompatible; }
void project_reset_stage(void) { project_defaults_for(&project_stage, proj.set.bt); }

/* Validate indices before the UI, sequencer or DSP can see an imported project.
 * Unused sound cells and motion lanes are opaque, just as they are in storage.
 * Break slot indices are resolved against the companion's complete sample manifest:
 * a slot may not exist here yet, because sample restoration follows this check. */
static int project_validate_for(const project_t *p)
{
    const settings_t *s = &p->set;
    uint32_t length;
    int t, v, i, k, b;
#if X0X_BASS_TAGGED_SOUND
    int bass_status=engine_bass_sound_validate(&p->sound);
    if(bass_status)return bass_status;
#endif
    if (s->magic != PROJ_MAGIC || s->format != PROJ_FORMAT || s->stereo != PROJ_STEREO ||
        s->snd_rev != PROJ_SND_REV || s->palette > 5 || s->keyled > 2 || s->clk_out > 1 ||
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
                if (
#if X0X_BASS_TAGGED_SOUND
                    t!=T_303 &&
#endif
                    p->sound.v[t][v][i] > engine_param(t, v, i)->max)
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
                    bstep_gate(&bp->step[i]) > G_TIE ||
                    ((bp->step[i].flags >> BS_END_SHIFT) && bstep_gate(&bp->step[i]) == G_REST))
                    return -1;
        }
        for (i = 0; i < BRK_NSET; i++)
            if (pat->brk.set[i] > breaks_param(i)->max)
                return -1;
        for (i = 0; i < 16; i++)
            if (pat->brk.slice[i] > 32)
                return -1;
        if (pat->brk.slice_mode > 15)
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
#if X0X_BASS_NOTE_SOUND
        if(l->t==BASS_NOTE_TARGET) {
            if(engine_bass_note_validate(l))return -1;
            if(bstep_gate(&p->pat[l->pat].bass[l->part-PART_303A].step[l->v])!=G_NOTE)return -1;
            for(int j=0;j<k;j++)if(p->arr.lane[j].used && p->arr.lane[j].t==l->t &&
                p->arr.lane[j].pat==l->pat && p->arr.lane[j].part==l->part && p->arr.lane[j].v==l->v)return -1;
            continue;
        }
#endif
        if (l->t == T_PROB) {
            if ((l->v >> 4) != l->part || (l->v & 15) >= (l->part < NKIT ? NDRUM : 1) || l->i)
                return -1;
            max = PROB_COND + PROB_NCOND - 1;
        } else if (l->t == T_VEL) {
            /* Drum black-key strike velocity only; 303 steps keep bstep.velocity. */
            if (l->part >= NKIT || (l->v >> 4) != l->part || (l->v & 15) >= NDRUM || l->i)
                return -1;
            max = 127;
        } else {
            if (l->t >= NTARGETS || l->v >= engine_nvoices(l->t) ||
#if X0X_BASS_TAGGED_SOUND
                (l->t==T_303?!engine_bass_bank_param(engine_bass_sound_bank(&p->sound,l->v),l->i):l->i>=engine_nparams(l->t,l->v))
#else
                l->i>=engine_nparams(l->t,l->v)
#endif
                )
                return -1;
            max =
#if X0X_BASS_TAGGED_SOUND
                l->t==T_303?engine_bass_bank_param(engine_bass_sound_bank(&p->sound,l->v),l->i)->max:
#endif
                engine_param(l->t, l->v, l->i)->max;
        }
        for (i = 0; i < NSTEPS; i++)
            if (l->val[i] != MOT_NONE && l->val[i] > max)
                return -1;
    }
    return measure_project(p, &length);
}

int project_validate_stage(void) { return project_validate_for(&project_stage); }


int project_load_stage(void)
{
    uint32_t live_length = packed_len;
    int rc = read_project(&project_stage);
    if (rc == -9) project_incompatible = 1;
    packed_len = live_length;
    rc=rc?rc:project_validate_stage();
    return rc;
}

void project_commit_stage(void)
{
    /* Device roles belong to this keyboard; loading music must not change its
     * Bluetooth connection or opt-in control surface mode. */
    project_stage.set.bt = (uint8_t)(((project_stage.set.bt & ~PROJ_DEVICE_BT) | (proj.set.bt & PROJ_DEVICE_BT)) & ~PROJ_BT_SMC_USER);
    __builtin_memcpy(&proj, &project_stage, sizeof proj);
    project_incompatible = 0; /* explicitly validated import/reset is the recovery boundary */
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
        get32(h + 20) != PROJ_STREAM ||
        get32(h + 24) != PROJ_SND_REV || get32(h + 28))
        return -1;
    if (get32(h + 12) != crc32_of((const uint8_t *)&project_stage, sizeof project_stage))
        return -4;
    if(!stream_layout_ok(&project_stage,get32(h+20)))return -9;
#if !X0X_ORIGINAL_SYNTH
    migrate_break_counts(&project_stage, get32(h+20));
#endif
    return project_validate_stage();
}

void project_apply_io(void)
{
    int p;
    seq.send_clock = proj.set.clk_out;
    seq.send_notes = proj.set.notes_out;
    seq.clk_src = PROJ_CLK_SRC(proj.set.bt);
    for (p = 0; p < 5; p++) {
        int v = proj.set.midi_ch[p];
        seq.ch[p] = (uint8_t)(!v ? SEQ_CH_DEFAULT[p] : v > 16 ? (int)SEQ_CH_OFF : v - 1);
    }
    engine_out_loud(proj.set.out_loud);
    engine_usb_full(proj.set.usb_full);
}

static void capture_settings(settings_t *set)
{
    int p;
    set->bt &= (uint8_t)~PROJ_BT_SMC_USER; /* ordinary save/export only; no forced migration write */
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
#if X0X_BASS_TAGGED_SOUND
    if(engine_bass_sound_validate(&proj.sound))return -9;
    for(int v=0;v<NBASS;v++)if(engine_bass_preset_pending(v))return -8;
#elif X0X_BASS_PRESET_PREVIEW
    if(engine_bass_preview_changed()) return -9; /* no untagged preview writes */
#endif
    if (project_incompatible || !stream_layout_ok(&proj, PROJ_STREAM))
        return -9; /* not one settings/sound/pattern object may be rewritten */
#ifndef X0X_HOST
    extern int store_stage_held(void);
    if (store_stage_held())                          /* boot upload is using the store buffer */
        return -8;
#endif
    capture_settings(&proj.set);
    if (measure_project(&proj, &packed_len))
        return -7;
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set))
        return -1;
    if (save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound))
        return -2;
    return save_stream();
}
