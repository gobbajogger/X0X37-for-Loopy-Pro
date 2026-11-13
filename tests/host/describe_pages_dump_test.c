/* SPDX-License-Identifier: GPL-3.0-only */
/* Dump describe_pages output for every view/part/engine/selection/mixsel,
 * plus framebuffer hashes of the screens that consume those pages.
 * Compare two dumps (before vs after a shrink) for bit-identity. */
#define SMC_UI_MAIN smc_ui_suite_unused
#include "smc_ui_test.c"

#include <stdint.h>

static uint64_t fnv1a(const void *p, size_t n)
{
    const uint8_t *b = p;
    uint64_t h = 14695981039346656037ull;
    while (n--) {
        h ^= *b++;
        h *= 1099511628211ull;
    }
    return h;
}

static void put_pref(FILE *fp, pref_t r)
{
    fprintf(fp, " {%u,%u,%u,%u}", r.kind, r.a, r.b, r.c);
}

static void dump_one(FILE *fp, int view, int p, int mixsel, const uint8_t *sel)
{
    pages_t out;
    memset(&out, 0, sizeof out);
    describe_pages(&out, 0, view, p, mixsel, sel);
    fprintf(fp, "V=%d P=%d MIX=%d SEL=%u,%u,%u,%u,%u ENG=%d,%d n=%d nmore=%d\n",
            view, p, mixsel,
            sel[0], sel[1], sel[2], sel[3], sel[4],
            engine_bass_bank(0), engine_bass_bank(1),
            out.n, out.nmore);
    for (int i = 0; i < out.n; i++) {
        fprintf(fp, "  P%d g=%u \"%s\"", i, out.group[i], out.title[i]);
        for (int k = 0; k < 4; k++)
            put_pref(fp, out.r[i][k]);
        fputc('\n', fp);
    }
    for (int i = 0; i < out.nmore; i++) {
        fprintf(fp, "  M%d", i);
        put_pref(fp, out.more[i]);
        fputc('\n', fp);
    }
    /* Query mode must agree with the full table, page by page. */
    for (int page = 0; page < out.n + 1; page++) {
        page_query_t q;
        memset(&q, 0, sizeof q);
        q.page = page;
        describe_pages(0, &q, view, p, mixsel, sel);
        CHECK(q.n == out.n);
        if (page < out.n) {
            CHECK(!strcmp(q.title, out.title[page]));
            CHECK(!memcmp(q.r, out.r[page], sizeof q.r));
        }
    }
}

static int apply_bank(int voice, int bank)
{
    if (engine_bass_bank(voice) == bank)
        return 1;
    if (!engine_bass_bank_available(voice, bank))
        return 0;
    if (engine_bass_select_preset(voice, bank, 0, &proj.sound) != 0)
        return 0;
    int32_t buf[512];
    for (int n = 0; n < 8000 && engine_bass_preset_pending(voice); n++)
        engine_render(buf, 256);
    return engine_bass_bank(voice) == bank;
}

static uint64_t frame_hash(void)
{
    ui_frame();
    return fnv1a(fb, 240 * 240 * 2);
}

int main(void)
{
    const char *path = getenv("DESCRIBE_DUMP");
    FILE *fp = path ? fopen(path, "w") : stdout;
    CHECK(fp);

    setup(V_HOME, PART_909);

    int n909 = engine_nvoices(T_909);
    int n808 = engine_nvoices(T_808);
    CHECK(n909 > 0 && n808 > 0);

    /* Exhaustive relevant axes, plus corner crosses of the others. */
    const int views[] = {V_HOME, V_PART, V_GEN, V_FX, V_MIX, V_GLO, V_SONG, V_PERF, V_SEQ, 99};
    const int nviews = (int)(sizeof views / sizeof *views);
    const int engines[] = {
        ORIGINAL_SOUND_CHICAGO, ORIGINAL_SOUND_MIAMI,
        ORIGINAL_SOUND_DRONE, ORIGINAL_SOUND_JUNO
    };

    uint8_t sel[NPARTS];
    memset(sel, 0, sizeof sel);

    int dumps = 0;
    for (int ev = 0; ev < 4; ev++) {
        int ea = engines[ev], eb = engines[(ev + 1) & 3];
        int got_a = apply_bank(0, ea);
        int got_b = apply_bank(1, eb);
        fprintf(fp, "# banks request %d,%d applied %d,%d got %d,%d\n",
                ea, eb, got_a, got_b, engine_bass_bank(0), engine_bass_bank(1));
        for (int vi = 0; vi < nviews; vi++) {
            int view = views[vi];
            for (int p = 0; p < NPARTS; p++) {
                int mix_lo = 0, mix_hi = 0;
                int s0_hi = 0, s1_hi = 0;
                if (view == V_MIX) { mix_hi = NPARTS; }
                if (view == V_PART || view == V_MIX || view == V_SEQ) {
                    s0_hi = n909 - 1;
                    s1_hi = n808 - 1;
                }
                /* Every voice on the relevant screens; 0 and last as corners always. */
                for (int mix = mix_lo; mix <= mix_hi; mix++) {
                    int s0s[] = {0, s0_hi / 2, s0_hi, n909 > 1 ? 1 : 0};
                    int s1s[] = {0, s1_hi / 2, s1_hi, n808 > 1 ? 1 : 0};
                    int ns0 = (view == V_PART || view == V_MIX) ? n909 : 4;
                    int ns1 = (view == V_PART || view == V_MIX) ? n808 : 4;
                    if (view == V_PART || view == V_MIX) {
                        for (int s0 = 0; s0 < n909; s0++) {
                            for (int s1 = 0; s1 < n808; s1++) {
                                sel[0] = (uint8_t)s0;
                                sel[1] = (uint8_t)s1;
                                sel[2] = sel[3] = sel[4] = 0;
                                dump_one(fp, view, p, mix, sel);
                                dumps++;
                            }
                        }
                    } else {
                        for (int i = 0; i < 4; i++) {
                            sel[0] = (uint8_t)s0s[i];
                            sel[1] = (uint8_t)s1s[i];
                            sel[2] = sel[3] = sel[4] = 0;
                            dump_one(fp, view, p, mix, sel);
                            dumps++;
                            (void)ns0; (void)ns1;
                        }
                    }
                }
            }
        }
    }

    /* Independent engine pairs: all 4x4 for V_PART bass pages (the only
     * describe_pages path that reads engine_bass_bank). */
    fprintf(fp, "# full engine pairs, V_PART bass\n");
    for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) {
        int got_a = apply_bank(0, engines[a]);
        int got_b = apply_bank(1, engines[b]);
        fprintf(fp, "# pair %d,%d applied %d,%d got %d,%d\n",
                engines[a], engines[b], got_a, got_b,
                engine_bass_bank(0), engine_bass_bank(1));
        sel[0] = sel[1] = 0;
        dump_one(fp, V_PART, PART_303A, 0, sel);
        dump_one(fp, V_PART, PART_303B, 0, sel);
        dumps += 2;
    }

    /* Real-framebuffer identity for every affected screen. */
    fprintf(fp, "# framebuffer hashes\n");
    for (int vi = 0; vi < 9; vi++) {
        int view = views[vi];
        for (int p = 0; p < NPARTS; p++) {
            setup(view, p);
            for (int ev = 0; ev < 4; ev++) {
                apply_bank(0, engines[ev]);
                apply_bank(1, engines[(ev + 2) & 3]);
                for (int s0 = 0; s0 < n909; s0 += (n909 > 1 ? n909 - 1 : 1)) {
                    ui.sel[0] = (uint8_t)s0;
                    ui.sel[1] = 0;
                    ui.mixsel = (uint8_t)p;
                    build_pages();
                    uint64_t h = frame_hash();
                    fprintf(fp, "FB V=%d P=%d MIX=%d SEL0=%d ENG=%d,%d HASH=%016llx n=%d nmore=%d title0=\"%s\"\n",
                            view, p, p, s0,
                            engine_bass_bank(0), engine_bass_bank(1),
                            (unsigned long long)h, pg.n, pg.nmore,
                            pg.n ? pg.title[0] : "");
                }
            }
        }
    }

    fprintf(fp, "# dumps=%d checks=%u\n", dumps, checks);
    if (fp != stdout)
        fclose(fp);
    printf("describe_pages_dump: %d dumps, %u query-identity checks PASS\n", dumps, checks);
    return 0;
}
