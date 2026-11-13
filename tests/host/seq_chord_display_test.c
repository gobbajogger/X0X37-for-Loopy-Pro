/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test of the SEQ chord display fix. Compiles the real ui.c draw path
 * and writes native 240x240 AFTER frames. Inverts the 94197fc8 baseline
 * "extra pitch is absent" checks, adds playback identity, Chicago/Miami
 * mono identity, draw-time, and the 23-row chord_name.py acceptance set against chord_label_fmt. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"

static const char *evidence_dir;
static FILE *meas;

static const char *evdir(void)
{
    const char *d = evidence_dir;
    if (!d) d = getenv("SEQ_CHORD_EVIDENCE");
    if (!d) d = "evidence/after";
    return d;
}

static void capture(const char *name)
{
    char path[1024];
    CHECK(snprintf(path, sizeof path, "%s/%s.png", evdir(), name) > 0);
    png_write(path);
    printf("WROTE %s\n", path);
    if (meas) fprintf(meas, "png %s\n", path);
}

static void paint(void)
{
    ui.help = 0;
    ui.msg_until = 0;
    ui.touched = -1;
    draw_header();
    draw_main();
    draw_knobs();
}

/* Match the fixed draw_303 window (all onset pitches, 14-st floor, ±2). */
static void pitch_window(const bpart_t *bp, int *lo_out, int *hi_out)
{
    int c, lo = 127, hi = 0;
    int poly = seq_bass_poly(&seq, bidx());
    for (c = 0; c < bp->len && c < NSTEPS; c++) {
        uint8_t notes[HELD_CHORD_NOTES];
        int nn, k;
        if (bstep_gate(&bp->step[c]) != G_NOTE)
            continue;
        nn = poly ? onset_chord_of(bp, c, notes) : 1;
        if (!poly || nn <= 0) {
            notes[0] = bp->step[c].note;
            nn = 1;
        }
        for (k = 0; k < nn; k++) {
            if (notes[k] < lo) lo = notes[k];
            if (notes[k] > hi) hi = notes[k];
        }
    }
    if (lo > hi)
        lo = hi = 12 * (bp->gen.base_oct + 1) + bp->gen.root;
    lo -= 2;
    hi += 2;
    if (hi - lo < 14)
        hi = lo + 14;
    *lo_out = lo;
    *hi_out = hi;
}

static uint16_t step_ink(int step)
{
    const bpart_t *bp = &cur_pat()->bass[bidx()];
    const bstep_t *st = &bp->step[step];
    if (step == ui.held_step) return C_WHITE;
    if (st->flags & BS_ACCENT) return part_col();
    return dim(part_col(), 9);
}

static int mark_at_pitch(int col, int midi, int lo, int hi, uint16_t ink)
{
    int x = (int)col_x(col);
    int y = MAIN_Y + note_y(midi, lo, hi);
    uint16_t want = swap16(ink);
    int i, hits = 0;
    if (y + 2 < MAIN_Y + ROLL_TOP - 2 || y + 2 > MAIN_Y + ROLL_BOT + 4)
        return 0;
    if (x < 0 || x + 11 >= 240 || y < 0 || y + 4 >= 240)
        return 0;
    for (i = 2; i <= 9; i++)
        if (fb[(y + 2) * 240 + x + i] == want)
            hits++;
    return hits >= 5;
}

static int in_window(int midi, int lo, int hi)
{
    return midi >= lo && midi <= hi;
}

static void clear_bass(int b)
{
    bpart_t *bp = &cur_pat()->bass[b];
    int s;
    for (s = 0; s < NSTEPS; s++) {
        (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, s, 0, 0);
        bp->step[s].flags = G_REST;
        bp->step[s].velocity = 0;
    }
}

static void set_chord(int b, int step, const uint8_t *notes, int n, uint8_t flags)
{
    bpart_t *bp = &cur_pat()->bass[b];
    uint8_t got[HELD_CHORD_NOTES];
    int ng, i;
    CHECK(n >= 1 && n <= HELD_CHORD_NOTES);
    CHECK((unsigned)step < NSTEPS);
    for (i = 1; i < n; i++)
        CHECK(notes[i] > notes[i - 1]);
    bp->step[step].note = notes[0];
    bp->step[step].flags = flags;
    bp->step[step].velocity = BASS_VELOCITY_DEFAULT;
    if (n > 1)
        CHECK(held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, notes, n) == 1);
    else
        (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, 0, 0);
    ng = held_chord_get(proj.arr.lane, seq.ppat[ui.part], b, step, notes[0], got);
    CHECK(ng == n);
    for (i = 0; i < n; i++)
        CHECK(got[i] == notes[i]);
}

static void set_tie(int b, int step, uint8_t root)
{
    bpart_t *bp = &cur_pat()->bass[b];
    uint8_t got[HELD_CHORD_NOTES];
    bp->step[step].note = root;
    bp->step[step].flags = (uint8_t)(G_TIE | 0xF0u);
    bp->step[step].velocity = BASS_VELOCITY_DEFAULT;
    (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, step, 0, 0);
    CHECK(held_chord_get(proj.arr.lane, seq.ppat[ui.part], b, step, root, got) == 1);
}

static void choose_kind(int kind)
{
    int voice = bidx();
    int32_t audio[512];
    int n;
    engine_bass_off(voice);
    CHECK(engine_bass_select_preset(voice, kind, 0, &proj.sound) == 0);
    for (n = 0; n < 1024 && engine_bass_preset_pending(voice); n++)
        engine_render(audio, 256);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_sound_bank(&proj.sound, voice) == kind);
    CHECK(engine_bass_bank(voice) == kind);
}

static void seq_ready_part(int part, int kind, int len, int spage)
{
    bpart_t *bp;
    setup(V_SEQ, part);
    choose_kind(kind);
    ui.view = V_SEQ;
    ui.part = part;
    ui.overlay = O_NONE;
    ui.held_step = -1;
    ui.rec = 0;
    ui.spage = (uint8_t)spage;
    seq.playing = 0;
    bp = &cur_pat()->bass[bidx()];
    bp->len = (uint8_t)len;
    bp->transpose = 24;
    clear_bass(bidx());
}

static void seq_ready(int len, int spage)
{
    seq_ready_part(PART_303B, ORIGINAL_SOUND_JUNO, len, spage);
}

static void report_column(const char *tag, int step, const uint8_t *notes, int n)
{
    const bpart_t *bp = &cur_pat()->bass[bidx()];
    int lo, hi, i, col = step - ui.spage * 16;
    uint16_t ink;
    pitch_window(bp, &lo, &hi);
    ink = step_ink(step);
    if (meas)
        fprintf(meas,
                "%s step=%d col=%d spage=%d held=%d playing=%d ph=%d "
                "window=[%d,%d] ink=%04x nstore=%d\n",
                tag, step, col, ui.spage, ui.held_step, seq.playing,
                seq.playing ? eng_step[TRK_BASS0 + bidx()] : -1, lo, hi, ink, n);
    CHECK(col >= 0 && col < 16);
    for (i = 0; i < n; i++) {
        int inside = in_window(notes[i], lo, hi);
        int drawn = mark_at_pitch(col, notes[i], lo, hi, ink);
        if (meas)
            fprintf(meas, "  note[%d]=%d inside=%d drawn=%d\n", i, notes[i], inside, drawn);
        printf("%s note[%d]=%d inside=%d drawn=%d\n", tag, i, notes[i], inside, drawn);
        CHECK(inside);
        CHECK(drawn); /* inverted: every stored pitch is painted */
    }
}

static void copy_fb(uint16_t *dst)
{
    memcpy(dst, fb, sizeof fb);
}

static int roll_equal(const uint16_t *a, const uint16_t *b)
{
    int y;
    for (y = MAIN_Y + ROLL_TOP - 4; y <= MAIN_Y + ROLL_BOT + 6; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 240 * 2) != 0)
            return 0;
    return 1;
}

static int plock_notch_at(int col, int root, int lo, int hi)
{
    int x = (int)col_x(col) + 5;
    int y = MAIN_Y + note_y(root, lo, hi) - 3;
    uint16_t white = swap16(C_WHITE);
    return fb[y * 240 + x] == white && fb[y * 240 + x + 1] == white &&
           fb[(y + 1) * 240 + x] == white && fb[(y + 1) * 240 + x + 1] == white;
}

static int label_equal(const uint16_t *a, const uint16_t *b)
{
    int y;
    for (y = MAIN_Y + 118; y < MAIN_Y + 2 * BAND_H; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 240 * 2) != 0)
            return 0;
    for (y = KNOB_Y + 13; y < KNOB_Y + 68; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 60 * 2) != 0)
            return 0;
    return 1;
}

static int fb_equal(const uint16_t *a, const uint16_t *b)
{
    return memcmp(a, b, 240 * 240 * 2) == 0;
}

static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}

static void time_paint(const char *tag, int n)
{
    int i;
    double t, full;
    paint();
    t = cpu_ms();
    for (i = 0; i < n; i++) {
        memset(blit_hash, 0, sizeof blit_hash);
        paint();
    }
    full = (cpu_ms() - t) / n;
    printf("DRAW_TIME %s iters=%d full_ms=%.6f\n", tag, n, full);
    if (meas) fprintf(meas, "DRAW_TIME %s iters=%d full_ms=%.6f\n", tag, n, full);
}

static uint32_t playback_hash(void)
{
    uint32_t h = 2166136261u;
    int32_t audio[512];
    int i, j;
    seq.playing = 0;
    seq_start(&seq);
    for (i = 0; i < 128; i++) {
        engine_render(audio, 256);
        for (j = 0; j < 256; j++) {
            h ^= (uint32_t)audio[j];
            h *= 16777619u;
        }
    }
    seq_stop(&seq);
    engine_render(audio, 256);
    for (j = 0; j < 256; j++) {
        h ^= (uint32_t)audio[j];
        h *= 16777619u;
    }
    return h;
}

/* 23-row acceptance from smk-ux/seq-chords/chord_name.py, against the
 * firmware function chord_label_fmt (sounding MIDI after +transpose-24).
 * TIER2 is a compile switch: this table lists both T1 and T2 expects.
 * CEGA-t1 is skipped when TIER2 is on; CEGA-t2 is skipped when TIER2 is
 * off, so each published expectation is asserted on a matching binary. */
#define SEQ_CHORD_HAVE_T2 (SEQ_CHORD_LABEL_TIER2 >= 7)

typedef struct {
    const char *id;
    uint8_t stored[6];
    int n, trans, need_t2;
    const char *sym_t2, *cell_t2;
    const char *sym_t1, *cell_t1;
    const char *notes;
    const char *held; /* NULL: do not assert held (T1/T2 held text differs) */
} chord_row_t;

static const chord_row_t CHORD_ROWS[] = {
    {"maj-root",         {48,52,55},       3, 24, 1, "C",           "C",         "C",        "C",       "C3 E3 G3",            "STEP 1  C  C3 E3 G3  V100"},
    {"maj-inv",          {52,55,60},       3, 24, 1, "C/E",         "C/E",       "C/E",      "C/E",     "E3 G3 C4",            "STEP 1  C/E  E3 G3 C4  V100"},
    {"min7-flats-in",    {48,51,55,58},    4, 24, 1, "Cmin7",       "Cmin7",     "Cmin7",    "Cmin7",   "C3 D#3 G3 A#3",       "STEP 1  Cmin7  C3 D#3 G3 A#3  V100"},
    {"maj-spread",       {48,55,60,64},    4, 24, 1, "C",           "C",         "C",        "C",       "C3 G3 C4 E4",         "STEP 1  C  C3 G3 C4 E4  V100"},
    {"power",            {48,55},          2, 24, 1, "C5",          "C5",        "C5",       "C5",      "C3 G3",               "STEP 1  C5  C3 G3  V100"},
    {"CEGA-t2",          {48,52,55,57},    4, 24, 1, "C6",          "C6",        "Amin7/C",  "Amin7/C", "C3 E3 G3 A3",         0},
    {"CEGA-t1",          {48,52,55,57},    4, 24, 0, "C6",          "C6",        "Amin7/C",  "Amin7/C", "C3 E3 G3 A3",         "STEP 1  Amin7/C  C3 E3 G3 A3  V100"},
    {"cluster",          {48,49,50},       3, 24, 1, "",            "C3x3",      "",         "C3x3",    "C3 C#3 D3",           "STEP 1  C3 C#3 D3  V100"},
    {"oct-double",       {60,72},          2, 24, 1, "",            "C4x2",      "",         "C4x2",    "C4 C5",               "STEP 1  C4 C5  V100"},
    {"mono",             {48},             1, 24, 1, "",            "C3",        "",         "C3",      "C3",                  "STEP 1  C3  V100"},
    {"maj7-recorded",    {48,52,55,59},    4, 24, 1, "Cmaj7",       "Cmaj7",     "Cmaj7",    "Cmaj7",   "C3 E3 G3 B3",         "STEP 1  Cmaj7  C3 E3 G3 B3  V100"},
    {"hex-recorded",     {48,52,55,59,62,65}, 6, 24, 1, "",         "C3x6",      "",         "C3x6",    "C3 E3 G3 B3 D4 F4",   "STEP 1  C3 E3 G3 B3 D4 F4  V100"},
    {"min",              {48,51,55},       3, 24, 1, "Cmin",        "Cmin",      "Cmin",     "Cmin",    "C3 D#3 G3",           "STEP 1  Cmin  C3 D#3 G3  V100"},
    {"dom7",             {48,52,55,58},    4, 24, 1, "C7",          "C7",        "C7",       "C7",      "C3 E3 G3 A#3",        "STEP 1  C7  C3 E3 G3 A#3  V100"},
    {"aug-root",         {48,52,56},       3, 24, 1, "Caug",        "Caug",      "",         "C3x3",    "C3 E3 G#3",           0},
    {"aug-inv",          {52,56,60},       3, 24, 1, "Eaug",        "Eaug",      "",         "E3x3",    "E3 G#3 C4",           0},
    {"dim7-root",        {48,51,54,57},    4, 24, 1, "Cdim7",       "Cdim7",     "",         "C3x4",    "C3 D#3 F#3 A3",       0},
    {"min7b5-slash",     {46,48,51,54},    4, 24, 1, "Cmin7b5/A#",  "Cmin7b5",  "",         "A#2x4",   "A#2 C3 D#3 F#3",      0},
    {"gs-seven",         {44,48,51,54},    4, 24, 1, "G#7",         "G#7",       "G#7",      "G#7",     "G#2 C3 D#3 F#3",      "STEP 1  G#7  G#2 C3 D#3 F#3  V100"},
    {"longest-produced", {44,46,49,52},    4, 24, 1, "A#min7b5/G#", "A#min7b5", "",         "G#2x4",   "G#2 A#2 C#3 E3",      0},
    {"sus-ambiguous",    {50,55,60},       3, 24, 1, "",            "D3x3",      "",         "D3x3",    "D3 G3 C4",            "STEP 1  D3 G3 C4  V100"},
    {"dyad-third",       {48,52},          2, 24, 1, "",            "C3x2",      "",         "C3x2",    "C3 E3",               "STEP 1  C3 E3  V100"},
    {"transpose-plus12", {48,52,55},       3, 36, 1, "C",           "C",         "C",        "C",       "C4 E4 G4",            "STEP 1  C  C4 E4 G4  V100"},
};

static int chord_row_applies(const chord_row_t *r)
{
    /* The two CEGA rows share pitches and only one published expect applies. */
    if (r->need_t2 && !SEQ_CHORD_HAVE_T2 && r->sym_t2[0] &&
        strcmp(r->sym_t2, r->sym_t1) != 0 && strcmp(r->id, "CEGA-t2") == 0)
        return 0;
    if (!r->need_t2 && SEQ_CHORD_HAVE_T2 && strcmp(r->id, "CEGA-t1") == 0)
        return 0;
    return 1;
}

static void acc_row(const chord_row_t *r, int *nrun, int *nwrong)
{
    uint8_t sound[6];
    int i, trans = r->trans;
    chord_label_t o;
    const char *want_sym, *want_cell;
    if (!chord_row_applies(r))
        return;
    for (i = 0; i < r->n; i++) {
        int p = (int)r->stored[i] + trans - 24;
        if (p < 0) p = 0;
        if (p > 127) p = 127;
        sound[i] = (uint8_t)p;
    }
    /* Store is not passed to the namer; sounding MIDI is. Stored values
     * stay as in the table (this test never writes back). */
    chord_label_fmt(&o, sound, r->n, sound, r->n, "STEP 1  ", " V100");
    want_sym  = SEQ_CHORD_HAVE_T2 ? r->sym_t2  : r->sym_t1;
    want_cell = SEQ_CHORD_HAVE_T2 ? r->cell_t2 : r->cell_t1;
    printf("ACC %s trans=%d t2=%d symbol='%s' cell='%s' notes='%s' held='%s' (want sym='%s' cell='%s' notes='%s')\n",
           r->id, trans, SEQ_CHORD_HAVE_T2, o.symbol, o.cell, o.notes, o.held,
           want_sym, want_cell, r->notes);
    if (meas)
        fprintf(meas, "ACC %s symbol='%s' cell='%s' notes='%s' held='%s' want_sym='%s' want_cell='%s' want_notes='%s'\n",
                r->id, o.symbol, o.cell, o.notes, o.held, want_sym, want_cell, r->notes);
    (*nrun)++;
    if (strcmp(o.symbol, want_sym) != 0 ||
        strcmp(o.cell, want_cell) != 0 ||
        strcmp(o.notes, r->notes) != 0 ||
        (r->held && strcmp(o.held, r->held) != 0) ||
        (want_sym[0] && strchr(o.symbol, '?'))) {
        (*nwrong)++;
        printf("  WRONG %s\n", r->id);
    }
    CHECK(strcmp(o.symbol, want_sym) == 0);
    CHECK(strcmp(o.cell, want_cell) == 0);
    CHECK(strcmp(o.notes, r->notes) == 0);
    if (r->held)
        CHECK(strcmp(o.held, r->held) == 0);
    if (want_sym[0])
        CHECK(strchr(o.symbol, '?') == 0);
    /* transpose-plus12: sounding is +12; the stored table is still 48,52,55. */
    if (strcmp(r->id, "transpose-plus12") == 0) {
        CHECK(r->stored[0] == 48 && r->stored[1] == 52 && r->stored[2] == 55);
        CHECK(sound[0] == 60 && sound[1] == 64 && sound[2] == 67);
    }
}

static void acc(const char *tag, const uint8_t *n, int nn, const char *sym, const char *cell)
{
    chord_label_t o;
    chord_label_fmt(&o, n, nn, n, nn, "STEP 1  ", " V100");
    printf("ACC %s symbol='%s' cell='%s' notes='%s' rec=%d (want sym='%s' cell='%s')\n",
           tag, o.symbol, o.cell, o.notes, o.recognized, sym, cell);
    if (meas)
        fprintf(meas, "ACC %s symbol='%s' cell='%s' want_sym='%s' want_cell='%s'\n",
                tag, o.symbol, o.cell, sym, cell);
    CHECK(strcmp(o.symbol, sym) == 0);
    CHECK(strcmp(o.cell, cell) == 0);
}

static void hold_step(int s)
{
    ui.held_step = s;
    ui.held_step_part = ui.part;
    ui.held_step_pat = seq.ppat[ui.part];
}

static void test_symbols(void)
{
    /* 23-row UX acceptance (chord_name.py) against chord_label_fmt. */
    static const uint8_t ceg[] = {48, 52, 55};
    static const uint8_t egc[] = {52, 55, 60};
    static const uint8_t cmin7[] = {48, 51, 55, 58}; /* C D# G A# (put_note spelling of Eb Bb) */
    static const uint8_t fold[] = {48, 55, 60, 64};  /* C3 G3 C4 E4 */
    static const uint8_t cg[] = {48, 55};
    static const uint8_t cega[] = {48, 52, 55, 57};
    static const uint8_t cluster[] = {48, 49, 50};
    static const uint8_t oct[] = {60, 72};
    static const uint8_t mono[] = {48};
    static const uint8_t cmaj7[] = {48, 52, 55, 59};
    static const uint8_t c7[] = {48, 52, 55, 58};
    static const uint8_t cmin[] = {48, 51, 55};
    static const uint8_t gs7[] = {44, 48, 51, 54};
    static const uint8_t hexa[] = {48, 52, 55, 59, 62, 65};
    static const uint8_t dyad[] = {48, 52};
    int b, i, nrun = 0, nwrong = 0;
    CHECK((int)(sizeof CHORD_ROWS / sizeof CHORD_ROWS[0]) == 23);
#if !SEQ_CHORD_LABEL_TIER1
    /* Fallback-only compile: matcher is out; every multi-note set is compact. */
    {
        static const uint8_t ceg_fb[] = {48, 52, 55};
        static const uint8_t cmin7_fb[] = {48, 51, 55, 58};
        static const uint8_t mono_fb[] = {48};
        chord_label_t o;
        chord_label_fmt(&o, ceg_fb, 3, ceg_fb, 3, "STEP 1  ", " V100");
        CHECK(!o.recognized && o.symbol[0] == 0);
        CHECK(strcmp(o.cell, "C3x3") == 0);
        CHECK(strcmp(o.held, "STEP 1  C3 E3 G3  V100") == 0);
        chord_label_fmt(&o, cmin7_fb, 4, cmin7_fb, 4, "STEP 1  ", " V100");
        CHECK(strcmp(o.cell, "C3x4") == 0);
        CHECK(!o.recognized);
        chord_label_fmt(&o, mono_fb, 1, mono_fb, 1, "STEP 1  ", " V100");
        CHECK(strcmp(o.cell, "C3") == 0);
        CHECK(strcmp(o.held, "STEP 1  C3  V100") == 0);
        printf("fallback-only TIER1=0: CEG C3x3, Cmin7 C3x4, mono C3 — PASS\n");
        if (meas)
            fprintf(meas, "fallback-only TIER1=0 PASS\n");
        return;
    }
#endif
    for (i = 0; i < 23; i++)
        acc_row(&CHORD_ROWS[i], &nrun, &nwrong);
    printf("chord_name.py 23-row host: ran %d this compile (TIER1=%d TIER2=%d), %d wrong symbols\n",
           nrun, SEQ_CHORD_LABEL_TIER1, SEQ_CHORD_LABEL_TIER2, nwrong);
    if (meas)
        fprintf(meas, "23-row ran=%d wrong=%d TIER1=%d TIER2=%d\n",
                nrun, nwrong, SEQ_CHORD_LABEL_TIER1, SEQ_CHORD_LABEL_TIER2);
    CHECK(nwrong == 0);
    CHECK(nrun >= 22); /* 22 of 23 on each binary; the CEGA pair is split */

    /* Mono held line matches piano-roll §9 (two spaces before V). */
    {
        chord_label_t o;
        chord_label_fmt(&o, mono, 1, mono, 1, "STEP 1  ", " V100");
        CHECK(strcmp(o.held, "STEP 1  C3  V100") == 0);
        CHECK(o.held_font == &FONT_B);
        CHECK(o.cell_font == &FONT_M);
    }

#ifdef SEQ_CHORD_ROWS_ONLY
    (void)b; (void)ceg; (void)egc; (void)cmin7; (void)fold; (void)cg;
    (void)cega; (void)cluster; (void)oct; (void)cmaj7; (void)c7;
    (void)cmin; (void)gs7; (void)hexa; (void)dyad;
    return;
#else

    /* Frames for the chord-symbol cases (held Juno step). */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, ceg, 3, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, egc, 3, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-CslashE");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cmin7, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-Cmin7");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, fold, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C-folded");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cg, 2, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C5");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cega, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-CEGA");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cluster, 3, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C3x3");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, oct, 2, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C4x2");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C3");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cmaj7, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-Cmaj7");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, cmin, 3, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-Cmin");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, c7, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C7");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, gs7, 4, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-Gsharp7");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, hexa, 6, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C3x6");

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, dyad, 2, G_NOTE);
    hold_step(0);
    paint();
    capture("after-symbol-C3x2");
#endif /* !SEQ_CHORD_ROWS_ONLY */
}

static void test_mono_identity_and_chords(void)
{
    static const uint8_t dyad[] = {48, 52};
    static const uint8_t triad[] = {48, 52, 55};
    static const uint8_t tetrad[] = {48, 52, 55, 59};
    static const uint8_t hexa[] = {48, 52, 55, 59, 62, 65};
    static const uint8_t mono[] = {48};
    static const uint8_t mono_e[] = {52};
    static const uint8_t mono_a[] = {57};
    static const uint8_t page2[] = {60, 64, 67};
    uint16_t fb_mono[240 * 240], fb_triad[240 * 240], fb_mono_e[240 * 240];
    uint16_t fb_chi[240 * 240], fb_mia[240 * 240];
    int lo, hi, b, playhead_hits, x, y;
    uint16_t ph_ink;
    const int Ntime = 400;

    /* --- mono control --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    copy_fb(fb_mono);
    report_column("mono-C3", 0, mono, 1);
    capture("after-mono-control");
    time_paint("juno-mono", Ntime);

    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, mono_e, 1, G_NOTE);
    paint();
    copy_fb(fb_mono_e);
    report_column("mono-E3", 0, mono_e, 1);
    capture("after-mono-E3");
    CHECK(!roll_equal(fb_mono, fb_mono_e));

    /* --- 2-note --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, dyad, 2, G_NOTE);
    paint();
    report_column("dyad", 0, dyad, 2);
    capture("after-dyad");

    /* --- 3-note: extras painted, no false p-lock, differs from mono --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    paint();
    copy_fb(fb_triad);
    report_column("triad", 0, triad, 3);
    capture("after-triad");
    time_paint("juno-triad", Ntime);
    CHECK(!roll_equal(fb_mono, fb_triad));
    CHECK(!plock_notch_at(0, 48, 46, 60));
    pitch_window(&cur_pat()->bass[b], &lo, &hi);
    CHECK(in_window(52, lo, hi));
    CHECK(mark_at_pitch(0, 52, lo, hi, step_ink(0)) == 1);
    CHECK(mark_at_pitch(0, 48, lo, hi, step_ink(0)) == 1);
    CHECK(mark_at_pitch(0, 55, lo, hi, step_ink(0)) == 1);

    /* 2/3/4/6 frames must differ from each other (acceptance a). */
    /* --- 4-note --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, tetrad, 4, G_NOTE);
    paint();
    report_column("tetrad", 0, tetrad, 4);
    capture("after-tetrad");
    CHECK(!roll_equal(fb_triad, fb));

    /* --- six-note: window includes D4 and F4 --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, hexa, 6, G_NOTE);
    paint();
    pitch_window(&cur_pat()->bass[b], &lo, &hi);
    CHECK(lo == 46 && hi == 67);
    report_column("hexachord", 0, hexa, 6);
    capture("after-hexachord");

    /* --- chord beside a mono step --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    set_chord(b, 7, mono_a, 1, G_NOTE);
    paint();
    report_column("chord-beside-mono/chord", 0, triad, 3);
    report_column("chord-beside-mono/mono", 7, mono_a, 1);
    capture("after-chord-and-mono");

    /* --- selected (held) chord: all pitches, symbol labels, no false notch --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    hold_step(0);
    paint();
    CHECK(ui.held_step == 0);
    CHECK(step_ink(0) == C_WHITE);
    report_column("selected-triad", 0, triad, 3);
    capture("after-selected-chord");
    time_paint("juno-held-triad", Ntime);
    {
        uint16_t fb_sel_chord[240 * 240];
        uint16_t fb_sel_mono[240 * 240];
        chord_label_t lab;
        copy_fb(fb_sel_chord);
        seq_ready(16, 0);
        b = bidx();
        set_chord(b, 0, mono, 1, G_NOTE);
        hold_step(0);
        paint();
        copy_fb(fb_sel_mono);
        capture("after-selected-mono");
        CHECK(!roll_equal(fb_sel_mono, fb_sel_chord));
        CHECK(!label_equal(fb_sel_mono, fb_sel_chord));
        CHECK(!plock_notch_at(0, 48, 46, 60));
        chord_label_fmt(&lab, triad, 3, triad, 3, "STEP 1  ", " V100");
        CHECK(strcmp(lab.cell, "C") == 0);
        CHECK(strcmp(lab.symbol, "C") == 0);
    }

    /* --- playing --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    seq.playing = 1;
    eng_step[TRK_BASS0 + b] = 0;
    paint();
    report_column("playing-triad", 0, triad, 3);
    capture("after-playing-chord");
    ph_ink = swap16(dim(part_col(), 2));
    x = (int)col_x(0);
    playhead_hits = 0;
    for (y = MAIN_Y + ROLL_TOP - 4; y < MAIN_Y + ROLL_BOT + 6; y++)
        if (fb[y * 240 + x + 1] == ph_ink || fb[y * 240 + x + 5] == ph_ink)
            playhead_hits++;
    printf("playing playhead_hits=%d\n", playhead_hits);
    if (meas) fprintf(meas, "playing playhead_hits=%d\n", playhead_hits);
    CHECK(playhead_hits >= 20);

    /* --- sustained triad, ties at each pitch --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    set_tie(b, 1, 48);
    set_tie(b, 2, 48);
    set_tie(b, 3, 48);
    seq.playing = 1;
    eng_step[TRK_BASS0 + b] = 2;
    paint();
    report_column("sustained/onset", 0, triad, 3);
    {
        const bpart_t *bp = &cur_pat()->bass[b];
        pitch_window(bp, &lo, &hi);
        CHECK(held_note_of(bp, 2) == 48);
        CHECK(note_start_of(bp, 2) == 0);
        CHECK(mark_at_pitch(2, 48, lo, hi, step_ink(2)));
        CHECK(mark_at_pitch(2, 52, lo, hi, step_ink(2)));
        CHECK(mark_at_pitch(2, 55, lo, hi, step_ink(2)));
    }
    capture("after-sustained-triad");

    /* --- slide: extras marked, slide stays root to root --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, (uint8_t)(G_NOTE | BS_SLIDE));
    set_chord(b, 1, mono_e, 1, G_NOTE);
    paint();
    report_column("slide/onset", 0, triad, 3);
    capture("after-slide-triad");

    /* --- WRITE line --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.rec = 1;
    ui.held_step = -1;
    ui.wpos[b] = 0;
    paint();
    capture("after-write-triad");
    {
        chord_label_t lab;
        chord_label_fmt(&lab, triad, 3, triad, 3, "WRITE 1  ", " V100");
        CHECK(lab.recognized);
        CHECK(strcmp(lab.symbol, "C") == 0);
        CHECK(strstr(lab.held, "WRITE 1  C  C3 E3 G3") == lab.held);
    }

    /* --- page 2 --- */
    seq_ready(32, 0);
    b = bidx();
    set_chord(b, 0, mono_a, 1, G_NOTE);
    set_chord(b, 16, page2, 3, G_NOTE);
    paint();
    report_column("page1-of-32", 0, mono_a, 1);
    capture("after-page1-with-page2-chord");
    ui.spage = 1;
    ui.held_step = -1;
    paint();
    report_column("page2-chord", 16, page2, 3);
    capture("after-page2-chord");
    hold_step(16);
    paint();
    CHECK(ui.held_step == 16);
    report_column("page2-selected", 16, page2, 3);
    capture("after-page2-selected-chord");
    ui.held_step = -1;
    seq.playing = 1;
    eng_step[TRK_BASS0 + b] = 16;
    paint();
    report_column("page2-playing", 16, page2, 3);
    capture("after-page2-playing-chord");

    /* --- Chicago and Miami mono: byte-identical frames vs this after render
     * will be compared to evidence/before PNGs by the wrapper. Hash the fb. */
    seq_ready_part(PART_303A, ORIGINAL_SOUND_CHICAGO, 16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    copy_fb(fb_chi);
    capture("after-chicago-mono");
    time_paint("chicago-mono", Ntime);

    seq_ready_part(PART_303B, ORIGINAL_SOUND_MIAMI, 16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    copy_fb(fb_mia);
    capture("after-miami-mono");
    time_paint("miami-mono", Ntime);
    CHECK(!fb_equal(fb_chi, fb_mia)); /* different engine name in the header */

    /* --- Drone 6-note: last 4 sound (G7), first 2 dim. */
    seq_ready_part(PART_303B, ORIGINAL_SOUND_DRONE, 16, 0);
    b = bidx();
    set_chord(b, 0, hexa, 6, G_NOTE);
    hold_step(0);
    paint();
    pitch_window(&cur_pat()->bass[b], &lo, &hi);
    CHECK(mark_at_pitch(0, 55, lo, hi, C_WHITE)); /* G3 sounds, held white */
    CHECK(mark_at_pitch(0, 59, lo, hi, C_WHITE));
    CHECK(mark_at_pitch(0, 62, lo, hi, C_WHITE));
    CHECK(mark_at_pitch(0, 65, lo, hi, C_WHITE));
    CHECK(mark_at_pitch(0, 48, lo, hi, C_DIM));   /* C3, E3 stored, not sounding */
    CHECK(mark_at_pitch(0, 52, lo, hi, C_DIM));
    {
        uint8_t sound[4] = {55, 59, 62, 65};
        uint8_t listed[6] = {48, 52, 55, 59, 62, 65};
        chord_label_t lab;
        uint8_t snd_midi[4], list_midi[6];
        int i;
        for (i = 0; i < 4; i++) snd_midi[i] = (uint8_t)chord_sound_midi(sound[i], 24);
        for (i = 0; i < 6; i++) list_midi[i] = (uint8_t)chord_sound_midi(listed[i], 24);
        chord_label_fmt(&lab, snd_midi, 4, list_midi, 6, "STEP 1  ", " V100");
        printf("DRONE hex symbol='%s' cell='%s'\n", lab.symbol, lab.cell);
        CHECK(strcmp(lab.symbol, "G7") == 0);
    }
    capture("after-drone-hexachord");

    /* --- playback identity (golden from unfixed 94197fc8). --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, (uint8_t)(G_NOTE | 0xF0u));
    {
        bpart_t *bp = &cur_pat()->bass[b];
        int s;
        for (s = 1; s <= 3; s++) {
            bp->step[s].note = 48;
            bp->step[s].flags = (uint8_t)(G_TIE | 0xF0u);
            bp->step[s].velocity = BASS_VELOCITY_DEFAULT;
            (void)held_chord_set(proj.arr.lane, seq.ppat[ui.part], b, s, 0, 0);
        }
    }
    proj.set.bt |= PROJ_BT_CLK_IN;
    seq.ppat[TRK_BASS0 + b] = 0;
    seq.mute = 0;
    {
        uint32_t ph = playback_hash();
        printf("PLAYBACK_HASH 0x%08x (golden 0x4c5a264a)\n", ph);
        if (meas) fprintf(meas, "PLAYBACK_HASH 0x%08x\n", ph);
        CHECK(ph == 0x4c5a264au);
    }
}

int main(int argc, char **argv)
{
    char mpath[1024];
    if (argc > 1)
        evidence_dir = argv[1];
    CHECK(snprintf(mpath, sizeof mpath, "%s/measurements.txt", evdir()) > 0);
    meas = fopen(mpath, "w");
    CHECK(meas);
    fprintf(meas, "SEQ chord display AFTER measurements (draw-only fix)\n");
    fprintf(meas, "host render of real ui.c draw_303 / draw_main / draw_knobs\n");
    fprintf(meas, "label function=chord_label_fmt TIER1=%d TIER2=%d\n",
            SEQ_CHORD_LABEL_TIER1, SEQ_CHORD_LABEL_TIER2);
    test_symbols();
#ifndef SEQ_CHORD_ROWS_ONLY
    test_mono_identity_and_chords();
#endif
    fclose(meas);
    meas = 0;
    printf("seq_chord_display: PASS (%u checks) evidence=%s\n", checks, evdir());
    return 0;
}
