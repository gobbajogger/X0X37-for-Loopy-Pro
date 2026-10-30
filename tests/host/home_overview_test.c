/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual HOME content/assignment regression, derived from independent QA. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static int failures;
static const int display_position[5] = {0,1,4,3,2};
static const char *outdir;
static unsigned row_mask(int pattern, int part, int row)
{
    int i, x = 6 + pattern * 14 + (pattern >= 8 ? 4 : 0);
    unsigned mask = 0;
    int y = MAIN_Y + 5 + display_position[part] * 3 + row;
    for (i = 0; i < 8; ++i)
        if (fb[y * 240 + x + i]) mask |= 1u << i;
    return mask;
}
static void reset(void)
{
    int p;
    boot();
    for (p = 0; p < NPAT; ++p) pattern_init(&proj.pat[p], 1u, 2u);
    seq_init(&seq, proj.pat);
    ui_init();
    set_view(V_HOME);
    now_ms += 1000;
    audio_due_ms = (double)now_ms;
    ui.frame = 0;
}
static void render(const char *name)
{
    char path[1024];
    pattern_t before[NPAT]; memcpy(before, proj.pat, sizeof before);
    draw_header();
    draw_main();
    assert(!memcmp(before, proj.pat, sizeof before));
    snprintf(path, sizeof path, "%s/%s.png", outdir, name);
    png_write(path);
}
static void check(const char *name, int pattern, int part, unsigned expected_pb,
                  unsigned expected_mask)
{
    unsigned pb = pat_part_bits16(&proj.pat[pattern], part);
    unsigned a = row_mask(pattern, part, 0), b = row_mask(pattern, part, 1);
    printf("%s part=%d current=%d used=%d bits16=%04x pixels=%02x/%02x expected=%04x,%02x\n",
           name, part, seq.ppat[part] == pattern, part_used(&proj.pat[pattern], part),
           pb, a, b, expected_pb, expected_mask);
    if (pb != expected_pb || a != expected_mask || b != expected_mask) ++failures;
    if (!seq.playing && seq.ppat[part] == pattern && part >= PART_303A && part < PART_BRK) {
        unsigned lane_bits = 0;
        int y = MAIN_Y + BAND_H + (display_position[part] - 2) * 16 + 7;
        for (int step = 0; step < 16; ++step)
            if (fb[y * 240 + col_x(step) + 6] == swap16(PART_COL[part])) lane_bits |= 1u << step;
        if (lane_bits != expected_pb) {
            fprintf(stderr, "HOME bass lane mismatch: %s bits=%04x expected=%04x\n", name, lane_bits, expected_pb);
            ++failures;
        }
    }
    {
        int x = 4 + pattern * 14 + (pattern >= 8 ? 4 : 0);
        int y = MAIN_Y + 5 + display_position[part] * 3;
        uint16_t tick = fb[y * 240 + x];
        int active = seq.ppat[part] == pattern;
        /* Rounded ordinary/cue frames can shade this border; only an active
         * assignment may use this part colour on both tick pixels. */
        uint16_t expected = swap16(PART_COL[part]);
        if ((tick == expected) != active || (fb[(y + 1) * 240 + x] == expected) != active) {
            fprintf(stderr, "assignment tick mismatch: %s part %d\n", name, part); ++failures;
        }
    }
}
static void put_bits(pattern_t *pt, int part, unsigned bits, int offset)
{
    int s;
    for (s = 0; s < 16; ++s) if ((bits >> s) & 1u) {
        if (part < NKIT) sm_set(pt->drum[part].hit[s % NDRUM], offset + s);
        else if (part == PART_BRK) pt->brk.steps |= 1u << s;
        else { pt->bass[part - NKIT].step[offset + s].flags = G_NOTE;
               pt->bass[part - NKIT].step[offset + s].note = 48; }
    }
}
static void content_matrix(void)
{
    int p;
    reset();
    for (p = 0; p < NPARTS; ++p) {
        put_bits(&proj.pat[1], p, 0x8043u, 0); /* pairs 0,3,7 */
        put_bits(&proj.pat[2], p, 0xffffu, 0);
        put_bits(&proj.pat[3], p, 0x5555u, 0); /* 8 notes = all 8 pair-columns */
        if (p < NKIT) proj.pat[4].drum[p].len = 64;
        else if (p < PART_BRK) proj.pat[4].bass[p - NKIT].len = 64;
        if (p != PART_BRK) put_bits(&proj.pat[4], p, 1u, 16);
    }
    render("matrix-current-empty");
    for (p = 0; p < NPARTS; ++p) {
        check("empty-current", 0, p, 0, 0);
        check("empty-noncurrent", 5, p, 0, 0);
        check("sparse-noncurrent", 1, p, 0x8043, 0x89);
        check("dense-noncurrent", 2, p, 0xffff, 0xff);
        check("alternating-noncurrent", 3, p, 0x5555, 0xff);
        if (p != PART_BRK) check("later-only-noncurrent", 4, p, 0, 0x04);
    }
    /* Exercise the production HOME white-key handler, not a separate selection implementation. */
    key_event(WHITE_KEY[1], 1); key_event(WHITE_KEY[1], 0);
    run_ms(20); /* the audio/main handoff applies a stopped-transport cue */
    for (p = 0; p < NPARTS; ++p) assert(seq.ppat[p] == 1);
    render("matrix-current-sparse");
    for (p = 0; p < NPARTS; ++p) {
        check("sparse-current", 1, p, 0x8043, 0x89);
        check("empty-no-longer-current", 0, p, 0, 0);
    }
    key_event(WHITE_KEY[4], 1); key_event(WHITE_KEY[4], 0);
    run_ms(20);
    render("matrix-current-later-only");
    for (p = 0; p < NPARTS; ++p)
        check("later-only-current", 4, p, 0, p == PART_BRK ? 0 : 0x04);
    key_event(WHITE_KEY[2], 1); key_event(WHITE_KEY[2], 0);
    run_ms(20);
    render("matrix-current-dense");
    for (p = 0; p < NPARTS; ++p) check("dense-current", 2, p, 0xffff, 0xff);
    seq.playing = 1;
    key_event(WHITE_KEY[1], 1); key_event(WHITE_KEY[1], 0);
    assert(seq_cue_of(&seq, PART_909) == 1 && seq.ppat[PART_909] == 2);
    ui.frame = 16;
    render("matrix-cue-highlight");
    for (p = 0; p < NPARTS; ++p) check("cue-frame-does-not-fill", 1, p, 0x8043, 0x89);
}
static void tie_matrix(void)
{
    int b, s;
    reset();
    for (b = 0; b < NBASS; ++b) {
        bpart_t *all = &proj.pat[1].bass[b], *orphan = &proj.pat[2].bass[b];
        bpart_t *held = &proj.pat[3].bass[b], *wrap = &proj.pat[4].bass[b];
        for (s = 0; s < 16; ++s) all->step[s].flags = G_TIE;
        orphan->step[4].flags = G_TIE;
        held->step[3].flags = G_NOTE; held->step[3].note = 48;
        held->step[4].flags = held->step[5].flags = G_TIE;
        wrap->len = 64; wrap->step[63].flags = G_NOTE; wrap->step[63].note = 48;
        wrap->step[0].flags = wrap->step[1].flags = G_TIE;
        assert(note_start_of(all, 4) == -1);
        assert(note_start_of(orphan, 4) == -1);
        assert(note_start_of(held, 5) == 3);
        assert(note_start_of(wrap, 0) == 63);
    }
    render("ties-noncurrent");
    for (b = 0; b < NBASS; ++b) {
        check("all-tie-silent-noncurrent", 1, PART_303A+b, 0, 0);
        check("single-orphan-tie-noncurrent", 2, PART_303A+b, 0, 0);
        check("valid-note-and-hold-noncurrent", 3, PART_303A+b, 0x38, 6);
        check("valid-wrap-hold-noncurrent", 4, PART_303A+b, 3, 1);
    }
    key_event(WHITE_KEY[1], 1); key_event(WHITE_KEY[1], 0);
    run_ms(20);
    for (b = 0; b < NBASS; ++b) assert(seq.ppat[PART_303A+b] == 1);
    render("ties-current-all-tie");
    for (b = 0; b < NBASS; ++b) check("all-tie-silent-current", 1, PART_303A+b, 0, 0);
    for (int pat = 3; pat <= 4; ++pat) {
        key_event(WHITE_KEY[pat], 1); key_event(WHITE_KEY[pat], 0); run_ms(20);
        render(pat == 3 ? "ties-current-valid-hold" : "ties-current-wrap-hold");
        for (b = 0; b < NBASS; ++b)
            check(pat == 3 ? "valid-note-and-hold-current" : "valid-wrap-hold-current",
                  pat, PART_303A+b, pat == 3 ? 0x38 : 3, pat == 3 ? 6 : 1);
    }
}
static void editor_reachable_ties(void)
{
    int b, s;
    reset();
    for (b = 0; b < NBASS; ++b) {
        ui.part = PART_303A+b;
        set_view(V_SEQ);
        ui.rec = 1;
        ui.btn = 1u << B_LFO;
        for (s = 0; s < 16; ++s) button_tap(B_OCTUP);
        for (s = 0; s < 16; ++s) assert(bstep_gate(&proj.pat[0].bass[b].step[s]) == G_TIE);
        assert(note_start_of(&proj.pat[0].bass[b], 4) == -1);
    }
    ui.btn = 0; ui.rec = 0;
    set_view(V_HOME);
    render("ties-current-editor-created");
    for (b = 0; b < NBASS; ++b) check("all-tie-editor-created-current", 0, PART_303A+b, 0, 0);
}
int main(int argc, char **argv)
{
    assert(argc == 2); outdir = argv[1]; load_force = 0;
    setvbuf(stdout, 0, _IONBF, 0);
    content_matrix(); tie_matrix(); editor_reachable_ties();
    printf("HOME CONTENT CONTRACT %s: failures=%d\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
