/* SPDX-License-Identifier: GPL-3.0-only */
/* M1 keyboard FX+SEQ and SMC BANK L+R routes on the host app.
 * Device and native builds are not this test. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
int engine_part_sounding(void);

static int fails;
static int clip_stops;
#define CHECK(c) do { \
    if (!(c)) { fails++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } \
} while (0)

uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
uint8_t smk_clip_transport_button(uint8_t button, uint8_t pressed)
{ (void)button; (void)pressed; return 0; }
uint8_t smk_clip_history_button(uint8_t button, uint8_t pressed, uint8_t patch_held)
{ (void)button; (void)pressed; (void)patch_held; return 0; }
void smk_play_note_button(uint8_t pressed, uint8_t home_held)
{ (void)pressed; (void)home_held; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *last, uint32_t force) { (void)last; (void)force; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t note) { (void)note; return 0; }
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t sixteenths) { (void)sixteenths; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *msg, uint8_t len)
{ (void)slot; (void)msg; (void)len; return 1; }
void smk_ck_midi_stop(void) { clip_stops++; }

#include "app/smc_mixer.c"

static void settle(void) { run_ms(8); }
static void tap(int button)
{
    held_btn |= 1u << button;
    settle();
    held_btn &= ~(1u << button);
    settle();
}
static void down(int button)
{
    held_btn |= 1u << button;
    settle();
}
static void up(int button)
{
    held_btn &= ~(1u << button);
    settle();
}
static void note3(uint8_t status, uint8_t note, uint8_t vel)
{
    uint8_t msg[3] = {status, note, vel};
    smc_ble_midi(msg, 3);
    smc_poll();
    settle();
}
static void prepare_line(void)
{
    pattern_t *pat = &proj.pat[0];
    bpart_t *bp = &pat->bass[0];
    int i, kit, drum, word;
    pat->brk.steps = 0;
    for (kit = 0; kit < NKIT; kit++)
        for (drum = 0; drum < NDRUM; drum++)
            for (word = 0; word < NSW; word++)
                pat->drum[kit].hit[drum][word] = 0;
    bp->len = 16;
    bp->rate = 0;
    bp->dir = DIR_FWD;
    for (i = 0; i < 16; i++)
        bp->step[i] = (bstep_t){99, G_TIE, 100};
    bp->step[0] = (bstep_t){36, G_NOTE, 110};
    bp->step[8] = (bstep_t){48, G_NOTE, 110};
    for (i = 0; i < NTRACKS; i++)
        seq.ppat[i] = 0;
    seq.send_notes = 1;
    seq.send_clock = 1;
    seq.bpm = 150.0f;
}
static void play_gate(void)
{
    int i;
    seq_start(&seq);
    for (i = 0; i < 400 && !(engine_part_sounding() & 2); i++)
        run_ms(1);
    CHECK(seq.playing == 1);
    CHECK(engine_part_sounding() & 2);
}
static void connect_smc(int user)
{
    proj.set.bt = (uint8_t)(PROJ_BT_SMC_CENTRAL | (user ? PROJ_BT_SMC_USER : 0));
    smc_ble_link(1);
    smc_poll();
}

static void keyboard_route(void)
{
    uint32_t transport;
    int stops;
    boot();
    prepare_line();
    CHECK(ui.view == V_SEQ);
    play_gate();
    tap(B_OCTUP);
    CHECK(seq.playing == 1);
    CHECK(engine_part_sounding() & 2);
    CHECK(clip_stops == 0);
    tap(B_SEQ);
    CHECK(ui.view == V_SEQ);
    CHECK(engine_part_sounding() & 2);
    tap(B_FX);
    CHECK(ui.view == V_FX);
    CHECK(engine_part_sounding() & 2);
    transport = transport_out_count;
    stops = clip_stops;
    down(B_FX);
    down(B_SEQ);
    CHECK(ui.view == V_FX);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK((engine_part_sounding() & 2) == 0);
    CHECK(clip_stops == stops + 1);
    CHECK(transport_out_count == transport);
    up(B_FX);
    up(B_SEQ);
    CHECK(ui.view == V_FX);
    CHECK(clip_stops == stops + 1);
    down(B_SEQ);
    up(B_SEQ);
    CHECK(ui.view == V_SEQ);
    CHECK(clip_stops == stops + 1);
    down(B_FX);
    down(B_SEQ);
    CHECK(clip_stops == stops + 2);
    CHECK(seq.playing == 1);
    up(B_FX);
    up(B_SEQ);
    {
        int i, returned = 0;
        for (i = 0; i < 1200; i++) {
            run_ms(1);
            if (engine_part_sounding() & 2) {
                returned = 1;
                break;
            }
        }
        CHECK(returned);
        CHECK(seq.playing == 1);
        CHECK(seq.t[TRK_BASS0].pos == 8);
    }
}

static void smc_route(void)
{
    int stops;
    uint8_t part;
    boot();
    clip_stops = 0;
    prepare_line();
    play_gate();
    set_view(V_HOME); /* BANK L/R move parts here; drum SEQ keeps its drum-select action */
    connect_smc(0);
    part = ui.part;
    note3(0x90, 0x2F, 127);
    CHECK(ui.part != part);
    CHECK(engine_part_sounding() & 2);
    CHECK(clip_stops == 0);
    note3(0x80, 0x2F, 0);
    note3(0x90, 0x31, 127);
    note3(0x90, 0x30, 127);
    CHECK(clip_stops == 0);
    CHECK(engine_part_sounding() & 2);
    note3(0x80, 0x31, 0);
    note3(0x80, 0x30, 0);
    stops = clip_stops;
    part = ui.part;
    note3(0x90, 0x2F, 127);
    note3(0x90, 0x2E, 127);
    CHECK(clip_stops == stops + 1);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK((engine_part_sounding() & 2) == 0);
    CHECK(ui.part == part_neighbor(part, 1));
    note3(0x80, 0x2E, 0);
    note3(0x90, 0x2E, 127);
    CHECK(clip_stops == stops + 1);
    note3(0x80, 0x2F, 0);
    note3(0x80, 0x2E, 0);
    connect_smc(1);
    play_gate();
    stops = clip_stops;
    {
        uint8_t bank_r[3] = {0x90, 0x2F, 127};
        uint8_t bank_l[3] = {0x90, 0x2E, 127};
        smc_ble_midi(bank_r, 3);
        smc_ble_midi(bank_l, 3);
        smc_poll();
        settle();
    }
    CHECK(clip_stops == stops);
    CHECK(engine_part_sounding() & 2);
    note3(0x80, 0x2F, 0);
    note3(0x80, 0x2E, 0);
    {
        uint8_t mmc_l[6] = {0xF0, 0x7F, 0x7F, 0x06, 38, 0xF7};
        uint8_t mmc_r[6] = {0xF0, 0x7F, 0x7F, 0x06, 39, 0xF7};
        smc_ble_midi(mmc_l, 6);
        smc_ble_midi(mmc_r, 6);
        smc_poll();
        settle();
    }
    CHECK(clip_stops == stops);
    CHECK(seq.playing == 1);
    CHECK(engine_part_sounding() & 2);
    note3(0x90, 0x2E, 127);
    proj.set.bt = (uint8_t)(proj.set.bt & (uint8_t)~PROJ_BT_SMC_USER);
    note3(0x90, 0x2F, 127);
    CHECK(clip_stops == stops);
    CHECK(engine_part_sounding() & 2);
    note3(0x80, 0x2E, 0);
    note3(0x80, 0x2F, 0);
    note3(0x90, 0x2E, 127);
    note3(0x90, 0x2F, 127);
    CHECK(clip_stops == stops + 1);
    CHECK(seq.playing == 1);
    smc_ble_link(0);
    smc_poll();
    smc_ble_link(1);
    smc_poll();
    note3(0x90, 0x2F, 127);
    CHECK(clip_stops == stops + 1);
    note3(0x80, 0x2F, 0);
}

static void peer_bank(unsigned peer, uint8_t note, int down)
{
    uint8_t msg[3] = {down ? 0x90 : 0x80, note, down ? 127 : 0};
    smc_ble_midi_slot(peer, msg, 3);
    smc_poll();
    settle();
}

static void independent_peer_chords(void)
{
    boot();
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link_slot(0, 0);
    smc_ble_link_slot(1, 0);
    smc_poll();
    smc_ble_link_slot(0, 1);
    smc_ble_link_slot(1, 1);
    smc_poll();
    clip_stops = 0;
    peer_bank(0, 0x2E, 1);
    peer_bank(1, 0x2F, 1);
    CHECK(clip_stops == 0); /* Different controllers cannot form one chord. */
    peer_bank(0, 0x2F, 1);
    CHECK(clip_stops == 1);
    peer_bank(0, 0x2E, 0);
    peer_bank(0, 0x2F, 0);
    smc_ble_link_slot(0, 0);
    smc_poll();
    peer_bank(1, 0x2E, 1);
    CHECK(clip_stops == 2); /* The other peer's disconnect cannot clear this hold. */
    peer_bank(1, 0x2E, 0);
    peer_bank(1, 0x2F, 0);
}

static void lost_bank_release(void)
{
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    for (unsigned peer = 0; peer < SMC_PEERS; peer++) {
        uint8_t filler[3] = {0x90, 0x40, 127};
        uint8_t release[3] = {0x80, 0x2E, 0};
        smc_ble_link_slot(peer, 0);
        smc_poll();
        smc_ble_link_slot(peer, 1);
        smc_poll();
        int stops = clip_stops;
        peer_bank(peer, 0x2E, 1);
        uint32_t lost = smc_inputs[peer].release_lost[0];
        for (unsigned i = 0; i < SMC_Q; i++)
            smc_ble_midi_slot(peer, filler, sizeof filler);
        smc_ble_midi_slot(peer, release, sizeof release);
        CHECK(smc_inputs[peer].release_lost[0] == lost + 1u);
        smc_poll();
        settle();
        peer_bank(peer, 0x2F, 1);
        CHECK(clip_stops == stops);
        peer_bank(peer, 0x2E, 1);
        CHECK(clip_stops == stops + 1);
        peer_bank(peer, 0x2E, 0);
        peer_bank(peer, 0x2F, 0);
    }
}

int main(void)
{
    keyboard_route();
    smc_route();
    independent_peer_chords();
    lost_bank_release();
    if (fails) {
        fprintf(stderr, "panic route: %d failures\n", fails);
        return 1;
    }
    printf("panic route: 0 failures\n");
    return 0;
}
