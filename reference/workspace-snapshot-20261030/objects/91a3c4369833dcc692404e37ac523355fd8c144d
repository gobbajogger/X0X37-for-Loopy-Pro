/* Production UI/engine/SMC regression. Hardware I/O and unused clip functions
 * are substitutes. Direct messages bypass Bluetooth/HCI; host queues are real. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
void smc_poll(void);
void smc_frame(void);
int smc_connected(void);
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int d) { (void)n; (void)d; return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { (void)m; (void)n; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#ifndef SMC_SOURCE_FILE
#define SMC_SOURCE_FILE "app/smc_mixer.c"
#endif
#include SMC_SOURCE_FILE

enum { DAW, USER_DIRECT, HOST_NOTE, HOST_MMC0, HOST_MMC1, ROUTES };
static const char *const route_name[] = {"DAW", "direct User MMC", "host User note", "host User MMC0", "host User MMC1"};
static int route;
static uint8_t saved_project[sizeof proj];
static void queue(const uint8_t *m, uint8_t n)
{
    if (route >= HOST_NOTE && n == 3) {
        uint32_t packet = 0x0Bu | (uint32_t)m[0] << 8 | (uint32_t)m[1] << 16 | (uint32_t)m[2] << 24;
        assert(smc_midi_host(packet));
    } else
        smc_ble_midi(m, n);
}
static void note(int n)
{
    if (route == USER_DIRECT || route >= HOST_MMC0) {
        int cmd;
        for (cmd = 1; cmd <= 43 && smc_mmc_note(cmd) != n; cmd++) {}
        assert(cmd <= 43);
        if (route >= HOST_MMC0) smc_mmc_host(cmd, route == HOST_MMC1);
        else {
            uint8_t m[6] = {0xF0, 0x7F, 0x7F, 6, (uint8_t)cmd, 0xF7};
            queue(m, sizeof m);
        }
    } else {
        uint8_t m[3] = {route == HOST_NOTE ? 0x9F : 0x90, (uint8_t)n, 127};
        queue(m, sizeof m);
    }
}
static void edits(void)
{
    uint8_t knob[3] = {route ? 0xBF : 0xB0, 16, 1};
    uint8_t fader[3] = {route ? 0xBF : 0xE0, route ? 40 : 0, 127};
    note(16);                         /* mute */
    note(24);                         /* selection */
    note(0x5F);                       /* recording */
    note(14);                         /* menu */
    queue(knob, 3);
    queue(fader, 3);
}
static void render_block(void)
{
    int32_t block[512];
    engine_render(block, 256);
}
static void setup(void)
{
    assert(!perf_testing());
    memset(&ui, 0, sizeof ui);
    memset(&smc, 0, sizeof smc);
    smc_qr = smc_qw = smc_host_r = smc_host_w = 0;
    memset((void *)smc_mmc_r, 0, sizeof smc_mmc_r);
    memset((void *)smc_mmc_w, 0, sizeof smc_mmc_w);
    boot();
    proj.set.bt = PROJ_BT_SMC_CENTRAL | (route ? PROJ_BT_SMC_USER : 0);
    smc_ble_link(1);
    smc_poll();
    set_view(V_HOME);
    build_pages();
    seq.mute = 0x12345;
    seq.vmute = 0x400;
    seq.song_on = 1;
    seq_chain(&seq, 2, 7);
    seq.bpm = 139.0f;
    for (int p = 0; p < NPARTS; p++) seq.ppat[p] = (uint8_t)(p + 3);
    memcpy(saved_project, &proj, sizeof proj);
}
static void restore_and_check(void)
{
    assert(perf_testing() && perf.phase == 3 && !ui.rec);
    render_block();                    /* commit the requested transport stop */
    perf_test_tick();                  /* production deferred restoration */
    assert(!perf_testing() && perf.test == PT_IDLE && perf.phase == 4);
    assert(seq.pat == proj.pat && seq.mute == 0x12345 && seq.vmute == 0x400);
    assert(seq.song_on == 1 && seq.chain_a == 2 && seq.chain_b == 7 && seq.bpm == 139.0f);
    for (int p = 0; p < NPARTS; p++) assert(seq.ppat[p] == p + 3 && seq.pcue[p] == 0xFF);
    assert(!seq.playing && !seq.req_start && !seq.req_stop && !ui.rec);
    assert(!memcmp(saved_project, &proj, sizeof proj));
    smc_poll();                        /* drained inputs cannot replay after restoration */
    assert(!memcmp(saved_project, &proj, sizeof proj));
    assert(seq.mute == 0x12345 && seq.vmute == 0x400 && !ui.rec && !seq.req_start);
    note(16);
    smc_poll();
    assert(seq.mute != 0x12345);        /* ordinary input resumes after restoration */
}
static void abort_scenario(int phase, int key)
{
    setup();
    perf_test_start();
    if (phase >= 1) {
        perf_test_tick();
        render_block();
    }
    if (phase == 2) {
        now_ms += 1001;
        perf_test_tick();
    }
    assert(perf_testing() && perf.phase == phase);
    int view_before = ui.view, part_before = ui.part;
    edits();
    smc_poll();
    assert(perf.phase == phase && ui.view == view_before && ui.part == part_before && !ui.rec);
    assert(!memcmp(saved_project, &proj, sizeof proj));
    /* Releases must not abort. Stock MMC has no release event. */
    if (route == DAW || route == HOST_NOTE) {
        uint8_t release[3] = {route ? 0x8F : 0x80, (uint8_t)key, 0};
        queue(release, 3);
        smc_poll();
        assert(perf.phase == phase);
        release[0] = route ? 0x9F : 0x90;
        queue(release, 3);
        smc_poll();
        assert(perf.phase == phase);
    }
    note(key);
    edits();                           /* same queued batch after the abort */
    smc_poll();
    if (perf.phase != 3) {
        fprintf(stderr, "FAIL: %s key %02X phase %d did not abort\n", route_name[route], key, phase);
        exit(1);
    }
    assert(ui.view == view_before && ui.part == part_before && ui.overlay == O_NONE && !ui.rec);
    assert(!memcmp(saved_project, &proj, sizeof proj));
    note(0x5E);                        /* PLAY must not restart during pending restore */
    edits();
    smc_poll();
    assert(perf.phase == 3 && !seq.req_start && !ui.rec);
    restore_and_check();
}
static void start_abort_same_poll(void)
{
    setup();
    set_view(V_PERF);
    open_list_of_pages();
    for (ui.list_sel = 0; ui.list_sel < ui.list_n; ui.list_sel++)
        if (list_rows[ui.list_sel].kind == R_ACT && list_rows[ui.list_sel].a == ACT_PERF_TEST) break;
    assert(ui.list_sel < ui.list_n);
    note(5);                           /* start through the production SMC menu action */
    note(0x5D);
    edits();
    smc_poll();
    assert(perf_testing() && perf.phase == 3 && ui.view == V_PERF && ui.overlay == O_NONE);
    restore_and_check();
}
int main(void)
{
    for (route = 0; route < ROUTES; route++) {
        for (int phase = 0; phase < 3; phase++) {
            abort_scenario(phase, 0x5D);
            abort_scenario(phase, 0x5E);
            abort_scenario(phase, 6);
        }
        start_abort_same_poll();
        printf("PASS: %s STOP/PLAY/back abort phases 0-2; guards, deferred restore and queued start/abort\n", route_name[route]);
    }
    return 0;
}
