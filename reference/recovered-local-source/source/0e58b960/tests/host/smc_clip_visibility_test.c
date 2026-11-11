/* Exercise the production SMC input queues, dispatch, encoder routing and clip
 * editor entry with menu/help covering the stage or its X0X fallback page. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "x0x_cold.h"
#include "app/smc_protocol.h"
#include "smc_fx_harness_stubs.h"
#define CK_FN __attribute__((unused))
#include "app/smk_clip_keys.h"

enum { O_NONE, O_LIST, O_ASK, R_NONE, R_HDR, R_ACT, R_ENG, EN_K1, NE,
       B_HOME, NKIT = 2, PROJ_BT_SMC_CENTRAL = 16, PROJ_BT_SMC_USER = 32 };
typedef struct { uint8_t kind; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct {
    uint8_t overlay, help;
    uint32_t act_t;
    uint32_t enc_t[NE];
    uint8_t enc_run[NE];
} ui;
static struct { uint8_t pg; } ck;
static uint8_t clip_grid_mode, fields[CK_NE];
static unsigned dirty, notices, fallback_turns;
static int page_value;
static uint32_t ck_get(uint32_t f) { return fields[f]; }
static void ck_set(uint32_t f, uint32_t v) { fields[f] = (uint8_t)v; }
static void mark_dirty(void) { dirty++; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; notices++; }
static void smc_ui_clip_show(int n, const char *a, const char *b)
{ (void)n; (void)a; (void)b; notices++; }
#define smc_ui_cancel() ((void)0) /* This fixture observes routing, not presentation. */
static char *put_s(char *d, const char *s) { while ((*d++ = *s++)) {} return d - 1; }
static const char *ck_val_txt(uint32_t f, uint32_t v, char *s)
{
    (void)f;
    sprintf(s, "%u", (unsigned)v);
    return s;
}
#include "clip_turn.inc"

static uint8_t perf_testing(void) { return 0; }
static int ui_project_busy(void) { return 0; }
static void button_tap(int button) { (void)button; }
static void engine_smc_solo(uint8_t solo) { (void)solo; }
static void smc_button(uint8_t note, int mmc) { (void)note; (void)mmc; }
static void smc_release(uint8_t note) { (void)note; }
static void smc_fader(int n, uint8_t value) { (void)n; (void)value; }
static pref_t smc_page_target(int n) { (void)n; return (pref_t){R_NONE}; }
static int smc_fader_of(pref_t r) { (void)r; return 0; }
static pref_t smc_knob_target(int n) { (void)n; return (pref_t){R_ENG}; }
static void turn_ref(pref_t r, int delta, int enc)
{
    (void)r; (void)enc;
    fallback_turns++;
    page_value += delta;
}
static uint32_t plat_ms(void) { return 1234u; }
static void smc_show(int n, int knob, pref_t r) { (void)n; (void)knob; (void)r; notices++; }
#include "smc_input.inc"
#include "smc_turn.inc"

static int fails, route;
static const char *const ROUTES[] = {"BLE DAW", "BLE DAW legacy bit", "BLE DAW plus retired host noise"};
static void check(const char *label, int ok)
{
    printf("%s %s page %u: %s\n", ok ? "PASS" : "FAIL", ROUTES[route], ck.pg & 3u, label);
    fails += !ok;
}
static void turn(int n, uint8_t value)
{
    uint8_t msg[3] = {0xB0u, (uint8_t)(16 + n), value};
    if (route == 2) {
        uint8_t before=smc_host_w;
        check("retired host leaves encoder message unconsumed", !smc_midi_host(0x0Bu | 0xBFu << 8 | (uint32_t)msg[1] << 16 | (uint32_t)msg[2] << 24));
        check("retired host cannot enqueue", smc_host_w==before);
    }
    smc_ble_midi(msg, sizeof msg);
    smc_poll();
}
static void covered(uint8_t overlay, uint8_t help)
{
    uint8_t before[CK_NE];
    unsigned d = dirty, nt = notices, ft = fallback_turns;
    int pv = page_value;
    uint32_t act = ui.act_t;
    memcpy(before, fields, sizeof before);
    ui.overlay = overlay;
    ui.help = help;
    for (int n = 0; n < 8; n++)
        turn(n, 1);
    if (overlay == O_LIST && !help) {
        check("menu owns encoders without editing covered clip fields",
              !memcmp(before, fields, sizeof before) && dirty == d &&
              fallback_turns == ft + 8 && page_value == pv + 8);
    } else if (help) {
        if (clip_grid_mode && !(ck.pg & 3u)) {
            /* Grid itself still swallows encoders; help must not become a second lock. */
            check("help leaves clip-grid encoders inactive",
                  !memcmp(before, fields, sizeof before) && dirty == d &&
                  fallback_turns == ft && page_value == pv && ui.help == 1);
        } else {
            check("help leaves encoders working under the card",
                  (dirty != d || fallback_turns != ft || page_value != pv ||
                   memcmp(before, fields, sizeof before)) && ui.help == 1);
        }
    } else {
        check("confirmation blocks clip fields and X0X fallback",
              !memcmp(before, fields, sizeof before) && dirty == d && notices == nt &&
              fallback_turns == ft && page_value == pv && ui.act_t == act);
    }
    check("covered encoder events are drained", smc_qr == smc_qw && smc_host_r == smc_host_w);
    memcpy(before, fields, sizeof before);
    d = dirty;
    ft = fallback_turns;
    pv = page_value;
    ui.overlay = O_NONE;
    ui.help = 0;
    smc_poll();
    check("closing the overlay does not replay turns", !memcmp(before, fields, sizeof before) &&
          dirty == d && fallback_turns == ft && page_value == pv);
}
int main(void)
{
    for (route = 0; route < 3; route++) {
        proj.set.bt = PROJ_BT_SMC_CENTRAL | (route ? PROJ_BT_SMC_USER : 0);
        smc_ble_link(1);
        smc_poll();
        clip_grid_mode = 1;
        for (unsigned page = 1; page <= 3; page++) {
            unsigned d = dirty, ft = fallback_turns;
            uint32_t f = CK_PAGE_FIELDS[page - 1][0];
            memset(fields, 0, sizeof fields);
            ck.pg = (uint8_t)page;
            turn(0, 1);
            check("visible stage edits its first field", fields[f] == 1 && dirty == d + 1 && fallback_turns == ft);
            covered(O_LIST, 0);
            covered(O_NONE, 1);
            covered(O_ASK, 0);
            turn(0, 0x41);
            check("visible stage resumes editing", fields[f] == 0 && fallback_turns == ft + 8);
        }
        ck.pg = 0;                    /* clip grid, with the normal X0X page underneath */
        covered(O_LIST, 0);
        covered(O_NONE, 1);
        clip_grid_mode = 0;            /* normal X0X page, with a stale stage selection */
        ck.pg = 1;
        covered(O_LIST, 0);
        covered(O_NONE, 1);
        {
            unsigned ft = fallback_turns;
            int pv = page_value;
            turn(0, 1);
            check("visible X0X fallback still edits", fallback_turns == ft + 1 && page_value == pv + 1);
        }
    }
    printf("smc_clip_visibility: %s (%d failures)\n", fails ? "failed" : "ok", fails);
    return fails != 0;
}
