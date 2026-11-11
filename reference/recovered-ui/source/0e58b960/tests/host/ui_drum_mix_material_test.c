/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
/* Synthetic telemetry, actual production renderer. No audio or device claims. */
int main(void)
{
 for(int kit=0;kit<2;kit++)for(int state=0;state<3;state++) {
  setup(V_MIX,kit);*page_of(V_MIX)=kit+1;ui.sel[kit]=state==2?10:state==1?6:0;build_pages();
  int t=kit?T_808:T_909;
  for(int v=0;v<NDRUM;v++) {
   pref_t lv=find_ref(t,v,"Level"),pn=find_ref(t,v,"Pan");
   CHECK(lv.kind && pn.kind);CHECK(tw(&FONT_XS,engine_voice_name(t,v))<=17);
   proj.sound.v[t][v][lv.c]=state==0?127:state==1?0:12*v;
   proj.sound.v[t][v][pn.c]=state==0?0:state==1?64:127;
   eng_hit[kit][v]=state==0?255:state==1?0:23*v;
  }
  if(state==2)seq.vmute=1u<<(kit*NDRUM+6);
  project_t *copy=malloc(sizeof proj);CHECK(copy);memcpy(copy,&proj,sizeof proj);
  ui_frame();CHECK(!memcmp(copy,&proj,sizeof proj));free(copy);
  CHECK(NDRUM==11);if(kit)CHECK(!strcmp(engine_voice_name(t,6),"CP"));
  char name[80];snprintf(name,sizeof name,"mix%d-%s",kit?808:909,state==0?"full-left-hit":state==1?"zero-center":"levels-right-muted");shot(name);
 }
 /* Direct production-band assertions: top captions, 96px rail, mute/hit
  * separation, clipping, and no drawing into the spaces between strips. */
 for(int kit=0;kit<2;kit++)for(int muted=0;muted<2;muted++)for(int hit=0;hit<2;hit++) {
  setup(V_MIX,kit);*page_of(V_MIX)=kit+1;ui.sel[kit]=0;build_pages();
  int t=kit?T_808:T_909;pref_t lv=find_ref(t,0,"Level");
  proj.sound.v[t][0][lv.c]=127;eng_hit[kit][0]=hit?255:0;
  seq.vmute=muted?(1u<<(kit*NDRUM)):0;
  cv_begin(240,BAND_H,C_BLACK);draw_drum_mix(0,kit);
  uint16_t ink=muted?C_DIM:hit?C_WHITE:dim(PART_COL[kit],6);
  CHECK(cv_px[20*240+13]==swap16(ink));
  CHECK(cv_px[60*240+13]==swap16(ink));
  CHECK(cv_px[2*240+7]==swap16(PART_COL[kit])); /* selected caption face */
  for(int y=0;y<BAND_H;y++)for(int v=0;v<NDRUM-1;v++)
   CHECK(cv_px[y*240+4+v*21+19]==swap16(C_BLACK));
  cv_begin(240,BAND_H,C_BLACK);draw_drum_mix(1,kit);
  CHECK(cv_px[(111-BAND_H)*240+13]==swap16(ink));
  CHECK(cv_px[(113-BAND_H)*240+13]!=swap16(ink));
 }
 test_distinct_maps(); /* Both kits, both banks: level and pan mapping unchanged. */
 printf("drum_mix_material PASS (%u checks)\n",checks);return 0;
}
