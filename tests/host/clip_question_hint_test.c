/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual production UI/framebuffer, physical button filter and clip transport.
 * The established SMC fixture supplies hardware, clock, storage and clip-grid
 * substitutes. Generated includes replace only its named input substitutes. */
#define SMC_UI_MAIN clip_question_unused_smc_main
#include "clip_question_fixture.inc"
#include "app/smk_apc40.h"

static uint8_t apc_play_feedback, apc_record_feedback;
static unsigned apc_count, history_count;
static uint8_t apc_notes[8], apc_values[8];
static uint32_t history_packets[8];
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t saved) { (void)saved; }
static void midi_out_event(uint32_t packet)
{ if (history_count < 8) history_packets[history_count++] = packet; }
static void apc_note(uint8_t note, uint8_t value)
{
    if (apc_count < 8) {
        apc_notes[apc_count] = note;
        apc_values[apc_count++] = value;
    }
}
#include "clip_question_dispatch.inc"

static unsigned question_checks, question_failures, question_cases;
#define QCHECK(x) do { question_checks++; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
    question_failures++; } } while (0)

static uint16_t normal_frame[240 * 240], clip_frame[240 * 240];

static uint32_t pixels_hash(const uint16_t *pixels)
{
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < 240u * 240u; i++) {
        h = (h ^ (pixels[i] & 255u)) * 16777619u;
        h = (h ^ (pixels[i] >> 8)) * 16777619u;
    }
    return h;
}

static void input_edge(int role, int down)
{
    uint32_t bit = 1u << panel.btn[role];
    if (down) fm1_in.buttons |= bit;
    else fm1_in.buttons &= ~bit;
    input_body();
}
static void physical_tap(int role) { input_edge(role, 1); input_edge(role, 0); }

static void fresh_question(int clip)
{
    fm1_in.buttons = 0;
    setup(V_GLO, PART_909);
    clip_grid_mode = (uint8_t)clip;
    any_button = 1; /* GLO/LOAD was reached by the musician's controls. */
    global_section(4);
    ui.list_sel = 1; /* Actual project section's LOAD action. */
    ui.dirty = 1;
    list_enter();
    apc_count = history_count = 0;
    QCHECK(ui.overlay == O_ASK && ui.ask_act == ACT_LOAD);
    QCHECK(!strcmp(ui.ask_q[0], "LOAD SAVED PROJECT?"));
    QCHECK(!strcmp(ui.ask_q[1], "DISCARD UNSAVED EDITS"));
}

static void footer_contract(int clip)
{
    const char *a, *b, *c;
    (void)footer_text(&a, &b, &c);
    QCHECK(a && !strcmp(a, clip ? UI_CAP_HOME ": NO" : UI_CAP_SEL ": YES"));
    QCHECK(clip ? !b : b && !strcmp(b, UI_CAP_HOME ": NO"));
    QCHECK(!c);
}

static void render_contract(void)
{
    question_cases++;
    fresh_question(0);
    ui_frame();
    memcpy(normal_frame, fb, sizeof normal_frame);
    footer_contract(0);
    shot("question-normal");
    /* The host framebuffer retains the panel's byte-swapped RGB565 format. */
    unsigned affirmative = 0;
    for (unsigned y = MAIN_Y + BAND_H + 10; y < MAIN_Y + BAND_H + 34; y++)
        for (unsigned x = 22; x < 114; x++) affirmative += fb[y * 240 + x] != swap16(C_BLACK);
    QCHECK(affirmative > 1000); /* The existing normal confirmation stays visible. */

    clip_grid_mode = 1;
    ui_frame();
    memcpy(clip_frame, fb, sizeof clip_frame);
    footer_contract(1);
    shot("question-clip");
    unsigned clip_affirmative = 0, other_main_changes = 0;
    for (unsigned y = MAIN_Y; y < MAIN_Y + 2 * BAND_H; y++)
        for (unsigned x = 0; x < 240; x++) {
            int yes = x >= 22 && x < 114 && y >= MAIN_Y + BAND_H + 10 &&
                      y < MAIN_Y + BAND_H + 34;
            if (yes) clip_affirmative += fb[y * 240 + x] != swap16(C_BLACK);
            else other_main_changes += fb[y * 240 + x] != normal_frame[y * 240 + x];
        }
    QCHECK(!other_main_changes); /* Prompt, borders and NO affordance stay pixel-identical. */
    QCHECK(clip_affirmative == 0); /* Neither affirmative fill nor SEL/YES glyph survives. */
    QCHECK(memcmp(normal_frame + KNOB_Y * 240, clip_frame + KNOB_Y * 240,
                  KNOB_H * 240 * sizeof *fb)); /* Actual footer pixels must change too. */
    /* Compare the actual clip footer with the existing normal PATCH/NO glyphs.
     * Packing the first available action must not drop cancellation at a null segment. */
    int no_x = 4 + tw(&FONT_T, UI_CAP_SEL ": YES") + FOOT_GAP;
    int no_w = tw(&FONT_T, UI_CAP_HOME ": NO");
    unsigned footer_changes = 0, other_knob_changes = 0;
    cv_begin(240, KNOB_H, C_BLACK);
    night_panel(0, 70, 240, 12, 0); /* Actual existing footer material, including its edge. */
    for (unsigned y = KNOB_Y; y < KNOB_Y + KNOB_H; y++)
        for (unsigned x = 0; x < 240; x++) {
            if (y < KNOB_Y + FOOT_Y) {
                other_knob_changes += fb[y * 240 + x] != normal_frame[y * 240 + x];
            } else {
                uint16_t expected = x >= 4 && x < (unsigned)(4 + no_w)
                    ? normal_frame[y * 240 + no_x + x - 4] : cv_px[(y - KNOB_Y) * 240 + x];
                footer_changes += fb[y * 240 + x] != expected;
            }
        }
    QCHECK(!other_knob_changes);
    QCHECK(!footer_changes); /* PATCH NO remains visible; SCALE YES leaves no pixels. */
    QCHECK(ui.overlay == O_ASK && ui.ask_act == ACT_LOAD && ui.dirty);
    QCHECK(ui.list_sel == 1 && ui.glo_section == 4);

    clip_grid_mode = 0;
    ui_frame();
    footer_contract(0);
    shot("question-normal-return");
    QCHECK(!memcmp(normal_frame, fb, sizeof normal_frame));
    printf("PIXELS model=%s normal=%08x clip=%08x returned=%08x\n",
           SMK_MODEL, pixels_hash(normal_frame), pixels_hash(clip_frame), pixels_hash(fb));
}

static void light_contract(void)
{
    question_cases++;
    fresh_question(0);
    leds();
    uint32_t buttons = lit_btn, keys = lit_keys, dim = dim_keys;
    QCHECK(buttons & (1u << B_SEL));
    clip_grid_mode = 1;
    leds();
    QCHECK(!(lit_btn & (1u << B_SEL))); /* Masked physical SCALE must not suggest YES. */
    QCHECK((lit_btn & ~(1u << B_SEL)) == (buttons & ~(1u << B_SEL)));
    QCHECK(lit_keys == keys && dim_keys == dim);
    clip_grid_mode = 0;
    leds();
    QCHECK(lit_btn == buttons && lit_keys == keys && dim_keys == dim);

    question_cases++;
    ui.overlay = O_LIST; /* Existing non-question overlay lighting is unchanged. */
    leds();
    buttons = lit_btn;
    clip_grid_mode = 1;
    leds();
    QCHECK(lit_btn == buttons && (lit_btn & (1u << B_SEL)));
    QCHECK(lit_keys == keys && dim_keys == dim);
}

static void cancel_and_confirm(void)
{
    for (int clip = 0; clip < 2; clip++) {
        question_cases++;
        fresh_question(clip);
        sound_t before = proj.sound;
        if (clip) {
            physical_tap(B_SEL);
            QCHECK(ui.overlay == O_ASK && ui.dirty);
            QCHECK(!memcmp(&proj.sound, &before, sizeof before));
        }
        physical_tap(B_HOME); /* PATCH still answers NO in both modes. */
        QCHECK(ui.overlay == O_LIST && ui.glo_section == 4 && ui.list_sel == 1);
        QCHECK(ui.dirty && !memcmp(&proj.sound, &before, sizeof before));
        QCHECK(!strcmp(ui.msg, "CANCELLED"));
    }
    for (int controller = 0; controller < 2; controller++) {
        question_cases++;
        fresh_question(controller);
        sm_set(proj.pat[0].drum[0].hit[0], 0);
        ask(ACT_CLEAR_PAT, 0, "CLEAR THIS PATTERN?", "ALL FIVE PARTS");
        if (controller) { /* Clip SMC confirmation remains available despite physical SCALE masking. */
            message(0x90, 5, 127);
            message(0x90, 5, 0);
        } else physical_tap(B_SEL);
        QCHECK(ui.overlay == O_LIST);
        QCHECK(!sm_any(proj.pat[0].drum[0].hit[0]));
    }
}

static void play_contract(void)
{
    for (int clip = 0; clip < 2; clip++) for (int playing = 0; playing < 2; playing++) {
        question_cases++;
        fresh_question(clip);
        seq.playing = playing;
        seq.req_start = seq.req_stop = seq.req_cont = 0;
        physical_tap(B_PLAY);
        if (clip) {
            QCHECK(ui.overlay == O_ASK && ui.dirty);
            QCHECK(!seq.req_start && !seq.req_stop && !seq.req_cont);
            QCHECK(apc_count == 2 && apc_notes[0] == smk_apc40_transport_note(0));
            QCHECK(apc_values[0] == 127 && apc_values[1] == 0);
        } else {
            QCHECK(ui.overlay == O_LIST && !strcmp(ui.msg, "CANCELLED"));
            QCHECK(playing ? seq.req_stop && !seq.req_start : seq.req_start && !seq.req_stop);
            QCHECK(!apc_count);
        }
    }
    question_cases++;
    fresh_question(1);
    seq.req_start = seq.req_stop = seq.req_cont = 0;
    input_edge(B_PLAY, 1);
    clip_grid_mode = 0;
    input_edge(B_PLAY, 0); /* Release still belongs to the original clip transport. */
    QCHECK(apc_count == 2 && apc_values[0] == 127 && apc_values[1] == 0);
    QCHECK(ui.overlay == O_ASK && !seq.req_start && !seq.req_stop && !seq.req_cont);
}

int main(void)
{
    render_contract();
    light_contract();
    cancel_and_confirm();
    play_contract();
    printf("clip-question-hint model=%s: %u checks, %u cases, %u failures\n",
           SMK_MODEL, question_checks, question_cases, question_failures);
    return question_failures ? 1 : 0;
}
