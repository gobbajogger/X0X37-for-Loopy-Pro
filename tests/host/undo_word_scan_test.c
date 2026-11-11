/* SPDX-License-Identifier: GPL-3.0-only */
/* Bit-identical undo entries: the word skip versus the previous byte-only scan.
 * undo_commit_ref is the parent function, copied verbatim. The mutant skips a
 * differing word and must not match that reference. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/undo.c"

static int undo_commit_ref(undo_t *u)
{
    uint32_t i = 0, sz = 8, runs = 0, lo = 0, hi = 0;
    uint32_t written_runs = 0;
    uint8_t *e;
    /* first pass: the runs and the entry's size */
    while (i < u->len) {
        uint32_t s, end, q;
        if (u->region[i] == u->shadow[i]) {
            i++;
            continue;
        }
        s = end = i;
        while (end + 1u < u->len) {                       /* extend over short gaps */
            uint32_t nxt = end + 1u, lim = end + 1u + UNDO_GAP, found = 0;
            for (q = nxt; q < u->len && q < lim; q++)
                if (u->region[q] != u->shadow[q]) {
                    found = q;
                    break;
                }
            if (!found)
                break;
            end = found;
        }
        if (!runs)
            lo = s;
        hi = end;
        runs++;
        sz += 4u + ((end - s + 1u + 1u) & ~1u);
        i = end + 1u;
    }
    if (!runs)
        return 0;
    u->n = u->cur;                                        /* what was undone is gone */
    u->used = u->at[u->n];
    if (sz > UNDO_POOL) {
        undo_reset(u);
        return -1;
    }
    while (u->n > 0 && (u->n >= UNDO_MAX || u->used + sz > UNDO_POOL))
        ud_drop_oldest(u);
    e = u->pool + u->used;
    ud_put16(e, runs);
    ud_put16(e + 2, lo);
    ud_put16(e + 4, hi);
    ud_put16(e + 6, 0);
    e += 8;
    /* second pass, the same runs: the previous bytes into the entry, the shadow brought up to date */
    i = lo;
    while (i <= hi) {
        uint32_t s, end, q, k;
        if (u->region[i] == u->shadow[i]) {
            i++;
            continue;
        }
        s = end = i;
        while (end + 1u < u->len) {
            uint32_t nxt = end + 1u, lim = end + 1u + UNDO_GAP, found = 0;
            for (q = nxt; q < u->len && q < lim; q++)
                if (u->region[q] != u->shadow[q]) {
                    found = q;
                    break;
                }
            if (!found)
                break;
            end = found;
        }
        /* A concurrent project writer could change the run shape after the
         * sizing pass. Never write beyond the reserved entry in that case. */
        uint32_t n = end - s + 1u;
        if ((uint32_t)(e - (u->pool + u->used)) + 4u + ((n + 1u) & ~1u) > sz) {
            undo_reset(u);
            return -1;
        }
        ud_put16(e, s);
        ud_put16(e + 2, n);
        e += 4;
        for (k = s; k <= end; k++) {
            *e++ = u->shadow[k];
            u->shadow[k] = u->region[k];
        }
        if ((end - s + 1u) & 1u)
            *e++ = 0;
        written_runs++;
        i = end + 1u;
    }
    if (written_runs != runs || (uint32_t)(e - (u->pool + u->used)) != sz) {
        undo_reset(u);
        return -1;
    }
    u->used += sz;
    u->n++;
    u->cur = u->n;
    u->at[u->n] = u->used;
    return 1;
}

static int undo_commit_mutant(undo_t *u)
{
    uint32_t i = 0, sz = 8, runs = 0, lo = 0, hi = 0;
    uint32_t written_runs = 0;
    uint8_t *e;
    /* first pass: the runs and the entry's size */
    while (i < u->len) {
        uint32_t s, end, q;
        /* Skip equal words without alignment or aliasing assumptions. On a
         * mismatch the byte path still finds the exact first changed byte. */
        while (u->len - i >= sizeof(uint32_t)) {
            uint32_t current, previous;
            __builtin_memcpy(&current, u->region + i, sizeof current);
            __builtin_memcpy(&previous, u->shadow + i, sizeof previous);
            if (current != previous) {
                i += sizeof current; /* mutant: drop the differing word */
                continue;
            }
            i += sizeof current;
        }
        if (i == u->len)
            break;
        if (u->region[i] == u->shadow[i]) {
            i++;
            continue;
        }
        s = end = i;
        while (end + 1u < u->len) {                       /* extend over short gaps */
            uint32_t nxt = end + 1u, lim = end + 1u + UNDO_GAP, found = 0;
            for (q = nxt; q < u->len && q < lim; q++)
                if (u->region[q] != u->shadow[q]) {
                    found = q;
                    break;
                }
            if (!found)
                break;
            end = found;
        }
        if (!runs)
            lo = s;
        hi = end;
        runs++;
        sz += 4u + ((end - s + 1u + 1u) & ~1u);
        i = end + 1u;
    }
    if (!runs)
        return 0;
    u->n = u->cur;                                        /* what was undone is gone */
    u->used = u->at[u->n];
    if (sz > UNDO_POOL) {
        undo_reset(u);
        return -1;
    }
    while (u->n > 0 && (u->n >= UNDO_MAX || u->used + sz > UNDO_POOL))
        ud_drop_oldest(u);
    e = u->pool + u->used;
    ud_put16(e, runs);
    ud_put16(e + 2, lo);
    ud_put16(e + 4, hi);
    ud_put16(e + 6, 0);
    e += 8;
    /* second pass, the same runs: the previous bytes into the entry, the shadow brought up to date */
    i = lo;
    while (i <= hi) {
        uint32_t s, end, q, k;
        if (u->region[i] == u->shadow[i]) {
            i++;
            continue;
        }
        s = end = i;
        while (end + 1u < u->len) {
            uint32_t nxt = end + 1u, lim = end + 1u + UNDO_GAP, found = 0;
            for (q = nxt; q < u->len && q < lim; q++)
                if (u->region[q] != u->shadow[q]) {
                    found = q;
                    break;
                }
            if (!found)
                break;
            end = found;
        }
        /* A concurrent project writer could change the run shape after the
         * sizing pass. Never write beyond the reserved entry in that case. */
        uint32_t n = end - s + 1u;
        if ((uint32_t)(e - (u->pool + u->used)) + 4u + ((n + 1u) & ~1u) > sz) {
            undo_reset(u);
            return -1;
        }
        ud_put16(e, s);
        ud_put16(e + 2, n);
        e += 4;
        for (k = s; k <= end; k++) {
            *e++ = u->shadow[k];
            u->shadow[k] = u->region[k];
        }
        if ((end - s + 1u) & 1u)
            *e++ = 0;
        written_runs++;
        i = end + 1u;
    }
    if (written_runs != runs || (uint32_t)(e - (u->pool + u->used)) != sz) {
        undo_reset(u);
        return -1;
    }
    u->used += sz;
    u->n++;
    u->cur = u->n;
    u->at[u->n] = u->used;
    return 1;
}

static int mutant;
static int fails;
static int stop;
static unsigned cases;
static int saw_unaligned;
static int saw_mod[4];
#define MAX_LEN 4200u
static uint8_t reg_n[MAX_LEN], reg_r[MAX_LEN], sh_n[MAX_LEN], sh_r[MAX_LEN];
static uint8_t src_r[MAX_LEN], src_s[MAX_LEN];
static undo_t un, ur;

static uint32_t rng = 1u;
static uint32_t rnd(void)
{
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

static void arm(undo_t *u, uint8_t *region, uint8_t *shadow, uint32_t len)
{
    memset(u, 0, sizeof *u);
    u->region = region;
    u->shadow = shadow;
    u->len = len;
}

static int commit_new(undo_t *u)
{
    return mutant ? undo_commit_mutant(u) : undo_commit(u);
}

static void note_len(uint32_t len)
{
    if (len % 4u)
        saw_unaligned = 1;
}

static int same_state(const char *what, int ra, int rb, uint32_t len)
{
    int k;
    cases++;
    note_len(len);
    if (ra == rb && un.n == ur.n && un.cur == ur.cur && un.used == ur.used &&
        (len == 0u || (!memcmp(un.region, ur.region, len) && !memcmp(un.shadow, ur.shadow, len))) &&
        (un.used == 0u || !memcmp(un.pool, ur.pool, un.used))) {
        for (k = 0; k <= un.n; k++)
            if (un.at[k] != ur.at[k])
                break;
        if (k > un.n)
            return 1;
    }
    if (!fails)
        printf("mismatch %s rc %d/%d n %d/%d cur %d/%d used %u/%u len %u\n",
               what, ra, rb, un.n, ur.n, un.cur, ur.cur, un.used, ur.used, len);
    fails++;
    stop = 1;
    return 0;
}

static void load(uint32_t len)
{
    if (len) {
        memcpy(reg_n, src_r, len);
        memcpy(reg_r, src_r, len);
        memcpy(sh_n, src_s, len);
        memcpy(sh_r, src_s, len);
    }
    arm(&un, reg_n, sh_n, len);
    arm(&ur, reg_r, sh_r, len);
}

static void one(const char *what, uint32_t len)
{
    int ra, rb;
    if (stop)
        return;
    load(len);
    ra = commit_new(&un);
    rb = undo_commit_ref(&ur);
    same_state(what, ra, rb, len);
}

static void fill_pat(uint8_t *p, uint32_t len, uint32_t seed)
{
    uint32_t i, s = seed ? seed : 1u;
    for (i = 0; i < len; i++) {
        s = s * 1664525u + 1013904223u;
        p[i] = (uint8_t)(s >> 16);
    }
}

static void single_at(uint32_t len, uint32_t pos)
{
    char what[64];
    if (pos >= len)
        return;
    fill_pat(src_r, len, 3u + len);
    memcpy(src_s, src_r, len);
    src_r[pos] ^= 0x5Au;
    saw_mod[pos % 4u] = 1;
    snprintf(what, sizeof what, "single len %u pos %u", len, pos);
    one(what, len);
}

static void gap_pair(uint32_t len, uint32_t a, uint32_t sep)
{
    char what[64];
    if (a + sep >= len)
        return;
    fill_pat(src_r, len, 9u + sep);
    memcpy(src_s, src_r, len);
    src_r[a] ^= 0x11u;
    src_r[a + sep] ^= 0x22u;
    snprintf(what, sizeof what, "gap len %u at %u sep %u", len, a, sep);
    one(what, len);
}

static void random_regions(void)
{
    uint32_t n;
    for (n = 0; n < 240u && !stop; n++) {
        uint32_t len = rnd() % (MAX_LEN + 1u);
        uint32_t i, diffs;
        char what[48];
        if (n < 40u)
            len = n % 4u == 0u ? n : (n * 17u) % 257u;
        fill_pat(src_r, len, rnd() | 1u);
        if (len)
            memcpy(src_s, src_r, len);
        diffs = len ? (rnd() % (len < 8u ? len + 1u : 8u)) : 0u;
        if ((n % 11u) == 0u)
            diffs = 0u; /* equal buffers */
        for (i = 0; i < diffs; i++) {
            uint32_t pos = rnd() % len;
            src_r[pos] ^= (uint8_t)(1u + (rnd() & 0x7Fu));
            saw_mod[pos % 4u] = 1;
        }
        if ((n % 5u) == 0u && len > 4u) {
            uint32_t pos = (rnd() % 4u) + (len - 1u - (len - 1u) % 4u);
            if (pos >= len)
                pos = len - 1u;
            src_r[pos] ^= 0x3Cu;
            saw_mod[pos % 4u] = 1;
        }
        snprintf(what, sizeof what, "random %u", n);
        one(what, len);
    }
}

static void edit_both(uint32_t len, uint32_t seed)
{
    uint32_t s = seed ? seed : 1u;
    uint32_t k, nedit;
    s = s * 1664525u + 1013904223u;
    nedit = 1u + (s % 4u);
    for (k = 0; k < nedit; k++) {
        uint32_t at, nb, j;
        s = s * 1664525u + 1013904223u;
        at = s % len;
        s = s * 1664525u + 1013904223u;
        nb = 1u + (s % 23u);
        for (j = 0; j < nb && at + j < len; j++) {
            s = s * 1664525u + 1013904223u;
            reg_n[at + j] = (uint8_t)s;
            reg_r[at + j] = (uint8_t)s;
        }
    }
}

static void history(uint32_t len, uint32_t steps)
{
    uint32_t step;
    if (stop)
        return;
    fill_pat(reg_n, len, 99u);
    memcpy(reg_r, reg_n, len);
    undo_init(&un, reg_n, sh_n, len);
    undo_init(&ur, reg_r, sh_r, len);
    for (step = 0; step < steps && !stop; step++) {
        char what[48];
        int ra, rb;
        edit_both(len, 1000u + step * 17u + len);
        ra = commit_new(&un);
        rb = undo_commit_ref(&ur);
        snprintf(what, sizeof what, "history len %u step %u", len, step);
        same_state(what, ra, rb, len);
        if ((step % 17u) == 0u && !stop) {
            ra = commit_new(&un);
            rb = undo_commit_ref(&ur);
            snprintf(what, sizeof what, "history idle %u", step);
            same_state(what, ra, rb, len);
        }
    }
}

int main(int argc, char **argv)
{
    static const uint32_t lens[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65,
        127, 128, 129, 255, 256, 257, 1000, 1001, 4080, 4081, 4083, 4085, 4088
    };
    unsigned li;
    mutant = argc > 1 && !strcmp(argv[1], "mutant");
    for (li = 0; li < sizeof lens / sizeof lens[0] && !stop; li++) {
        uint32_t len = lens[li], pos;
        fill_pat(src_r, len, 1u);
        if (len)
            memcpy(src_s, src_r, len);
        one("equal", len);
        if (!len)
            continue;
        for (pos = 0; pos < len && pos < 20u && !stop; pos++)
            single_at(len, pos);
        if (len > 24u) {
            for (pos = len - 20u; pos < len && !stop; pos++)
                single_at(len, pos);
        }
        for (pos = 0; pos + 9u < len && pos < 12u && !stop; pos++) {
            gap_pair(len, pos, 1u);
            gap_pair(len, pos, 8u);
            gap_pair(len, pos, 9u);
        }
    }
    random_regions();
    history(5u, 40u);
    history(7u, 40u);
    history(180u, 80u);
    history(257u, 60u);
    if (fails) {
        if (mutant)
            puts("MUTANT undo-word-scan");
        printf("RED undo-word-scan: %s%d mismatches\n",
               mutant ? "mutant entry differs from the byte scan " : "", fails);
        return 1;
    }
    if (mutant) {
        puts("MUTANT undo-word-scan");
        puts("RED undo-word-scan: mutant matched the byte scan");
        return 1;
    }
    if (!saw_unaligned || !saw_mod[0] || !saw_mod[1] || !saw_mod[2] || !saw_mod[3]) {
        printf("RED undo-word-scan: coverage unaligned %d mods %d %d %d %d\n",
               saw_unaligned, saw_mod[0], saw_mod[1], saw_mod[2], saw_mod[3]);
        return 1;
    }
    printf("GREEN undo-word-scan: bit-identical undo entries (%u cases)\n", cases);
    return 0;
}
