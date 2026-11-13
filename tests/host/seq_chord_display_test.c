/* SPDX-License-Identifier: GPL-3.0-only */
/* Host reproduction of "chords do not show properly on screen in SEQ"
 * at 94197fc8. Compiles the real ui.c draw path and writes native 240x240
 * RGB565 BEFORE framebuffers. This file does not change firmware sources.
 *
 * Assertions document the current monophonic draw: extra held-chord pitches
 * are stored (held_chord_get) but never painted (draw_303 never calls it).
 * A later draw-only fix must invert the "extra pitch is absent" checks.
 */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"

static const char *evidence_dir;
static FILE *meas;

static const char *evdir(void)
{
    const char *d = evidence_dir;
    if (!d) d = getenv("SEQ_CHORD_EVIDENCE");
    if (!d) d = "evidence/before";
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

/* Force a full 240x240 frame from the real header, SEQ body and knob strip. */
static void paint(void)
{
    /* Draw only: ui_frame() runs input() and would clear a synthetic
     * ui.held_step (no physical key is down) and could advance a
     * synthetic playhead. The real draw_header/draw_main/draw_knobs
     * path is what this repro measures. */
    ui.help = 0;
    ui.msg_until = 0;
    ui.touched = -1;
    draw_header();
    draw_main();
    draw_knobs();
}

static void pitch_window(const bpart_t *bp, int *lo_out, int *hi_out)
{
    int c, lo = 127, hi = 0;
    for (c = 0; c < bp->len && c < NSTEPS; c++) {
        int start = note_start_of(bp, c);
        if (start >= 0) {
            int pitch = bp->step[start].note;
            if (pitch < lo) lo = pitch;
            if (pitch > hi) hi = pitch;
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

/* True when the 5-px night_mark body at this pitch is present in column col. */
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
    /* Centre row of the 5-px mark is solid ink (highlight is y+1, shadow y+3). */
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

/* Write a 1..6 note cell and require the store to report the same snapshot. */
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
    /* REC-while-playing form: TIE cells carry the root only, no extra lanes. */
    CHECK(held_chord_get(proj.arr.lane, seq.ppat[ui.part], b, step, root, got) == 1);
}

static void choose_juno(void)
{
    int voice = bidx();
    int32_t audio[512];
    int n;
    engine_bass_off(voice);
    CHECK(engine_bass_select_preset(voice, ORIGINAL_SOUND_JUNO, 0, &proj.sound) == 0);
    for (n = 0; n < 1024 && engine_bass_preset_pending(voice); n++)
        engine_render(audio, 256);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_sound_bank(&proj.sound, voice) == ORIGINAL_SOUND_JUNO);
    CHECK(engine_bass_bank(voice) == ORIGINAL_SOUND_JUNO);
}

static void seq_ready(int len, int spage)
{
    bpart_t *bp;
    setup(V_SEQ, PART_303B);
    choose_juno();
    ui.view = V_SEQ;
    ui.part = PART_303B;
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

/* Report each stored pitch: in-window?, mark present?, expected ink. */
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
        if (i == 0) {
            /* The primary (root) is the only pitch draw_303 paints. */
            CHECK(inside);
            CHECK(drawn);
        } else if (inside) {
            /* Extra pitches sit inside the 14-st window and are still omitted. */
            CHECK(!drawn);
        } else {
            /* Outside the primary-only window: would clip even if drawn. */
            CHECK(!drawn);
        }
    }
}

static void copy_fb(uint16_t *dst)
{
    memcpy(dst, fb, sizeof fb);
}

/* Pixels of the piano-roll body (not the AC/SL rows or the held-step label). */
static int roll_equal(const uint16_t *a, const uint16_t *b)
{
    int y;
    for (y = MAIN_Y + ROLL_TOP - 4; y <= MAIN_Y + ROLL_BOT + 6; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 240 * 2) != 0)
            return 0;
    return 1;
}

/* Chord lanes share the motion-lane pool. lock_marks(-1,-1,...) in draw_303
 * does not skip HELD_CHORD_TARGET, so a chord paints a 2x2 C_WHITE p-lock
 * notch at (col_x+5, note_y-3) on the root bar (ui.c 5578, 5620-5621,
 * lock_marks 3082-3096). Return 1 iff that notch is the only roll difference. */
static int roll_only_false_plock(const uint16_t *mono, const uint16_t *chord,
                                 int col, int root, int lo, int hi)
{
    int x = (int)col_x(col) + 5;
    int y = MAIN_Y + note_y(root, lo, hi) - 3;
    int yy, xx, diffs = 0, notch = 0;
    uint16_t white = swap16(C_WHITE);
    if (y < 0 || y + 1 >= 240 || x < 0 || x + 1 >= 240)
        return 0;
    for (yy = MAIN_Y + ROLL_TOP - 4; yy <= MAIN_Y + ROLL_BOT + 6; yy++) {
        for (xx = 0; xx < 240; xx++) {
            uint16_t a = mono[yy * 240 + xx], b = chord[yy * 240 + xx];
            if (a == b)
                continue;
            diffs++;
            if (xx >= x && xx <= x + 1 && yy >= y && yy <= y + 1 && b == white)
                notch++;
            else
                return 0;
        }
    }
    return diffs == 4 && notch == 4;
}

static int plock_notch_at(int col, int root, int lo, int hi)
{
    int x = (int)col_x(col) + 5;
    int y = MAIN_Y + note_y(root, lo, hi) - 3;
    uint16_t white = swap16(C_WHITE);
    return fb[y * 240 + x] == white && fb[y * 240 + x + 1] == white &&
           fb[(y + 1) * 240 + x] == white && fb[(y + 1) * 240 + x + 1] == white;
}

/* Label strip at y 122 in band coordinates, plus the NOTE knob cell. */
static int label_equal(const uint16_t *a, const uint16_t *b)
{
    int y;
    for (y = MAIN_Y + 118; y < MAIN_Y + 2 * BAND_H; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 240 * 2) != 0)
            return 0;
    for (y = KNOB_Y + 13; y < KNOB_Y + 68; y++)
        if (memcmp(a + 240 * y, b + 240 * y, 60 * 2) != 0) /* NOTE cell, left 60 px */
            return 0;
    return 1;
}

static void test_mono_identity_and_chords(void)
{
    /* MIDI: C3=48 E3=52 G3=55 B3=59 D4=62 F4=65 A3=57 C4=60 E4=64 G4=67 */
    static const uint8_t dyad[] = {48, 52};
    static const uint8_t triad[] = {48, 52, 55};
    static const uint8_t tetrad[] = {48, 52, 55, 59};
    static const uint8_t hexa[] = {48, 52, 55, 59, 62, 65};
    static const uint8_t mono[] = {48};
    static const uint8_t mono_e[] = {52};
    static const uint8_t mono_a[] = {57};
    static const uint8_t page2[] = {60, 64, 67};
    uint16_t fb_mono[240 * 240], fb_triad[240 * 240], fb_mono_e[240 * 240];
    int lo, hi, b, playhead_hits, x, y;
    uint16_t ph_ink;

    /* --- mono control at the same root, for identity compares --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, mono, 1, G_NOTE);
    paint();
    copy_fb(fb_mono);
    report_column("mono-C3", 0, mono, 1);
    capture("before-mono-control");

    /* A mono E3 lands on a different row: the roll can show that pitch. */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, mono_e, 1, G_NOTE);
    paint();
    copy_fb(fb_mono_e);
    report_column("mono-E3", 0, mono_e, 1);
    capture("before-mono-E3");
    CHECK(!roll_equal(fb_mono, fb_mono_e));

    /* --- 2-note chord --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, dyad, 2, G_NOTE);
    paint();
    report_column("dyad", 0, dyad, 2);
    capture("before-dyad");

    /* --- 3-note chord: roll and label match the mono-root control --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    paint();
    copy_fb(fb_triad);
    report_column("triad", 0, triad, 3);
    capture("before-triad");
    CHECK(roll_only_false_plock(fb_mono, fb_triad, 0, 48, 46, 60));
    CHECK(plock_notch_at(0, 48, 46, 60));
    if (meas) fprintf(meas, "triad vs mono-C3: only difference is false 2x2 p-lock notch (chord lanes in lock_marks)\n");

    /* The E3 row that WAS painted for mono E3 is empty in the triad frame. */
    pitch_window(&cur_pat()->bass[b], &lo, &hi);
    CHECK(in_window(52, lo, hi));
    CHECK(mark_at_pitch(0, 52, lo, hi, step_ink(0)) == 0);
    CHECK(mark_at_pitch(0, 48, lo, hi, step_ink(0)) == 1);

    /* --- 4-note chord, all four inside the primary-only window --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, tetrad, 4, G_NOTE);
    paint();
    report_column("tetrad", 0, tetrad, 4);
    capture("before-tetrad");

    /* --- six-note store-and-Juno limit: D4 and F4 sit above hi=60 --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, hexa, 6, G_NOTE);
    paint();
    pitch_window(&cur_pat()->bass[b], &lo, &hi);
    CHECK(lo == 46 && hi == 60);
    report_column("hexachord", 0, hexa, 6);
    capture("before-hexachord");

    /* --- chord beside a mono step --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    set_chord(b, 7, mono_a, 1, G_NOTE);
    paint();
    report_column("chord-beside-mono/chord", 0, triad, 3);
    report_column("chord-beside-mono/mono", 7, mono_a, 1);
    capture("before-chord-and-mono");

    /* --- selected (held) chord step: root turns white; extras still omitted;
     * the held-step label and NOTE knob name only the root. --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    ui.held_step = 0;
    ui.held_step_part = ui.part;
    ui.held_step_pat = seq.ppat[ui.part];
    paint();
    CHECK(ui.held_step == 0);
    CHECK(step_ink(0) == C_WHITE);
    report_column("selected-triad", 0, triad, 3);
    capture("before-selected-chord");
    {
        uint16_t fb_sel_chord[240 * 240];
        uint16_t fb_sel_mono[240 * 240];
        copy_fb(fb_sel_chord);
        seq_ready(16, 0);
        b = bidx();
        set_chord(b, 0, mono, 1, G_NOTE);
        ui.held_step = 0;
        ui.held_step_part = ui.part;
        ui.held_step_pat = seq.ppat[ui.part];
        paint();
        copy_fb(fb_sel_mono);
        capture("before-selected-mono");
        CHECK(roll_only_false_plock(fb_sel_mono, fb_sel_chord, 0, 48, 46, 60));
        CHECK(label_equal(fb_sel_mono, fb_sel_chord));
        if (meas) fprintf(meas, "selected triad vs selected mono: labels+NOTE knob identical; roll differs only by false p-lock notch\n");
    }

    /* --- playing chord step: playhead bar is drawn; extras still omitted --- */
    seq_ready(16, 0);
    b = bidx();
    set_chord(b, 0, triad, 3, G_NOTE);
    seq.playing = 1;
    eng_step[TRK_BASS0 + b] = 0;
    paint();
    report_column("playing-triad", 0, triad, 3);
    capture("before-playing-chord");
    ph_ink = swap16(dim(part_col(), 2));
    x = (int)col_x(0);
    playhead_hits = 0;
    for (y = MAIN_Y + ROLL_TOP - 4; y < MAIN_Y + ROLL_BOT + 6; y++)
        if (fb[y * 240 + x + 1] == ph_ink || fb[y * 240 + x + 5] == ph_ink)
            playhead_hits++;
    printf("playing playhead_hits=%d\n", playhead_hits);
    if (meas) fprintf(meas, "playing playhead_hits=%d\n", playhead_hits);
    CHECK(playhead_hits >= 20);

    /* --- sustained triad (NOTE + TIE TIE TIE), playhead on the third TIE --- */
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
    /* TIE columns: held_note_of walks back to the onset root, still one bar. */
    {
        const bpart_t *bp = &cur_pat()->bass[b];
        pitch_window(bp, &lo, &hi);
        CHECK(held_note_of(bp, 2) == 48);
        CHECK(note_start_of(bp, 2) == 0);
        CHECK(mark_at_pitch(2, 48, lo, hi, step_ink(2)));
        CHECK(!mark_at_pitch(2, 52, lo, hi, step_ink(2)));
        CHECK(!mark_at_pitch(2, 55, lo, hi, step_ink(2)));
    }
    capture("before-sustained-triad");

    /* --- page 2: a C4 major triad on step 16 (column 0 of spage 1).
     * A mono A3 on step 0 lives on page 1, so the page change is observable. */
    seq_ready(32, 0);
    b = bidx();
    set_chord(b, 0, mono_a, 1, G_NOTE);
    set_chord(b, 16, page2, 3, G_NOTE);
    paint();
    report_column("page1-of-32", 0, mono_a, 1);
    capture("before-page1-with-page2-chord");
    ui.spage = 1;
    ui.held_step = -1;
    paint();
    report_column("page2-chord", 16, page2, 3);
    capture("before-page2-chord");
    /* Selected chord on page 2. */
    ui.held_step = 16;
    ui.held_step_part = ui.part;
    ui.held_step_pat = seq.ppat[ui.part];
    paint();
    CHECK(ui.held_step == 16);
    report_column("page2-selected", 16, page2, 3);
    capture("before-page2-selected-chord");
    /* Playing chord on page 2. */
    ui.held_step = -1;
    seq.playing = 1;
    eng_step[TRK_BASS0 + b] = 16;
    paint();
    report_column("page2-playing", 16, page2, 3);
    capture("before-page2-playing-chord");
}

int main(int argc, char **argv)
{
    char mpath[1024];
    if (argc > 1)
        evidence_dir = argv[1];
    CHECK(snprintf(mpath, sizeof mpath, "%s/measurements.txt", evdir()) > 0);
    meas = fopen(mpath, "w");
    CHECK(meas);
    fprintf(meas, "SEQ chord display BEFORE measurements at 94197fc8\n");
    fprintf(meas, "host render of real ui.c draw_303 / draw_main / draw_knobs\n");
    test_mono_identity_and_chords();
    fclose(meas);
    meas = 0;
    printf("seq_chord_display: PASS (%u checks) evidence=%s\n", checks, evdir());
    return 0;
}
