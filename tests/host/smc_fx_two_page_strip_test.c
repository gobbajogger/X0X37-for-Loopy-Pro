#include <stdint.h>
#include <stdio.h>
#include "app/smc_fx_strip.h"
static unsigned checks;
#define C(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while(0)
static const uint8_t expected[8][2][4] = {
 {{SMCFX_REV,SMCFX_DLY,SMCFX_STUT,SMCFX_SHUF}, {255,255,255,255}},
 {{SMCFX_DLY,SMCFX_REV,SMCFX_STUT,SMCFX_SHUF}, {255,255,255,255}},
 {{SMCFX_HP,SMCFX_DLY,SMCFX_REV,SMCFX_LP}, {SMCFX_GATE8,SMCFX_GATE16,SMCFX_TRANCE,255}},
 {{SMCFX_HP,SMCFX_LP,SMCFX_DLY,SMCFX_REV}, {SMCFX_GATE16,SMCFX_GATE8,255,255}},
 {{SMCFX_GATE,SMCFX_REV,SMCFX_DLY,SMCFX_HP}, {255,255,255,255}},
 {{SMCFX_REV,SMCFX_GATE8,SMCFX_GATE16,SMCFX_TRANCE}, {SMCFX_GATE,SMCFX_STUT,SMCFX_SHUF,255}},
 {{SMCFX_DLY,SMCFX_STUT,SMCFX_SHUF,SMCFX_GATE16}, {SMCFX_GATE8,255,255,255}},
 {{SMCFX_GATE16,SMCFX_GATE8,SMCFX_TRANCE,SMCFX_STUT}, {SMCFX_SHUF,SMCFX_HP,SMCFX_LP,255}}
};
int main(void) {
 smcfx_strip_t s={0};
 smcfx_strip_init(&s);
 for(int bus=0;bus<8;bus++)for(int pg=0;pg<2;pg++)for(int b=0;b<4;b++) {
   C(smcfx_strip_get(&s,pg,bus,b)==expected[bus][pg][b]);
   if(expected[bus][pg][b]!=255)C(smcfx_suit_ok(bus,expected[bus][pg][b]));
 }
 for(int note=0;note<32;note++)C(smcfx_button(note)==note/8);
 C(smcfx_button(32)==-1);C(smcfx_button(0x60)==-1);
 C(!smcfx_strip_press(&s,8,0,1));C(smcfx_strip_live_fx(&s,0)==255);
 C(smcfx_strip_press(&s,24+2,SMCFX_303A,0));
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==SMCFX_LP);
 C(!smcfx_strip_press(&s,24+2,SMCFX_303A,0));
 C(smcfx_strip_press(&s,2,SMCFX_303A,0));
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==SMCFX_HP);
 C(smcfx_strip_release(&s,2)==SMCFX_303A);
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==SMCFX_LP);
 C(smcfx_strip_release(&s,24+2)==SMCFX_303A);
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==255);
 C(smcfx_strip_release(&s,24+2)==-1);
 uint8_t old=smcfx_strip_get(&s,0,SMCFX_303A,SMCFX_SOLO);
 uint8_t pg2=smcfx_strip_get(&s,1,SMCFX_303A,SMCFX_SOLO);
 C(smcfx_strip_cycle(&s,0,SMCFX_303A,SMCFX_SOLO)==smcfx_cycle(SMCFX_303A,old));
 C(smcfx_strip_get(&s,1,SMCFX_303A,SMCFX_SOLO)==pg2);
 C(smcfx_strip_get(&s,0,SMCFX_303A,SMCFX_REC)==SMCFX_HP);
 C(smcfx_strip_cycle(&s,1,SMCFX_303A,SMCFX_SELECT)==SMCFX_SUIT[SMCFX_303A][0]);
 C(smcfx_strip_press(&s,26,SMCFX_303A,1));
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==SMCFX_HP);
 smcfx_strip_clear_holds(&s);
 C(smcfx_strip_live_fx(&s,SMCFX_303A)==255);
 C(smcfx_strip_get(&s,1,SMCFX_303A,SMCFX_SELECT)==SMCFX_HP);
 C(!smcfx_strip_press(&s,32,0,0));
 C(smcfx_strip_get(&s,2,0,0)==255);
 printf("PASS SMC FX 2-page strip: %u assertions\n",checks);
 return 0;
}
