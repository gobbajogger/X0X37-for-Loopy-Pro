/* Production-extracted test of temporary mixer screen peer context.
 * The companion Python tool includes current smc_mixer.c function bodies. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_mixer_edges.h"

#define SMC_PEERS 2u
#define BOARD_SMK37 0
#define O_NONE 0
#define O_ASK 2

typedef struct {
    uint8_t held[16];
    uint8_t locked, lock_view, lock_part, lock_page, knob_bank;
} smc_state_t;
static smc_state_t smc_states[SMC_PEERS];
static uint8_t smc_slot;
#define smc (smc_states[smc_slot])
static struct {
    uint8_t view,part,mixsel,sel[2],overlay,help,list_sel,list_top,spage,pbank;
} ui;
static unsigned project_revision = 13;
static uint32_t clock_ms;
static uint32_t blit_hash[4] = {1,2,3,4};
static int busy,clip;
static int smk_clip_mode_on(void) { return clip; }
static uint32_t plat_ms(void) { return clock_ms; }
static int ui_project_busy(void) { return busy; }
static int cur_page(void) { return ui.view & 3; }
static int smc_page_now(void) { return smc.locked ? smc.lock_page : cur_page(); }

#include "smc_fx_ui_extract.inc"

static unsigned checks;
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr,"FAIL check %u %s:%d: %s\n",checks,__FILE__,__LINE__,#expr);return 1; \
} } while (0)
static void reset_all(void)
{
    memset(smc_states,0,sizeof smc_states);
    memset(&ui,0,sizeof ui);
    memset(smc_ui_raw,0,sizeof smc_ui_raw);
    smc_slot=0; clock_ms=0; busy=clip=0;
}
int main(void)
{
    reset_all();
    smc_states[0].knob_bank=0;
    smc_states[1].knob_bank=2;
    clock_ms=123u;
    smc_slot=1; smc_ui_begin(3,1);
    CHECK(smc_ui.on && smc_ui.peer == 1);
    smc_slot=0; /* same as production smc_poll() restoring saved peer */
    CHECK(smc_ui_active() == 1); /* must use originating peer state */
    CHECK(smc_slot == 0);
    CHECK(smc_ui_active() == 1);
    smc_states[0].knob_bank=3; /* other peer changes bank: no cancellation */
    CHECK(smc_ui_active() == 1);
    clock_ms+=1201;
    CHECK(smc_ui_active() == 0); /* timeout */
    CHECK(!blit_hash[0] && !blit_hash[3]);

    reset_all(); smc_states[0].knob_bank=1; smc_states[1].knob_bank=2;
    clock_ms=500; smc_slot=1; smc_ui_begin(3,3); smc_slot=0;
    CHECK(smc_hold_press(smc_states[1].held,29));
    clock_ms+=2000;
    CHECK(smc_ui_active() == 1); /* original peer holds FX */
    CHECK(smc_hold_release(smc_states[1].held,29));
    CHECK(smc_ui_active() == 0); /* original peer releases past idle deadline */

    reset_all(); smc_states[1].knob_bank=2;
    clock_ms=UINT32_MAX-500u; smc_slot=1; smc_ui_begin(1,1);smc_slot=0;
    clock_ms+=1100u;
    CHECK(smc_ui_active() == 1); /* deadline wraps */
    clock_ms+=101u;
    CHECK(smc_ui_active() == 0);

    reset_all(); smc_states[1].knob_bank=2;
    clock_ms=200; smc_slot=1; smc_ui_begin(1,1);smc_slot=0;
    smc_states[1].knob_bank=3;
    CHECK(smc_ui_active() == 0); /* original peer changed bank */
    reset_all(); smc_slot=1; smc_ui_begin(1,1);smc_slot=0;
    ui.view=1;
    CHECK(smc_ui_active() == 0); /* global view navigation cancels */
    reset_all(); smc_slot=1;smc_ui_begin(1,1);smc_slot=0;
    ui.overlay=O_ASK;
    CHECK(smc_ui_active() == 0); /* question owns screen */
    reset_all(); smc_slot=1;smc_ui_begin(1,1);smc_slot=0;
    smc_ui.peer=SMC_PEERS;
    CHECK(smc_ui_active() == 0); /* corrupted peer fails closed */
    printf("PASS: %u exact-source SMC peer UI assertions\n",checks);
    return 0;
}
