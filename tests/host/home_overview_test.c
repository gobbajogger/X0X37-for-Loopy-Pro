/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual HOME content/assignment regression, derived from independent QA. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static int failures;
static const char *outdir;
static unsigned row_mask(int pattern, int part, int row)
{
    int i, x = 6 + pattern * 14 + (pattern >= 8 ? 4 : 0);
    unsigned mask = 0;
    int y = MAIN_Y + 5 + part * 3 + row;
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
    {
        int x = 4 + pattern * 14 + (pattern >= 8 ? 4 : 0);
        int y = MAIN_Y + 5 + part * 3;
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
        if (p != PART_BRK) check("later-only-noncurrent", 4, p, 0, 0);
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
        check("later-only-current", 4, p, 0, 0);
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
int main(int argc, char **argv)
{
    assert(argc == 2); outdir = argv[1]; load_force = 0;
    setvbuf(stdout, 0, _IONBF, 0);
    content_matrix();
    printf("HOME CONTENT CONTRACT %s: failures=%d\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
