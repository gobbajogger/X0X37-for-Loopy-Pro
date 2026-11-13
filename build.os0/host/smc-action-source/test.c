/* Host-only action and UI test with verbatim production function bodies.
 * This is a scoped compile test, not a full firmware or device simulation. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "app/smc_mixer_edges.h"

enum { O_NONE=0, O_ASK=1, NPARTS=5, SMC_UI_MS=1200 };
typedef struct { uint8_t held[16],home_down,select_down,play_down,solo_target[8]; } smc_state_t;
static smc_state_t smc_states[2];
static uint8_t smc_slot;
#define smc (smc_states[smc_slot])
static struct {char msg[32]; uint32_t msg_until;uint8_t help,overlay;} ui;
static struct {uint32_t until,context; uint8_t on,kind,strip,action,peer;} smc_ui;
static uint32_t now;
static unsigned apply_count,lock_down_count,lock_up_count,fx_release_count;
static uint32_t plat_ms(void){return now;}
static int ui_project_busy(void){return 0;}
static uint32_t smc_ui_context(void){return 9u;}
static void smc_ui_cancel(void){smc_ui.on=0;}
static void smc_ui_begin(int strip,int kind){smc_ui.on=1;smc_ui.strip=(uint8_t)strip;smc_ui.kind=(uint8_t)kind;smc_ui.peer=smc_slot;smc_ui.until=now+SMC_UI_MS;smc_ui.context=9u;}
static int smc_lock_button(int down){if(down)lock_down_count++;else lock_up_count++;return 1;}
static int smc_drum_mix(void){return -1;}
static int smc_drum_track(int kit,int strip){(void)kit;(void)strip;return -1;}
static int smc_lock_blocks(uint8_t note){(void)note;return 0;}
static void smc_fx_release_note(uint8_t note){(void)note;fx_release_count++;}
static int smc_fx_bus_now(uint8_t note){(void)note;return -1;}
static void smc_fx_clear(void){}
static void smc_solo_publish(void){}
static void smc_button_apply(uint8_t note,int mmc){(void)note;(void)mmc;apply_count++;}

/* PRODUCTION_FUNCTIONS_BEGIN */
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t velocity)
{
    uint8_t type = st & 0xF0u;
    /* Keep this function self-contained: the isolated queue test extracts it
     * without the application's C header include block. */
    return !(note & 0x80u) && !(velocity & 0x80u) &&
           (note < 32u || (note >= 0x2Eu && note <= 0x31u) ||
            (note >= 0x5Bu && note <= 0x63u)) &&
           (type == 0x80u || (type == 0x90u && !velocity));
}

int smc_ui_active(void)
{
    if (smc_ui.on && (ui_project_busy() || ui.help || ui.overlay == O_ASK ||
        smc_ui.context != smc_ui_context() ||
        !smc_ui_edge_alive(plat_ms(), smc_ui.until,
            smc_hold_any(smc_states[smc_ui.peer].held))))
        smc_ui_cancel();
    return smc_ui.on;
}

static void smc_button(uint8_t note, int mmc)
{
    /* A second NoteOn without its NoteOff is a repeat, not another toggle.
     * Each peer retains its own physical key owner across UI navigation. */
    if (!mmc && !smc_hold_press(smc.held, note)) return;
    if (!mmc && note == 0x5Du) {
        smc_lock_button(1);
        return;                         /* no transport action card for target lock */
    }
    if (!mmc && note == 6)
        smc.home_down = 1;
    if (!mmc && note >= 24u && note < 32u) {
        uint8_t bit = (uint8_t)(1u << (note - 24u));
        if (smc.select_down & bit) return;
        smc.select_down |= bit;
    }
    if (!mmc && note == 0x5Eu) {
        if (smc.play_down) return;
        smc.play_down = 1;
    }
    char saved[sizeof ui.msg];
    uint32_t until = ui.msg_until;
    int kit = smc_drum_mix();
    int action = !smc_lock_blocks(note) && (note == 0x5Du || note == 0x5Eu || note == 0x5Fu ||
        (note < 24u && (kit >= 0 ? smc_drum_track(kit, note & 7u) >= 0 :
            (((note & 7u) < NPARTS && note != 14u) || note >= 16u))));
    memcpy(saved, ui.msg, sizeof saved);
    smc_ui_cancel();                     /* navigation owns its new screen immediately */
    smc_button_apply(note, mmc);
    if (smc_fx_bus_now(24u) < 0) smc_fx_clear(); /* queued SMC navigation also cancels holds */
    if (action) {
        memcpy(ui.msg, saved, sizeof saved);  /* Help keeps its card and the preexisting notice too. */
        ui.msg_until = until;
        smc_ui.action = note;
        smc_ui_begin(note < 32u ? note & 7u : 0, 3);
    }
}

static void smc_release(uint8_t note)
{
    int was_held = smc_hold_release(smc.held, note);
    if (was_held && smc_ui.on && smc_ui.peer == smc_slot &&
        !smc_hold_any(smc.held))
        smc_ui.until = plat_ms() + SMC_UI_MS;
    if (note == 0x5Du) smc_lock_button(0);
    if (note == 6) smc.home_down = 0;
    if (note == 0x5Eu) smc.play_down = 0;
    smc_fx_release_note(note);
    if (smc_fx_bus_now(24u) < 0) smc_fx_clear(); /* a main tap may have navigated out of FX */
    if (note >= 24u && note < 32u)
        smc.select_down &= (uint8_t)~(1u << (note - 24u));
    if (note >= 8u && note < 16u && smc.solo_target[note - 8u]) {
        smc.solo_target[note - 8u] = 0; /* release the original target, even after navigation */
        smc_solo_publish();
    }
}
/* PRODUCTION_FUNCTIONS_END */

static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void)
{
    for(unsigned note=0;note<128u;note++) {
        CHECK(smc_is_solo_release(0x80u,(uint8_t)note,0) == smc_tracked_release(0x80u,(uint8_t)note,0));
        CHECK(smc_is_solo_release(0x90u,(uint8_t)note,0) == smc_tracked_release(0x90u,(uint8_t)note,0));
        CHECK(!smc_is_solo_release(0x90u,(uint8_t)note,127));
    }
    smc_slot=0;now=100;smc_button(0,0);CHECK(apply_count==1);CHECK(smc_ui_active());
    smc_button(0,0);CHECK(apply_count==1);now=5100;CHECK(smc_ui_active());
    smc_release(0);CHECK(!smc_hold_any(smc.held));CHECK(smc_ui_active());
    now+=1199;CHECK(smc_ui_active());now++;CHECK(!smc_ui_active());
    smc_slot=0;now=100;smc_button(16,0);CHECK(smc_ui_active());
    smc_button(17,0);CHECK(smc_ui_active());now+=6000;CHECK(smc_ui_active());
    smc_release(16);CHECK(smc_ui_active());now+=1200;CHECK(smc_ui_active());
    smc_release(17);now+=1199;CHECK(smc_ui_active());now++;CHECK(!smc_ui_active());
    smc_slot=0;now=100;smc_button(16,0);CHECK(smc_hold_any(smc.held));
    smc_slot=1;smc_button(16,0);CHECK(smc_hold_any(smc.held));
    smc_slot=0;smc_release(16);CHECK(!smc_hold_any(smc.held));
    smc_slot=1;CHECK(smc_hold_any(smc.held));smc_release(16);CHECK(!smc_hold_any(smc.held));
    smc_slot=0;smc_button(0x5d,0);smc_button(0x5d,0);CHECK(lock_down_count==1);smc_release(0x5d);CHECK(lock_up_count==1);
    printf("PASS exact-source SMC input/UI smoke: %u assertions\n",checks);
    return 0;
}
