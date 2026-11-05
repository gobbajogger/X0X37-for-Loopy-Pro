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
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

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

static const int display_position[5] = {0,1,4,3,2};
static void panel_select(int part)
{
    for (int i = 0; ui.part != part && i < NPARTS; i++) {
        enc_acc[EN_ALGO] = display_position[ui.part] < display_position[part] ? 1 : -1;
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
    unsigned expected = ui.rec ? 1u << display_position[ui.part] : 0;
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
    assert(ui.rec && is_303() && !seq.playing && ui.view == V_SEQ);
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

static void record_render(void)
{
    int32_t block[512];
    engine_render(block, 256);
}

static void external_drum_onset(int voice)
{
    int note = GM_DRUM[0][voice];
    for (int down = 1; down >= 0; down--) {
        uint8_t status = (uint8_t)((down ? 0x90 : 0x80) | seq.ch[PART_909]);
        min_q[mi_w++ % MQ] = (uint32_t)(status >> 4) | (uint32_t)status << 8 |
                             (uint32_t)note << 16 | (uint32_t)(down ? 90 : 0) << 24;
        record_render();
    }
}

static unsigned recorded_drum_hits(int voice)
{
    unsigned hits = 0;
    for (int step = 0; step < NSTEPS; step++)
        hits += sm_get(proj.pat[seq.ppat[PART_909]].drum[0].hit[voice], step);
    return hits;
}

/* Omitting the SMC boundary admits the queued old onset; omitting immediate
 * recorder eligibility drops the fresh onset before the next input pass. */
static void record_boundary(const char *label, int rec_button)
{
    assert(!ui.rec);
    panel_select(PART_909);
    smc_press(rec_button);
    assert(ui.rec);
    seq.req_start = 1;
    record_render();
    panel_input();
    assert(seq.playing);
    memset(proj.pat[seq.ppat[PART_909]].drum[0].hit, 0,
           sizeof proj.pat[0].drum[0].hit);

    external_drum_onset(2);             /* queued, but not drained into the pattern */
    assert(recorded_drum_hits(2) == 0);
    smc_press(rec_button);
    assert(!ui.rec);
    external_drum_onset(4);             /* must not enter either recording session */
    smc_press(rec_button);
    assert(ui.rec);
    external_drum_onset(3);             /* before any panel input republishes REC */
    ext_rec_drain();

    unsigned old = recorded_drum_hits(2), fresh = recorded_drum_hits(3);
    unsigned off = recorded_drum_hits(4);
    int pass = old == 0 && fresh == 1 && off == 0;
    printf("%s REC boundary: %s old=%u fresh=%u off=%u\n",
           pass ? "PASS" : "FAIL", label, old, fresh, off);
    if (!pass) failures++;
    record_feedback(label);
    smc_press(rec_button);
    assert(!ui.rec);
    seq.req_stop = 1;
    record_render();
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
    panel_button(B_EDIT);
    panel_select(PART_303A);
    panel_button(B_SEQ); /* recording onsets belong to the visible SEQ page */
    assert(ui.view == V_SEQ);
    panel_button(B_REC);
    record_note("panel REC on 303A", 0);

    /* SMC SELECT changes the actual recorder while REC remains enabled. */
    smc_press(24 + display_position[PART_303B]);
    assert(ui.view == V_PART); /* SMC SELECT intentionally opens EDIT */
    panel_button(B_SEQ);      /* return to SEQ to make a pattern write */
    record_note("SMC SELECT changes active recorder to 303B", 2);
    smc_press(0x5F);
    assert(!ui.rec);
    record_feedback("SMC global REC off");

    /* Conversely, panel selection must replace the target of SMC-started REC. */
    smc_press(display_position[PART_303A]);
    record_note("SMC strip REC on 303A", 4);
    panel_select(PART_303B);
    record_note("panel selection changes active recorder to 303B", 5);
    panel_button(B_REC);
    assert(!ui.rec);
    record_feedback("panel REC off");

    record_boundary("SMC global REC", 0x5F);
    record_boundary("SMC strip REC", display_position[PART_909]);

    printf("%s: %u real pattern writes, %u emitted Record projections, %u failures\n",
           failures ? "FAIL" : "PASS", writes, projections, failures);
    return failures ? 1 : 0;
}
