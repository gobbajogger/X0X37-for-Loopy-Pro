/* SPDX-License-Identifier: GPL-3.0-only */
/* The 16 SMK-37 pads are the 16 white step keys. With a settings list (O_LIST) or a
 * HELP card open, each pad does what it does with the settings closed, and the pad
 * LEDs show the same picture. Menu navigation stays on the buttons and knobs. */
enum { CX_NONE, CX_LIST, CX_HELP, CX_N };
static const char *const CXN[CX_N] = {"closed", "settings list", "HELP"};
static unsigned checks;
#define T(c, ...) do { checks++; CHECK(c, __VA_ARGS__); } while (0)

static void close_all(void)
{
    held_keys = 0; held_btn = 0; tick(30);
    ui.overlay = O_NONE; ui.help = 0; build_pages(); tick(30);
}
/* Open the context. HELP: one button held (FX) until its card shows, as on the device. */
static void open_cx(int cx)
{
    if (cx == CX_LIST) { open_list_of_pages(); tick(20); T(ui.overlay == O_LIST, "list open"); }
    if (cx == CX_HELP) { btn(B_FX, 1); tick(HELP_HOLD + 50); T(ui.help == B_FX + 1, "HELP open"); }
}
static void leave_cx(int cx)
{
    if (cx == CX_HELP) btn(B_FX, 0);
    tick(20);
}
static void same_picture(int cx, const char *page)
{
    uint32_t lit, dim, l2, d2;
    held_keys = 0; tick(20); lit = lit_keys; dim = dim_keys;
    open_cx(cx); tick(20); l2 = lit_keys; d2 = dim_keys;
    T(l2 == lit && d2 == dim, "%s: pad LEDs %08x/%08x with %s, %08x/%08x closed", page, l2, d2, CXN[cx], lit, dim);
    leave_cx(cx); close_all();
}
static void tap_pad(int w) { key(WHITE_KEY[w], 1); tick(15); key(WHITE_KEY[w], 0); tick(15); }

static void drum_seq(int cx)
{
    ui.part = PART_909; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "909 SEQ");
    for (int w = 0; w < 16; w++) {
        uint32_t before = drum_step_lights(PART_909);
        open_cx(cx); tap_pad(w); leave_cx(cx);
        uint32_t after = drum_step_lights(PART_909);
        T(((before ^ after) >> WHITE_KEY[w]) & 1u, "909 SEQ pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
static void drum_mix(int cx)
{
    ui.part = PART_909; set_view(V_MIX); build_pages();
    int p;
    for (p = 0; p < pg.n; p++) { *page_of(V_MIX) = (uint8_t)p; build_pages(); if (drum_mix_page() >= 0) break; }
    T(drum_mix_page() >= 0, "a drum MIX page exists");
    int kit = drum_mix_page();
    close_all();
    same_picture(cx, "drum MIX");
    for (int w = 0; w < 16; w++) {
        uint32_t before = drum_step_lights(kit);
        open_cx(cx); tap_pad(w); leave_cx(cx);
        T(((before ^ drum_step_lights(kit)) >> WHITE_KEY[w]) & 1u, "drum MIX pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
static void bass_seq(int cx, int bank)
{
    T(!engine_bass_select_preset(0, bank, 0, &proj.sound), "bank"); tick(200);
    ui.part = PART_303A; ui.freeplay[0] = 0; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "synth SEQ");
    for (int w = 0; w < 16; w++) {
        bpart_t *bp = &cur_pat()->bass[0];
        int s = ui.spage * 16 + w, before = s < bp->len ? bstep_gate(&bp->step[s]) : G_REST;
        open_cx(cx); tap_pad(w); leave_cx(cx);
        int after = s < bp->len ? bstep_gate(&cur_pat()->bass[0].step[s]) : G_REST;
        T(after != before, "synth bank %d SEQ pad %d edits its step with %s (%d -> %d)", bank, w + 1, CXN[cx], before, after);
        close_all();
    }
}
static void bass_play(int cx, int bank, int view, int freeplay)
{
    T(!engine_bass_select_preset(0, bank, 0, &proj.sound), "bank"); tick(200);
    ui.part = PART_303A; ui.freeplay[0] = (uint8_t)freeplay; set_view(view); build_pages(); close_all();
    same_picture(cx, view == V_PART ? "synth EDIT" : "synth SEQ FREE PLAY");
    for (int w = 0; w < 16; w++) {
        open_cx(cx);
        key(WHITE_KEY[w], 1); tick(15);
        T(engine_bass_keys_held() > 0, "synth bank %d %s pad %d plays its note with %s", bank,
          view == V_PART ? "EDIT" : "FREE PLAY", w + 1, CXN[cx]);
        key(WHITE_KEY[w], 0); tick(15);
        T(engine_bass_keys_held() == 0, "pad %d release ends the note", w + 1);
        leave_cx(cx); close_all();
    }
}
static void brk_seq(int cx)
{
    ui.part = PART_BRK; set_view(V_SEQ); build_pages(); close_all();
    same_picture(cx, "BREAK SEQ");
    for (int w = 0; w < 16; w++) {
        uint32_t before = cur_pat()->brk.steps;
        open_cx(cx); tap_pad(w); leave_cx(cx);
        T(((before ^ cur_pat()->brk.steps) >> w) & 1u, "BREAK SEQ pad %d toggles its step with %s", w + 1, CXN[cx]);
        close_all();
    }
}
int main(void)
{
    boot(); tick(300);
    seq_stop(&seq); tick(50);
    for (int cx = CX_NONE; cx < CX_N; cx++) {
        drum_seq(cx);
        drum_mix(cx);
        brk_seq(cx);
        for (int bank = FACTORY_BANK_CHICAGO; bank <= FACTORY_BANK_MIAMI; bank++) {
            bass_seq(cx, bank);
            bass_play(cx, bank, V_PART, 0);
            bass_play(cx, bank, V_SEQ, 1);
        }
    }
    printf("%s %u checks: pads x {909 SEQ, drum MIX, BREAK, Chicago/Miami SEQ/EDIT/FREE PLAY} x {closed, settings list, HELP}\n",
           fails ? "FAIL" : "PASS", checks);
    return fails != 0;
}
