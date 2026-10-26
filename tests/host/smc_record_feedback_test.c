/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise real panel/SMC selection and REC actions, then record keyboard notes
 * through production UI into the selected pattern. Compare emitted Record
 * lamps with that destination. Radio, hardware I/O and unused clip handlers
 * are substitutes; no physical-device or audio result is claimed. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
static uint8_t wire_led[128];
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    assert(len == 3);
    if (msg[0] == 0x90) wire_led[msg[1]] = msg[2];
    return 1;
}
#ifndef SMC_MIXER_SOURCE
#define SMC_MIXER_SOURCE "app/smc_mixer.c"
#endif
#include SMC_MIXER_SOURCE

static unsigned failures, writes, projections;

static void panel_input(void)
{
    now_ms++;
    input();
}

static void panel_button(int button)
{
    held_btn |= 1u << button;
    panel_input();
    held_btn &= ~(1u << button);
    panel_input();
}

static void panel_select(int part)
{
    for (int i = 0; ui.part != part && i < NPARTS; i++) {
        enc_acc[EN_ALGO] = ui.part < part ? 1 : -1;
        panel_input();
    }
    assert(ui.part == part);
}

static void smc_press(int note)
{
    const uint8_t down[3] = {0x90, (uint8_t)note, 127};
    const uint8_t up[3] = {0x80, (uint8_t)note, 0};
    smc_ble_midi(down, sizeof down);
    smc_ble_midi(up, sizeof up);
    smc_poll();
    assert((smc_rx_diag() >> 24) == 0);
}

static void record_feedback(const char *label)
{
    unsigned actual = 0;
    unsigned expected = ui.rec ? 1u << ui.part : 0;
    smc_frame();
    for (int part = 0; part < NPARTS; part++) {
        assert(wire_led[part] == 0 || wire_led[part] == 127);
        if (wire_led[part]) actual |= 1u << part;
    }
    projections++;
    if (actual != expected || wire_led[0x5F] != (ui.rec ? 127 : 0)) {
        failures++;
        fprintf(stderr, "FAIL %s: selected part=%u REC=%u emitted strip mask=%u expected=%u global=%u\n",
                label, ui.part, ui.rec, actual, expected, wire_led[0x5F]);
    }
}

static void record_note(const char *label, int key)
{
    assert(ui.rec && is_303() && !seq.playing);
    int selected = ui.part;
    if (ui.view != V_SEQ) panel_button(B_SEQ); /* SMC SELECT opens sound editing; step entry is visibly SEQ. */
    assert(ui.view == V_SEQ && ui.part == selected && ui.rec);
    int bass = bidx(), other = bass ^ 1;
    bpart_t *destination = &proj.pat[seq.ppat[ui.part]].bass[bass];
    bpart_t *untouched = &proj.pat[seq.ppat[PART_303A + other]].bass[other];
    bpart_t before_other = *untouched;
    int length = destination->len ? destination->len : 16;
    int step = ui.wpos[bass] % length;
    destination->step[step].note = 0;
    destination->step[step].flags = G_REST;
    held_keys |= 1u << key;
    panel_input();
    held_keys &= ~(1u << key);
    panel_input();
    assert(destination->step[step].note == kbd_note(key));
    assert(bstep_gate(&destination->step[step]) == G_NOTE);
    assert(ui.wpos[bass] == (step + 1) % length);
    assert(!memcmp(untouched, &before_other, sizeof before_other));
    writes++;
    printf("PASS actual recording: %s writes %s step=%d note=%u; other bass unchanged\n",
           label, PART_N[ui.part], step + 1, destination->step[step].note);
    record_feedback(label);
}

/* Separate consumer regression for the explicit SEQ/EDIT split in PR629.
 * Run smc_edit_no_write_test.sh against that UI; old pre-split EDIT legitimately
 * wrote steps, so this is an explicit consumer gate, not a behavior-based skip. */
static void edit_no_hidden_write(void)
{
    pattern_t *before = malloc(sizeof proj.pat);
    assert(before);
    const int parts[] = {PART_303A, PART_303B, PART_909, PART_808};
    for (unsigned n = 0; n < sizeof parts / sizeof *parts; n++) {
        if (ui.view != V_PART) panel_button(B_EDIT);
        panel_select(parts[n]);
        if (!ui.rec) panel_button(B_REC);
        assert(ui.view == V_PART && ui.rec && !seq.playing);
        memcpy(before, proj.pat, sizeof proj.pat);
        uint8_t positions[NBASS]; memcpy(positions, ui.wpos, sizeof positions);
        const int keys[] = {0, 4};
        for (unsigned k = 0; k < sizeof keys / sizeof *keys; k++) {
            held_keys |= 1u << keys[k]; panel_input();
            held_keys &= ~(1u << keys[k]); panel_input();
        }
        assert(!memcmp(before, proj.pat, sizeof proj.pat));
        assert(!memcmp(positions, ui.wpos, sizeof positions));
        assert(ui.view == V_PART && ui.part == parts[n] && ui.rec);
        printf("PASS EDIT no hidden write: %s, REC on, two key gestures, all patterns and bass positions unchanged\n",PART_N[parts[n]]);
        record_feedback("EDIT audition retains real Record feedback");
    }
    panel_button(B_REC); assert(!ui.rec);
    free(before);
}

int main(void)
{
    boot();
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link(1);
    smc_poll();
    memset(wire_led, 0xFF, sizeof wire_led);
    record_feedback("boot, recording off");

    /* Panel REC must report the real destination even before any SMC REC. */
    panel_button(B_SEQ);
    panel_select(PART_303A);
    panel_button(B_REC);
    record_note("panel REC on 303A", 0);

    /* SMC SELECT changes the actual recorder while REC remains enabled. */
    smc_press(24 + PART_303B);
    assert(ui.part == PART_303B && ui.rec);
    record_note("SMC SELECT changes active recorder to 303B", 2);
    smc_press(0x5F);
    assert(!ui.rec);
    record_feedback("SMC global REC off");

    /* Conversely, panel selection must replace the target of SMC-started REC. */
    smc_press(PART_303A);
    assert(ui.part == PART_303A && ui.rec);
    record_note("SMC strip REC on 303A", 4);
    panel_select(PART_303B);
    record_note("panel selection changes active recorder to 303B", 5);
    panel_button(B_REC);
    assert(!ui.rec);
    record_feedback("panel REC off");

    if (getenv("SMC_VERIFY_EDIT_NO_WRITE")) edit_no_hidden_write();
    printf("%s: %u real pattern writes, %u emitted Record projections, %u failures\n",
           failures ? "FAIL" : "PASS", writes, projections, failures);
    return failures ? 1 : 0;
}
