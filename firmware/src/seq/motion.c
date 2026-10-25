/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X motion; see motion.h. */
#include "motion.h"

void motion_init(motion_t *m, lane_t *lanes)
{
    uint32_t i;
    uint8_t *z = (uint8_t *)m;
    for (i = 0; i < sizeof *m; i++)
        z[i] = 0;
    m->lane = lanes;
    for (i = 0; i < NLANE; i++)
        m->rt[i].applied = m->rt[i].rec_last = MOT_NONE;
}

static void put(motion_t *m, const lane_t *l, lane_rt_t *r, int v)
{
    if (r->applied != v) {
        m->apply(m->ctx, l->t, l->v, l->i, v);
        r->applied = (uint8_t)v;
    }
}

static void to_base(motion_t *m, const lane_t *l, lane_rt_t *r)
{
    if (r->applied != MOT_NONE)
        m->apply(m->ctx, l->t, l->v, l->i, m->base(m->ctx, l->t, l->v, l->i));
    r->applied = MOT_NONE;
    r->ramp = 0;
}

void motion_step(motion_t *m, int part, int pat, int step, int next, int pass)
{
    int k, phase;
    if (part < 0 || part >= MOT_PARTS || step < 0 || step >= NSTEPS)
        return;
    if (m->age[part])
        m->len[part][m->last[part] & 1u] = m->age[part];
    m->age[part] = 0;
    m->last[part] = (uint8_t)step;
    if (next < 0 || next >= NSTEPS)
        next = 0;
    /* Restore departing patterns first; a later slot must never overwrite
     * the active pattern for the same parameter with the base value. */
    for (phase = 0; phase < 2; phase++) for (k = 0; k < NLANE; k++) {
        lane_t *l = &m->lane[k];
        lane_rt_t *r = &m->rt[k];
        int v, nv;
        if (!l->used || l->t == MOT_SNAPSHOT || l->part != part)
            continue;
        if (r->gen != l->gen) {                        /* a reused slot: a fresh runtime */
            r->gen = l->gen;
            r->applied = MOT_NONE;
            r->ramp = 0;
            r->rec_last = MOT_NONE;
            r->hold_left = 0;
        }
        if (l->pat != pat) {                           /* not this part's pattern now: the knob */
            if (phase)
                continue;
            /* A queued knob event belongs to the departing pattern, not the next
             * time this lane's pattern happens to come around. */
            r->rec_req = r->hold_req = 0;
            to_base(m, l, r);
            r->rec_last = MOT_NONE;
            r->hold_left = 0;
            continue;
        }
        if (!phase)
            continue;
        if (r->hold_req) {
            r->hold_req = 0;
            r->hold_left = (uint8_t)pass;
        }
        if (r->rec_req) {                              /* the knob moved: this step takes its value */
            int now = m->base(m->ctx, l->t, l->v, l->i), d, j;
            r->rec_req = 0;
            if (pass < 1)
                pass = 1;
            if (r->rec_last != MOT_NONE) {             /* a short pause in the gesture: a straight line */
                int from = l->val[r->rec_last];
                d = (step - r->rec_last + pass) % pass;
                if (d >= 2 && d <= MOT_GAP && from != MOT_NONE)
                    for (j = 1; j < d; j++)
                        l->val[(r->rec_last + j) % pass] = (uint8_t)(from + (now - from) * j / d);
            }
            l->val[step] = (uint8_t)now;
            r->rec_last = (uint8_t)step;
            r->rec_quiet = 0;
            r->hold_left = 0;
            r->applied = MOT_NONE;
            r->ramp = 0;
            continue;
        }
        if (r->rec_last != MOT_NONE && ++r->rec_quiet > MOT_GAP)
            r->rec_last = MOT_NONE;                    /* the gesture is over */
        if (r->rec_last != MOT_NONE || r->hold_left) { /* the knob has it */
            if (r->hold_left)
                r->hold_left--;
            r->applied = MOT_NONE;
            r->ramp = 0;
            continue;
        }
        v = l->val[step];
        put(m, l, r, v == MOT_NONE ? m->base(m->ctx, l->t, l->v, l->i) : v);
        nv = l->val[next];
        r->ramp = (uint8_t)(v != MOT_NONE && nv != MOT_NONE && nv != v);
        r->from = (uint8_t)v;
        r->to = (uint8_t)nv;
        r->part_parity = (uint8_t)(step & 1);
    }
}

void motion_tick(motion_t *m, uint32_t n)
{
    int k, p;
    for (p = 0; p < MOT_PARTS; p++)
        m->age[p] += n;
    for (k = 0; k < NLANE; k++) {
        const lane_t *l = &m->lane[k];
        lane_rt_t *r = &m->rt[k];
        uint32_t L, a;
        if (!r->ramp || !l->used || l->t == MOT_SNAPSHOT || r->gen != l->gen || l->part >= MOT_PARTS)
            continue;
        L = m->len[l->part][r->part_parity];
        if (!L)
            continue;                                  /* the first step: no length yet */
        a = m->age[l->part];
        if (a > L)
            a = L;
        put(m, l, r, (int)r->from + ((int)r->to - (int)r->from) * (int)a / (int)L);
    }
}

void motion_release(motion_t *m)
{
    int k, p;
    for (k = 0; k < NLANE; k++) {
        lane_rt_t *r = &m->rt[k];
        if (m->lane[k].used && m->lane[k].t != MOT_SNAPSHOT && r->gen == m->lane[k].gen)
            to_base(m, &m->lane[k], r);
        r->rec_last = MOT_NONE;
        r->hold_left = 0;
        /* Stop/load boundary: discard requests queued by the preceding transport. */
        r->rec_req = 0;
        r->hold_req = 0;
    }
    for (p = 0; p < MOT_PARTS; p++)
        m->age[p] = m->len[p][0] = m->len[p][1] = 0;
}

int motion_value(const motion_t *m, int k)
{
    if (k < 0 || k >= NLANE || !m->lane[k].used || m->lane[k].t == MOT_SNAPSHOT || m->rt[k].gen != m->lane[k].gen)
        return -1;
    return m->rt[k].applied == MOT_NONE ? -1 : m->rt[k].applied;
}

int motion_find(const lane_t *L, int pat, int part, int t, int v, int i)
{
    int k;
    for (k = 0; k < NLANE; k++)
        if (L[k].used && L[k].pat == pat && L[k].part == part && L[k].t == t && L[k].v == v && L[k].i == i)
            return k;
    return -1;
}

int motion_alloc(lane_t *L, int pat, int part, int t, int v, int i)
{
    int k, s;
    for (k = 0; k < NLANE; k++)
        if (!L[k].used)
            break;
    if (k == NLANE)
        return -1;
    L[k].pat = (uint8_t)pat;
    L[k].part = (uint8_t)part;
    L[k].t = (uint8_t)t;
    L[k].v = (uint8_t)v;
    L[k].i = (uint8_t)i;
    for (s = 0; s < NSTEPS; s++)
        L[k].val[s] = MOT_NONE;
    L[k].gen++;
    __asm__ volatile("" ::: "memory");                 /* the fields before `used`: the ISR reads it first */
    L[k].used = 1;
    return k;
}

void motion_clear(motion_t *m, int k)
{
    if (k >= 0 && k < NLANE) {
        /* This slot may be reused before the next audio step.  Drop knob
         * intents from its former owner, not the fresh owner's first event. */
        m->lane[k].used = 0;
        m->rt[k].rec_req = 0;
        m->rt[k].hold_req = 0;
    }
}

int motion_count(const lane_t *L)
{
    int k, n = 0;
    for (k = 0; k < NLANE; k++)
        n += L[k].used != 0;
    return n;
}

void motion_req_rec(motion_t *m, int k)
{
    if (k >= 0 && k < NLANE)
        m->rt[k].rec_req = 1;
}

void motion_req_hold(motion_t *m, int k)
{
    if (k >= 0 && k < NLANE)
        m->rt[k].hold_req = 1;
}

static int snapshot_key(int pat, int part, int step)
{
    return (unsigned)pat < NPAT && (unsigned)(part - NKIT) < NBASS && (unsigned)step < NSTEPS;
}

int motion_snapshot_find(const lane_t *L, int pat, int part, int step)
{
    return L && snapshot_key(pat, part, step) ?
        motion_find(L, pat, part, MOT_SNAPSHOT, step, MOT_SNAPSHOT_SCHEMA) : -1;
}

const uint8_t *motion_snapshot_read(const lane_t *L, int pat, int part, int step)
{
    int k = motion_snapshot_find(L, pat, part, step);
    unsigned bank;
    if (k < 0) return 0;
    bank = *(const volatile uint8_t *)&L[k].val[NSTEPS - 1];
    __asm__ volatile("" ::: "memory");
    return bank == 0 || bank == MOT_SNAPSHOT_PARAMS ? L[k].val + bank : 0;
}

int motion_snapshot_engine(const lane_t *L, int pat, int part, int step)
{
    int k = motion_snapshot_find(L, pat, part, step);
    const uint8_t *v = motion_snapshot_read(L, pat, part, step);
    return v ? L[k].val[32 + (unsigned)(v - L[k].val) / MOT_SNAPSHOT_PARAMS] : 0;
}

int motion_snapshot_capture(lane_t *L, int pat, int part, int step, int engine, const uint8_t values[MOT_SNAPSHOT_PARAMS])
{
    int k, j, fresh = 0;
    unsigned bank;
    /* A local value copy also makes capture safe when values aliases this lane. */
    uint8_t copy[MOT_SNAPSHOT_PARAMS];
    if (!L || !values || !snapshot_key(pat, part, step) || (engine != 1 && engine != 2)) return -1;
    for (j = 0; j < MOT_SNAPSHOT_PARAMS; j++) {
        if (values[j] > 127) return -1;
        copy[j] = values[j];
    }
    k = motion_snapshot_find(L, pat, part, step);
    if (k < 0) {
        for (k = 0; k < NLANE && L[k].used; k++) { }
        if (k == NLANE) return -1;
        fresh = 1;
        L[k].pat = (uint8_t)pat; L[k].part = (uint8_t)part;
        L[k].t = MOT_SNAPSHOT; L[k].v = (uint8_t)step; L[k].i = MOT_SNAPSHOT_SCHEMA;
        L[k].rsv = 0;
        for (j = 0; j < NSTEPS; j++) L[k].val[j] = MOT_NONE;
        L[k].gen++;
        bank = 0;
    } else {
        bank = L[k].val[NSTEPS - 1];
        if (bank != 0 && bank != MOT_SNAPSHOT_PARAMS) return -1;
        bank ^= MOT_SNAPSHOT_PARAMS;
    }
    for (j = 0; j < MOT_SNAPSHOT_PARAMS; j++) L[k].val[bank + j] = copy[j];
    L[k].val[32 + bank / MOT_SNAPSHOT_PARAMS] = (uint8_t)engine;
    __asm__ volatile("" ::: "memory");
    *(volatile uint8_t *)&L[k].val[NSTEPS - 1] = (uint8_t)bank;
    __asm__ volatile("" ::: "memory");
    if (fresh) L[k].used = 1;
    return k;
}

int motion_snapshot_edit(lane_t *L, int pat, int part, int step, int param, int value)
{
    const uint8_t *old;
    uint8_t values[MOT_SNAPSHOT_PARAMS];
    int j;
    if ((unsigned)param >= MOT_SNAPSHOT_PARAMS || (unsigned)value > 127) return -1;
    old = motion_snapshot_read(L, pat, part, step);
    if (!old) return -1;
    for (j = 0; j < MOT_SNAPSHOT_PARAMS; j++) values[j] = old[j];
    values[param] = (uint8_t)value;
    return motion_snapshot_capture(L, pat, part, step, motion_snapshot_engine(L, pat, part, step), values);
}

void motion_snapshot_remove(lane_t *L, int pat, int part, int step)
{
    motion_clear(L, motion_snapshot_find(L, pat, part, step));
}
