/* Miami WBL1: AMP ADSR page reaches SMC knob targets (DEV wobble fixture). */
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
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
#include "../../../experiments/wobble/wobble.h"

static int amp_adsr_page(void)
{
    for (int p = 0; p < pg.n; p++)
        if (!strcmp(pg.title[p], "AMP ADSR"))
            return p;
    return -1;
}

int main(void)
{
    int p, i;
    boot();
    assert(engine_bass_is_wobble(0) && "DEV fixture: 303A is WBL1");
    ui.view = V_PART;
    ui.part = PART_303A;
    build_pages();
    p = amp_adsr_page();
    assert(p >= 0 && "Miami AMP ADSR page exists");
    assert(pg.r[p][0].c == W_ATTACK && pg.r[p][1].c == W_DECAY);
    assert(pg.r[p][2].c == W_SUSTAIN && pg.r[p][3].c == W_RELEASE);
    assert(pg.r[p][0].b == 0 && "303A voice index");
    for (i = 0; i < 4; i++) {
        const x0x_param_t *d = engine_param(T_303, 0, pg.r[p][i].c);
        assert(d && d->name[0]);
    }
    *page_of(V_PART) = (uint8_t)p;
    build_pages();
    for (i = 0; i < 4; i++)
        assert(smc_same(smc_knob_target(i), pg.r[p][i]));
    puts("PASS miami amp adsr smc knobs");
    return 0;
}
