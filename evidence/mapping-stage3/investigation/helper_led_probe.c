
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
enum { O_NONE, O_LIST, O_ASK, V_HOME, V_MIX, V_GLO, V_FX, NPARTS=5, NDRUM=11, R_ENG, T_FX, FX_VOLUME };
typedef struct { int a,b,c,d; } pref_t;
#define PR(a,b,c,d) ((pref_t){a,b,c,d})
static struct { int overlay,help,view,sel[2],rec,part; } ui;
static struct { int rec_part,solo; } smc;
static struct { int playing; } seq;
static int kit,clip,page;
static uint32_t vmute;
static int drum_mix_page(void) { return kit; }
static int smk_clip_mode_on(void) { return clip; }
static int track_muted(int k,int t) { return (vmute >> (k*11+t))&1; }
static int part_muted(int p) { return p==2; }
static int pref_get(pref_t r) { (void)r; return 10; }
static pref_t smc_home_target(int s) { return PR(s,0,0,0); }
static int cur_page(void) { return page; }
static int smc_drum_mix(void)
{
    return ui.overlay == O_NONE && !smk_clip_mode_on() ? drum_mix_page() : -1;
}
static int smc_drum_track(int kit, int strip)
{
    int track = (ui.sel[kit] / 8) * 8 + strip;
    return track < NDRUM ? track : -1;
}
static int smc_led(int note)
{
    int strip = note & 7, row = note >> 3;
    if (note == 5) return ui.overlay != O_NONE;
    if (note == 6) return 0;
    if (note == 14) return ui.view == V_GLO;
    if (note >= 16 && note < 32) {
        int kit = smc_drum_mix();
        if (kit >= 0) {
            if (note >= 24) return strip == ui.sel[kit] % 8;
            if (!ui.help) {
                int track = smc_drum_track(kit, strip);
                return track >= 0 && track_muted(kit, track);
            }
        }
    }
    if (note < 32) {
        if (strip < NPARTS) {
            if (row == 0) return ui.rec && smc.rec_part == strip;
            if (row == 1) return !!(smc.solo & (1u << strip));
            if (row == 2) return part_muted(strip);
            return ui.part == strip;
        }
        if (row == 2) return pref_get(strip == 7 ? PR(R_ENG, T_FX, 0, FX_VOLUME) :
                                     smc_home_target(strip)) == 0;
        return (strip == 5 && ui.view == V_FX) || (strip == 6 && ui.view == V_FX && cur_page() == 1) ||
               (strip == 7 && (ui.view == V_MIX || ui.view == V_HOME));
    }
    if (note == 0x5D) return !seq.playing;
    if (note == 0x5E) return seq.playing;
    if (note == 0x5F) return ui.rec;
    return 0;
}
int main(void) {
 unsigned cases=0;
 for (kit=0;kit<2;kit++) for(int selected=0;selected<11;selected++) {
  ui.sel[kit]=selected;
  ui.view=V_MIX;
  ui.overlay=O_NONE;
  ui.help=clip=0;
  assert(smc_drum_mix()==kit);
  for(int bit=0;bit<22;bit++) {
   vmute=1u<<bit;
   for(int s=0;s<8;s++) {
    int expected=(selected<8?0:8)+s;
    assert(smc_drum_track(kit,s)==(expected<11?expected:-1));
    assert(smc_led(16+s)==(expected<11 && bit==kit*11+expected));
    assert(smc_led(24+s)==(s==selected%8));
    cases++;
   }
  }
  for(int owner=1;owner<=3;owner++) {
   ui.overlay=owner==3?O_NONE:owner;
   clip=owner==3;
   assert(smc_drum_mix()==-1);
   for(int s=0;s<8;s++) {
    assert(smc_led(16+s)==(s==2));
    cases++;
   }
  }
 }
 puts("PASS: all 11 selections, both kits, 22 mute bits, eight strips, and menu/confirmation/clip exclusions.");
 printf("PASS: %u independent helper/LED cases.\n",cases);
 return 0;
}
