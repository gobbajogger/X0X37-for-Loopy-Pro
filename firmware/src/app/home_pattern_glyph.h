/* SPDX-License-Identifier: GPL-3.0-only
 * HOME mini-pattern: eight columns cover the entire stored part length.
 * The strongest sounding event in each bin determines its brightness.
 * Pure, read-only host-testable projection; never edits a project or audio state.
 */
#pragma once
#include <stdint.h>
#include "../seq/pattern.h"
#include "../seq/motion.h"

static inline uint8_t home_glyph_max(uint8_t a, uint8_t b) { return a > b ? a : b; }
static inline unsigned home_glyph_length(uint8_t stored)
{
    return stored >= 1u && stored <= NSTEPS ? stored : 16u;
}
/* Brightness is a sixteenth of the part colour, with a visible floor.
 * Current and queued selection remain distinct through the existing border. */
static inline uint8_t home_glyph_brightness(uint8_t velocity, int current)
{
    unsigned t;
    if (!velocity) return 0;
    t = 4u + ((unsigned)velocity * 12u + 63u) / 127u;
    return (uint8_t)(current ? t : (t * 9u + 8u) / 16u);
}
static inline void home_glyph_put(uint8_t out[8], unsigned step, unsigned length, uint8_t vel)
{
    unsigned bin = step * 8u / length;
    if (bin > 7u) bin = 7u;
    out[bin] = home_glyph_max(out[bin], vel);
}

/* part 0/1 = 909/808, 2/3 = bass, 4 = BREAK (saved physical IDs).
 * `vel_target` is the runtime T_VEL pseudo-target, supplied by the caller.
 * A missing per-step drum velocity uses the saved global accent law.
 * `lanes` may be NULL for an unsaved/no-automation host preview. */
static inline void home_pattern_strength8(const pattern_t *pt, const lane_t *lanes,
                                           int pattern, int part, uint8_t unaccented,
                                           int vel_target, uint8_t out[8])
{
    unsigned s, v, len;
    for (s = 0; s < 8u; s++) out[s] = 0;
    if (!pt || part < 0 || part >= NKIT + NBASS + 1) return;
    if (part < NKIT) {
        const dpart_t *d = &pt->drum[part];
        const lane_t *vel_lane[NDRUM] = {0};
        len = home_glyph_length(d->len);
        if (lanes && pattern >= 0 && pattern < NPAT) {
            unsigned k;
            for (k = 0; k < NLANE; k++) {
                const lane_t *l = &lanes[k];
                unsigned voice;
                if (!l->used || l->pat != pattern || l->part != part ||
                    l->t != vel_target || l->i != 0) continue;
                voice = l->v & 15u;
                if ((l->v >> 4) == (unsigned)part && voice < NDRUM && !vel_lane[voice])
                    vel_lane[voice] = l; /* match motion_find's first matching lane */
            }
        }
        for (v = 0; v < NDRUM; v++) for (s = 0; s < len; s++) {
            uint8_t strength;
            if (!sm_get(d->hit[v], (int)s)) continue;
            strength = sm_get(d->accent, (int)s) ? 127u : (unaccented ? unaccented : 1u);
            if (vel_lane[v] && vel_lane[v]->val[s] != MOT_NONE &&
                vel_lane[v]->val[s] >= 1u && vel_lane[v]->val[s] <= 127u)
                strength = vel_lane[v]->val[s];
            home_glyph_put(out, s, len, strength);
        }
        return;
    }
    if (part == NKIT + NBASS) {
        for (s = 0; s < 16u; s++)
            if ((pt->brk.steps >> s) & 1u) home_glyph_put(out, s, 16u, 127u);
        return;
    }
    {
        const bpart_t *bp = &pt->bass[part - NKIT];
        uint8_t held = 0;
        len = home_glyph_length(bp->len);
        /* A wrapped leading TIE may inherit the last NOTE, but an all-TIE
         * or orphaned-TIE pattern must remain silent. */
        if (bstep_gate(&bp->step[0]) == G_TIE) {
            int j = (int)len - 1;
            while (j >= 0 && bstep_gate(&bp->step[j]) == G_TIE) j--;
            if (j >= 0 && bstep_gate(&bp->step[j]) == G_NOTE)
                held = bstep_velocity(&bp->step[j]);
        }
        for (s = 0; s < len; s++) {
            int gate = bstep_gate(&bp->step[s]);
            if (gate == G_NOTE) held = bstep_velocity(&bp->step[s]);
            else if (gate != G_TIE) held = 0;
            if (held) home_glyph_put(out, s, len, held);
        }
    }
}
