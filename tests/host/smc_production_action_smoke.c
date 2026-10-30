/* Host-only action and UI test with verbatim production function bodies.
 * This is a scoped compile test, not a full firmware or device simulation. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "app/smc_mixer_edges.h"

#define SMC_PEERS 2u
enum { O_NONE=0, O_ASK=1, NPARTS=5, SMC_UI_MS=1200 };
typedef struct { uint8_t held[16],home_down,select_down,play_down,solo_target[8]; } smc_state_t;
static smc_state_t smc_states[SMC_PEERS];
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
