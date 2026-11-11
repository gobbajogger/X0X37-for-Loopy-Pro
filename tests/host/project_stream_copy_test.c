/* SPDX-License-Identifier: GPL-3.0-only */
/* Bit-identical project streams. st_put_ref / measure_ref / save_stream_ref are the
 * parent functions with the put renamed so the old byte loop and the chunk copy can
 * run on the same buffer one after the other. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/project.c"

seq_t seq;
static uint8_t flash_obj[OBJ_NOBJ][PLAT_STORE_MAX];
static uint32_t flash_len[OBJ_NOBJ];
static uint8_t flash_seen[OBJ_NOBJ];
static unsigned writes;

int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    if (obj >= OBJ_NOBJ || len > PLAT_STORE_MAX)
        return -1;
    memcpy(flash_obj[obj], src, len);
    flash_len[obj] = len;
    flash_seen[obj] = 1;
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

/* Parent st_put: one byte each time. */
static int st_put_ref(void *ctx, const uint8_t *p, uint32_t n)
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

/* New copy with the last byte flipped. The comparison must reject it. */
static int st_put_mutant(void *ctx, const uint8_t *p, uint32_t n)
{
    st_out_t *o = ctx;
    o->total += n;
    if (o->measure)
        return o->total > STREAM_CAP ? -1 : 0;
    while (n) {
        uint32_t take;
        if (o->fill == PLAT_STORE_MAX) {
            if (o->k + 1 >= NSTREAM || st_flush(o)) {
                o->err = 1;
                return -1;
            }
        }
        take = PLAT_STORE_MAX - o->fill;
        if (take > n)
            take = n;
        __builtin_memcpy(io_buf + o->fill, p, take);
        if (take)
            io_buf[o->fill + take - 1] ^= 0xFFu;
        o->fill += take;
        p += take;
        n -= take;
    }
    return 0;
}

static int measure_ref(const project_t *pr, uint32_t *length)
{
    const uint8_t *raw = (const uint8_t *)pr + RAW_OFF;
    st_out_t o = {1, 0, 0, 0, 0, PROJ_STREAM};
    lz_sink_t sink = {st_put_ref, &o};
    if (lz_compress(raw, RAW_LEN, &sink))
        return -7;
    *length = o.total;
    return 0;
}

static int save_stream_ref(void)
{
    const uint8_t *raw = (const uint8_t *)&proj + RAW_OFF;
    uint32_t crc = crc32_of(raw, RAW_LEN);
    st_out_t o = {0, 0, 0, 0, 0, PROJ_STREAM};
    lz_sink_t sink = {st_put_ref, &o};
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

#define PAY0 (PLAT_STORE_MAX - STREAM_HDR)
#define PAYN (PLAT_STORE_MAX - 4u)

static int mutant;
static int fails;
static unsigned chunk_cases;
static unsigned projects;
static uint8_t raw_base[RAW_LEN];
static uint8_t blob[20000];

typedef struct {
    int rc, k, err;
    unsigned writes;
    uint32_t fill, total;
    uint8_t seen[OBJ_NOBJ];
    uint32_t len[OBJ_NOBJ];
    uint8_t obj[OBJ_NOBJ][PLAT_STORE_MAX];
    uint8_t buf[PLAT_STORE_MAX];
} snap_t;

static snap_t snap_a, snap_b;

typedef int (*put_fn)(void *, const uint8_t *, uint32_t);

static void clear_store(void)
{
    memset(stored_known, 0, sizeof stored_known);
    memset(stored_hash, 0, sizeof stored_hash);
    memset(flash_seen, 0, sizeof flash_seen);
    memset(flash_len, 0, sizeof flash_len);
    writes = 0;
}

static void take_snap(snap_t *s, int rc, const st_out_t *o)
{
    unsigned i;
    memset(s, 0, sizeof *s);
    s->rc = rc;
    s->k = o ? o->k : 0;
    s->err = o ? o->err : 0;
    s->fill = o ? o->fill : 0;
    s->total = o ? o->total : 0;
    s->writes = writes;
    memcpy(s->seen, flash_seen, sizeof flash_seen);
    memcpy(s->len, flash_len, sizeof flash_len);
    for (i = 0; i < OBJ_NOBJ; i++)
        if (flash_seen[i] && flash_len[i])
            memcpy(s->obj[i], flash_obj[i], flash_len[i]);
    memcpy(s->buf, io_buf, sizeof io_buf);
}

static int same_snap(const char *what)
{
    unsigned i;
    if (snap_a.rc != snap_b.rc || snap_a.k != snap_b.k || snap_a.err != snap_b.err ||
        snap_a.fill != snap_b.fill || snap_a.total != snap_b.total || snap_a.writes != snap_b.writes ||
        memcmp(snap_a.seen, snap_b.seen, sizeof snap_a.seen) ||
        memcmp(snap_a.len, snap_b.len, sizeof snap_a.len) ||
        memcmp(snap_a.buf, snap_b.buf, sizeof snap_a.buf)) {
        if (!fails)
            printf("mismatch %s rc %d/%d k %d/%d err %d/%d fill %u/%u total %u/%u writes %u/%u\n",
                   what, snap_a.rc, snap_b.rc, snap_a.k, snap_b.k, snap_a.err, snap_b.err,
                   snap_a.fill, snap_b.fill, snap_a.total, snap_b.total, snap_a.writes, snap_b.writes);
        fails++;
        return 0;
    }
    for (i = 0; i < OBJ_NOBJ; i++) {
        if (!snap_a.seen[i])
            continue;
        if (memcmp(snap_a.obj[i], snap_b.obj[i], snap_a.len[i])) {
            if (!fails)
                printf("mismatch %s object %u len %u\n", what, i, snap_a.len[i]);
            fails++;
            return 0;
        }
    }
    return 1;
}

static void prep_buf(uint32_t fill)
{
    uint32_t i;
    for (i = 0; i < PLAT_STORE_MAX; i++)
        io_buf[i] = (uint8_t)(0xA5u ^ i);
    if (fill >= 4u)
        put32(io_buf, PROJ_STREAM);
}

static int run_put(put_fn put, uint32_t fill0, int k0, int measure,
                   const uint32_t *lens, int nlens, snap_t *out)
{
    st_out_t o;
    uint32_t off = 0;
    int i, rc = 0;
    clear_store();
    prep_buf(fill0);
    memset(&o, 0, sizeof o);
    o.measure = measure;
    o.k = k0;
    o.fill = fill0;
    for (i = 0; i < nlens; i++) {
        rc = put(&o, blob + off, lens[i]);
        off += lens[i];
        if (rc)
            break;
    }
    take_snap(out, rc, &o);
    return rc;
}

static int compare_put(const char *what, put_fn neu, uint32_t fill0, int k0, int measure,
                       const uint32_t *lens, int nlens)
{
    if (fails)
        return 0;
    chunk_cases++;
    run_put(st_put_ref, fill0, k0, measure, lens, nlens, &snap_a);
    run_put(neu, fill0, k0, measure, lens, nlens, &snap_b);
    return same_snap(what);
}

static int chunk_suite(put_fn neu)
{
    static const uint32_t fills[] = {4u, 16u, PAYN, PAY0, PLAT_STORE_MAX - 1u, PLAT_STORE_MAX - 3u,
                                     PLAT_STORE_MAX - 128u, PLAT_STORE_MAX};
    static const uint32_t sizes[] = {
        0u, 1u, 2u, 3u, 4u, 5u, 7u, 15u, 16u, 17u, 127u, 128u, 129u, 255u, 511u, 512u, 1000u,
        PAY0 - 1u, PAY0, PAY0 + 1u, PAYN - 1u, PAYN, PAYN + 1u, 4000u, 8000u,
        PAY0 + PAYN, PAY0 + PAYN + 1u, PAY0 + 2u * PAYN, PAY0 + 3u * PAYN,
        STREAM_CAP - 1u, STREAM_CAP, STREAM_CAP + 1u, 16000u
    };
    unsigned i, f;
    uint32_t seq_l[4000];
    uint32_t mix[8];
    char what[64];
    for (i = 0; i < sizeof blob; i++)
        blob[i] = (uint8_t)(i * 13u + 7u);
    for (f = 0; f < sizeof fills / sizeof fills[0] && !fails; f++) {
        for (i = 0; i < sizeof sizes / sizeof sizes[0] && !fails; i++) {
            uint32_t n = sizes[i];
            if (n > sizeof blob)
                continue;
            snprintf(what, sizeof what, "put fill %u n %u", fills[f], n);
            compare_put(what, neu, fills[f], 0, 0, &n, 1);
        }
    }
    for (i = 0; i < 4000u && !fails; i++)
        seq_l[i] = 1u;
    compare_put("4000 singles", neu, STREAM_HDR, 0, 0, seq_l, 4000);
    for (i = 0; i < 40u && !fails; i++)
        seq_l[i] = 128u;
    compare_put("40x128", neu, STREAM_HDR, 0, 0, seq_l, 40);
    mix[0] = 1u;
    mix[1] = 3u;
    mix[2] = 128u;
    mix[3] = PAY0 - 10u;
    mix[4] = 7u;
    mix[5] = 4u;
    mix[6] = 2u;
    mix[7] = 1000u;
    compare_put("mixed chunks", neu, STREAM_HDR, 0, 0, mix, 8);
    {
        uint32_t n = 20u;
        compare_put("last object overflow", neu, PLAT_STORE_MAX - 5u, NSTREAM - 1, 0, &n, 1);
    }
    {
        uint32_t ns[] = {0u, 1u, STREAM_CAP - 1u, STREAM_CAP, STREAM_CAP + 1u, 16000u};
        for (i = 0; i < sizeof ns / sizeof ns[0] && !fails; i++) {
            snprintf(what, sizeof what, "measure %u", ns[i]);
            compare_put(what, neu, STREAM_HDR, 0, 1, &ns[i], 1);
        }
    }
    if (fails)
        return 0;
    /* The old loop's boundary shape, so a shared mistake cannot pass quietly. */
    {
        uint32_t         n = PAY0;
        run_put(st_put_ref, STREAM_HDR, 0, 0, &n, 1, &snap_a);
        if (snap_a.rc || snap_a.k || snap_a.fill != PLAT_STORE_MAX || snap_a.writes) {
            printf("byte loop did not fill the first object exactly (rc %d k %d fill %u writes %u)\n",
                   snap_a.rc, snap_a.k, snap_a.fill, snap_a.writes);
            fails++;
            return 0;
        }
        n = PAY0 + 1u;
        run_put(st_put_ref, STREAM_HDR, 0, 0, &n, 1, &snap_a);
        if (snap_a.rc || snap_a.k != 1 || snap_a.fill != 5u || snap_a.writes != 1u ||
            snap_a.len[STREAM_OBJ[0]] != PLAT_STORE_MAX) {
            printf("byte loop did not cross into the second object (rc %d k %d fill %u writes %u)\n",
                   snap_a.rc, snap_a.k, snap_a.fill, snap_a.writes);
            fails++;
            return 0;
        }
        n = STREAM_CAP;
        run_put(st_put_ref, STREAM_HDR, 0, 0, &n, 1, &snap_a);
        if (snap_a.rc || snap_a.k != NSTREAM - 1 || snap_a.fill != PLAT_STORE_MAX || snap_a.writes != 3u) {
            printf("byte loop did not fill the stream exactly (rc %d k %d fill %u writes %u)\n",
                   snap_a.rc, snap_a.k, snap_a.fill, snap_a.writes);
            fails++;
            return 0;
        }
        n = STREAM_CAP + 1u;
        run_put(st_put_ref, STREAM_HDR, 0, 0, &n, 1, &snap_a);
        if (snap_a.rc != -1 || snap_a.err != 1 || snap_a.writes != 3u) {
            printf("byte loop did not reject one byte past capacity (rc %d err %d writes %u)\n",
                   snap_a.rc, snap_a.err, snap_a.writes);
            fails++;
            return 0;
        }
    }
    return 1;
}

static void load_prefix(uint32_t seed, uint32_t n)
{
    uint8_t *raw = (uint8_t *)&proj + RAW_OFF;
    uint32_t i, s = seed ? seed : 1u;
    memcpy(raw, raw_base, RAW_LEN);
    for (i = 0; i < n && i < RAW_LEN; i++) {
        s = s * 1664525u + 1013904223u;
        raw[i] = (uint8_t)(s >> 16);
    }
}

static int measure_both(uint32_t *plen)
{
    uint32_t a = 0, b = 0;
    int ra = measure_ref(&proj, &a);
    int rb = measure_project(&proj, &b);
    if (ra != rb || (ra == 0 && a != b)) {
        if (!fails)
            printf("measure ref/new %d/%d plen %u/%u\n", ra, rb, a, b);
        fails++;
        return -1;
    }
    if (ra == 0)
        *plen = a;
    return ra;
}

static int save_both(const char *what)
{
    st_out_t dummy = {0, 0, 0, 0, 0, PROJ_STREAM};
    int ra, rb;
    clear_store();
    memset(io_buf, 0x5A, sizeof io_buf);
    ra = save_stream_ref();
    take_snap(&snap_a, ra, &dummy);
    snap_a.k = NSTREAM;
    clear_store();
    memset(io_buf, 0x5A, sizeof io_buf);
    rb = save_stream();
    take_snap(&snap_b, rb, &dummy);
    snap_b.k = NSTREAM;
    projects++;
    return same_snap(what);
}

static int shape_ok(uint32_t plen)
{
    uint32_t lens[4];
    unsigned i;
    for (i = 0; i < 4u; i++)
        lens[i] = 4u;
    if (plen <= PAY0) {
        lens[0] = STREAM_HDR + plen;
    } else if (plen <= PAY0 + PAYN) {
        lens[0] = PLAT_STORE_MAX;
        lens[1] = 4u + (plen - PAY0);
    } else if (plen <= PAY0 + 2u * PAYN) {
        lens[0] = lens[1] = PLAT_STORE_MAX;
        lens[2] = 4u + (plen - PAY0 - PAYN);
    } else {
        lens[0] = lens[1] = lens[2] = PLAT_STORE_MAX;
        lens[3] = 4u + (plen - PAY0 - 2u * PAYN);
    }
    for (i = 0; i < 4u; i++) {
        if (!snap_b.seen[STREAM_OBJ[i]] || snap_b.len[STREAM_OBJ[i]] != lens[i]) {
            if (!fails)
                printf("boundary plen %u obj %u len %u want %u seen %u\n",
                       plen, i, snap_b.seen[STREAM_OBJ[i]] ? snap_b.len[STREAM_OBJ[i]] : 0u,
                       lens[i], snap_b.seen[STREAM_OBJ[i]]);
            fails++;
            return 0;
        }
    }
    return snap_b.rc == 0 && snap_b.writes == 4u;
}

static int find_plen(uint32_t target, uint32_t *seed_out, uint32_t *n_out)
{
    uint32_t seed;
    for (seed = 1; seed <= 8u; seed++) {
        uint32_t lo = 0, hi = RAW_LEN, n, plen = 0;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo) / 2u;
            load_prefix(seed, mid);
            if (measure_project(&proj, &plen) || plen >= target)
                hi = mid;
            else
                lo = mid + 1u;
        }
        {
            uint32_t start = lo > 400u ? lo - 400u : 0u;
            uint32_t end = lo + 400u < RAW_LEN ? lo + 400u : RAW_LEN;
            for (n = start; n <= end; n++) {
                load_prefix(seed, n);
                plen = 0;
                if (!measure_project(&proj, &plen) && plen == target) {
                    *seed_out = seed;
                    *n_out = n;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int project_suite(void)
{
    static const uint32_t targets[] = {
        PAY0 - 1u, PAY0, PAY0 + 1u,
        PAY0 + PAYN - 1u, PAY0 + PAYN, PAY0 + PAYN + 1u,
        PAY0 + 2u * PAYN - 1u, PAY0 + 2u * PAYN, PAY0 + 2u * PAYN + 1u,
        STREAM_CAP - 1u, STREAM_CAP
    };
    unsigned i;
    uint32_t plen = 0;
    project_defaults();
    memcpy(raw_base, (uint8_t *)&proj + RAW_OFF, RAW_LEN);
    if (measure_both(&plen))
        return 0;
    packed_len = plen;
    if (!save_both("default project"))
        return 0;
    proj.pat[0].bass[0].step[0].note = 48;
    proj.pat[0].bass[0].step[0].flags = G_NOTE;
    proj.pat[0].bass[0].step[0].velocity = 100;
    proj.pat[1].bass[1].step[3].note = 36;
    proj.pat[1].bass[1].step[3].flags = G_NOTE;
    proj.pat[2].bass[0].step[8].note = 43;
    proj.pat[2].bass[0].step[8].flags = G_TIE;
    if (measure_both(&plen))
        return 0;
    packed_len = plen;
    if (!save_both("edited notes"))
        return 0;
    for (i = 0; i < sizeof targets / sizeof targets[0] && !fails; i++) {
        uint32_t seed = 0, n = 0;
        char what[48];
        if (!find_plen(targets[i], &seed, &n)) {
            printf("no project with packed length %u (raw %u)\n", targets[i], (unsigned)RAW_LEN);
            fails++;
            return 0;
        }
        load_prefix(seed, n);
        if (measure_both(&plen) || plen != targets[i])
            return 0;
        packed_len = plen;
        snprintf(what, sizeof what, "packed %u", plen);
        if (!save_both(what) || !shape_ok(plen))
            return 0;
    }
    {
        uint32_t lo = 0, hi = RAW_LEN, nfail = 0, plen_ok = 0;
        int saw = 0;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo) / 2u;
            load_prefix(1u, mid);
            if (measure_project(&proj, &plen_ok)) {
                saw = 1;
                hi = mid;
            } else
                lo = mid + 1u;
        }
        if (!saw || lo == 0u) {
            puts("incompressible prefix never reached stream capacity");
            fails++;
            return 0;
        }
        nfail = lo;
        load_prefix(1u, nfail - 1u);
        if (measure_both(&plen))
            return 0;
        packed_len = plen;
        if (!save_both("just under capacity") || !shape_ok(plen))
            return 0;
        load_prefix(1u, nfail);
        if (measure_both(&plen) != -7)
            return 0;
        packed_len = 0;
        if (!save_both("over capacity"))
            return 0;
        if (snap_b.rc != -3 || snap_b.writes != 3u) {
            printf("over capacity rc %d writes %u\n", snap_b.rc, snap_b.writes);
            fails++;
            return 0;
        }
    }
    return fails == 0;
}

int main(int argc, char **argv)
{
    uint32_t n = 48u;
    mutant = argc > 1 && argv[1] && !strcmp(argv[1], "mutant");
    if ((unsigned)PAY0 != PLAT_STORE_MAX - 16u || STREAM_CAP != PAY0 + 3u * PAYN) {
        puts("RED project-stream-copy: capacity constants moved");
        return 1;
    }
    if (mutant) {
        puts("MUTANT project-stream-copy");
        for (n = 0; n < sizeof blob; n++)
            blob[n] = (uint8_t)(n * 13u + 7u);
        n = 48u;
        run_put(st_put_ref, STREAM_HDR, 0, 0, &n, 1, &snap_a);
        run_put(st_put_mutant, STREAM_HDR, 0, 0, &n, 1, &snap_b);
        if (!same_snap("mutant")) {
            puts("RED project-stream-copy: mutant bytes differ from the byte copy");
            return 1;
        }
        puts("RED project-stream-copy: mutant matched the byte copy");
        return 1;
    }
    if (!chunk_suite(st_put) || !project_suite()) {
        printf("RED project-stream-copy: %d mismatches\n", fails ? fails : 1);
        return 1;
    }
    printf("GREEN project-stream-copy: bit-identical save_stream output (%u chunk cases, %u projects)\n",
           chunk_cases, projects);
    return 0;
}
