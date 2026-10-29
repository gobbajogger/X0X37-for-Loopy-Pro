/* SPDX-License-Identifier: GPL-3.0-only */
/* Every SMC preset x mixer page: each fader and knob reaches the destination
 * that page shows, pickup takes over during a realistic sweep, and nothing
 * else changes. Production UI/SMC/engine; no device I/O. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"

static int preset_user;
static void mfader(int n, int v)
{
    if (preset_user) message(0xB0 | 15, 40 + n, v);
    else message(0xE0 + n, (v << 7) & 0x7F, v); /* 14-bit: LSB, MSB */
}
static void mknob(int n, int d)
{
    message(preset_user ? 0xB0 | 15 : 0xB0, 16 + n, d > 0 ? d : 64 - d);
}
static void tick(void) { now_ms += 20; smc_frame(); ui_frame(); }

typedef struct { const char *name; int view, page, part; } ctx_t;
static void enter(const ctx_t *c)
{
    setup(c->view, c->part);
    if (preset_user) proj.set.bt |= PROJ_BT_SMC_USER;
    if (c->page >= 0) { build_pages(); *page_of(c->view) = (uint8_t)c->page; build_pages(); }
    tick();
}
/* Every other destination of the main and drum mixers keeps its value. */
static int snapshot(int *v)
{
    int n = 0;
    for (int p = 0; p < NPARTS; p++) {
        v[n++] = pref_get(PR(R_ENG, T_MIX, p, MX_LEVEL));
        v[n++] = pref_get(PR(R_ENG, T_MIX, p, MX_CUT));
    }
    for (int k = 0; k < NKIT; k++)
        for (int t = 0; t < NDRUM; t++) {
            v[n++] = pref_get(find_ref(k ? T_808 : T_909, t, "Level"));
            v[n++] = pref_get(find_ref(k ? T_808 : T_909, t, "Pan"));
        }
    v[n++] = pref_get(PR(R_ENG, T_FX, 0, FX_RV_LEVEL));
    v[n++] = pref_get(PR(R_ENG, T_FX, 0, FX_DL_LEVEL));
    v[n++] = pref_get(PR(R_ENG, T_MST, 0, MST_DJF));
    v[n++] = pref_get(PR(R_ENG, T_FX, 0, FX_RV_DECAY));
    v[n++] = pref_get(PR(R_ENG, T_FX, 0, FX_DL_FDBK));
    v[n++] = pref_get(PR(R_ENG, T_MST, 0, MST_RESO));
    return n;
}
static void check_fader(const ctx_t *c, int n, pref_t want)
{
    int before[64], after[64], cnt;
    pref_t r = smc_target(n);
    const x0x_param_t *d;
    CHECK(smc_same(r, want));
    if (r.kind == R_NONE) return;
    d = pref_desc(r);
    pref_set(r, d->max / 2); tick();
    cnt = snapshot(before);
    for (int v = 0; v <= 127; v += 3) { mfader(n, v); tick(); }
    mfader(n, 127); tick();
    CHECK(smc_same(smc_target(n), want));
    if (pref_get(r) != d->max) fprintf(stderr, "%s %s fader %d: %d of %d\n", c->name, preset_user ? "USER" : "DAW", n, pref_get(r), d->max);
    CHECK(pref_get(r) == d->max);
    CHECK(smc.picked & (1u << n));
    /* Down again: an acquired fader follows every sample, including zero. */
    mfader(n, 0); tick();
    CHECK(pref_get(r) == 0);
    snapshot(after);
    pref_set(r, d->max / 2);
    after[0] = after[0];
    int diffs = 0;
    for (int i = 0; i < cnt; i++) diffs += before[i] != after[i];
    CHECK(diffs <= 1);                 /* only this fader's own destination */
}
static void check_knob(const ctx_t *c, int n, pref_t want)
{
    pref_t r = smc_knob_target(n);
    if (!smc_same(r, want))
        fprintf(stderr, "%s knob %d: kind %d a %d b %d c %d\n", c->name, n, r.kind, r.a, r.b, r.c);
    CHECK(smc_same(r, want));
    if (r.kind == R_NONE || r.kind == R_HDR || r.kind == R_ACT) return;
    const x0x_param_t *d = pref_desc(r);
    int v0 = d->max / 2;
    pref_set(r, v0); tick();
    mknob(n, 1); tick();
    CHECK(pref_get(r) > v0);
    mknob(n, -1); mknob(n, -1); tick();
    CHECK(pref_get(r) < v0 + 1);
}


/* A burst larger than the input ring must not lose the newest fader position
 * or any encoder detent, in either preset, on the main and drum mixers. */
static void check_burst(void)
{
    for (preset_user = 0; preset_user < 2; preset_user++)
        for (int page = 0; page < 2; page++) {
            setup(V_MIX, PART_909);
            if (preset_user) proj.set.bt |= PROJ_BT_SMC_USER;
            build_pages(); *page_of(V_MIX) = (uint8_t)page; build_pages(); tick();
            pref_t r = smc_target(2);
            const x0x_param_t *d = pref_desc(r);
            pref_set(r, 0); tick();
            for (int v = 0; v <= 120; v += 2) {
                uint8_t m[3] = {(uint8_t)(preset_user ? 0xBF : 0xE2), (uint8_t)(preset_user ? 42 : 0), (uint8_t)v};
                smc_ble_midi(m, 3);
            }
            CHECK(smc_inputs[0].dropped == 0);
            /* A later sample for the same strip arrives after the ring has room. */
            smc_poll();
            { uint8_t m[3] = {(uint8_t)(preset_user ? 0xBF : 0xE2), (uint8_t)(preset_user ? 42 : 0), 110}; smc_ble_midi(m, 3); }
            smc_poll(); tick();
            CHECK(pref_get(r) == (110 * d->max + 63) / 127);
            /* Encoder: 40 single detents in one burst move as far as 40 in time. */
            pref_t k = smc_knob_target(2);
            if (k.kind == R_NONE) continue;
            pref_set(k, 0); tick();
            uint8_t st = (uint8_t)(preset_user ? 0xBF : 0xB0);
            for (int i = 0; i < 40; i++) { uint8_t m[3] = {st, 18, 1}; smc_ble_midi(m, 3); }
            smc_poll(); tick();
            int fast = pref_get(k);
            pref_set(k, 0); tick();
            for (int i = 0; i < 40; i++) { uint8_t m[3] = {st, 18, 1}; smc_ble_midi(m, 3); smc_poll(); }
            tick();
            CHECK(smc_inputs[0].dropped == 0);
            CHECK(fast > 0 && pref_get(k) > 0);
            if (fast < pref_get(k) * 3 / 4 && fast < pref_desc(k)->max)
                fprintf(stderr, "burst knob page %d: %d vs %d\n", page, fast, pref_get(k));
            CHECK(fast >= pref_get(k) * 3 / 4 || fast == pref_desc(k)->max);
        }
}

int main(void)
{
    static const ctx_t ctx[] = {
        {"HOME", V_HOME, -1, PART_909},
        {"MIX PARTS", V_MIX, 0, PART_909},
        {"909 MIX", V_MIX, 1, PART_909},
        {"808 MIX", V_MIX, 2, PART_909},
        {"909 SEQ", V_SEQ, -1, PART_909},
        {"808 SEQ", V_SEQ, -1, PART_808},
        {"MASTER", V_MIX, 3, PART_909},
    };
    for (preset_user = 0; preset_user < 2; preset_user++)
        for (unsigned i = 0; i < sizeof ctx / sizeof *ctx; i++) {
            const ctx_t *c = &ctx[i];
            enter(c);
            int kit = c->view == V_SEQ ? c->part : c->view == V_MIX && (c->page == 1 || c->page == 2) ? c->page - 1 : -1;
            for (int bank = 0; bank < (kit >= 0 ? 2 : 1); bank++) {
                if (kit >= 0) { ui.sel[kit] = (uint8_t)(bank * 8); tick(); }
                for (int n = 0; n < 8; n++) {
                    pref_t f, k;
                    if (kit >= 0) {
                        int t = bank * 8 + n;
                        f = t < NDRUM ? find_ref(kit ? T_808 : T_909, t, "Level") : NONE;
                        k = t < NDRUM ? find_ref(kit ? T_808 : T_909, t, "Pan") : NONE;
                    } else {
                        static const pref_t tail_f[3] = {PR(R_ENG, T_FX, 0, FX_RV_LEVEL), PR(R_ENG, T_FX, 0, FX_DL_LEVEL), PR(R_ENG, T_MST, 0, MST_DJF)};
                        static const pref_t tail_k[3] = {PR(R_ENG, T_FX, 0, FX_RV_DECAY), PR(R_ENG, T_FX, 0, FX_DL_FDBK), PR(R_ENG, T_MST, 0, MST_RESO)};
                        f = n < NPARTS ? PR(R_ENG, T_MIX, part_at(n), MX_LEVEL) : tail_f[n - NPARTS];
                        k = n < NPARTS ? PR(R_ENG, T_MIX, part_at(n), MX_CUT) : tail_k[n - NPARTS];
                        if (c->page == 3) {      /* MASTER: four visible fields, K4 is resonance */
                            static const pref_t mk[4] = {PR(R_ENG, T_FX, 0, FX_DRIVE), PR(R_ENG, T_MST, 0, MST_COMP1), PR(R_ENG, T_MST, 0, MST_PUMP), PR(R_ENG, T_MST, 0, MST_RESO)};
                            k = n < 4 ? mk[n] : NONE;
                        }
                    }
                    check_fader(c, n, f);
                    check_knob(c, n, k);
                }
            }
        }
    check_burst();
    printf("smc_mode_page_test: %u checks PASS\n", checks);
    return 0;
}
