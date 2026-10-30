/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
/* Actual engine telemetry and production renderer. No device claims. */
static void mix_render_ms(int ms)
{
 int32_t audio[512];
 for(int elapsed=0;elapsed<ms;elapsed+=6) {
  now_ms+=6;engine_render(audio,256);
 }
}
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
  }
  engine_apply_sound(&proj.sound);mix_render_ms(300);
  if(state!=1) {
   for(int v=0;v<NDRUM;v++)if(state==0 || v)
    engine_drum(kit,v,state==0?1.0f:23.0f*v/255.0f);
   mix_render_ms(6);
  }
  if(state==2)seq.vmute=1u<<(kit*NDRUM+6);
  project_t *copy=malloc(sizeof proj);CHECK(copy);memcpy(copy,&proj,sizeof proj);
  ui_frame();CHECK(!memcmp(copy,&proj,sizeof proj));free(copy);
  CHECK(NDRUM==11);if(kit)CHECK(!strcmp(engine_voice_name(t,6),"CP"));
  char name[80];snprintf(name,sizeof name,"mix%d-%s",kit?808:909,state==0?"full-left-live-hit":state==1?"zero-center-idle":"levels-right-live-muted");shot(name);
 }
 /* Direct production-band assertions: top captions, 96px rail, mute/hit
  * separation, clipping, and no drawing into the spaces between strips. */
 for(int kit=0;kit<2;kit++)for(int muted=0;muted<2;muted++)for(int hit=0;hit<2;hit++) {
  setup(V_MIX,kit);*page_of(V_MIX)=kit+1;ui.sel[kit]=0;build_pages();
  int t=kit?T_808:T_909;pref_t lv=find_ref(t,0,"Level");
  proj.sound.v[t][0][lv.c]=127;
  mix_render_ms(300);
  uint32_t age=0;float envelope=0;
  CHECK(!engine_drum_view(kit,0,&age,&envelope));
  if(hit) {
   engine_drum(kit,0,1.0f);mix_render_ms(6);
   CHECK(engine_drum_view(kit,0,&age,&envelope));
   CHECK(envelope>.99f);
  }
  seq.vmute=muted?(1u<<(kit*NDRUM)):0;
  cv_begin(240,BAND_H,C_BLACK);draw_drum_mix(0,kit);
  uint16_t ink=muted?C_DIM:hit?C_WHITE:dim(PART_COL[kit],6);
  CHECK(cv_px[20*240+13]==swap16(ink));
  CHECK(cv_px[60*240+13]==swap16(ink));
  /* Twelve 6px segments retain the two-pixel dim level gaps. A solid
   * hit-filled rail or a missing live overlay must fail this fixture. */
  uint16_t level_ink=muted?C_DIM:dim(PART_COL[kit],6);
  for(int y=16;y<BAND_H;y++)
   CHECK(cv_px[y*240+13]==swap16((111-y)%8<6?ink:level_ink));
  CHECK(cv_px[2*240+7]==swap16(PART_COL[kit])); /* selected caption face */
  for(int y=0;y<BAND_H;y++)for(int v=0;v<NDRUM-1;v++)
   CHECK(cv_px[y*240+4+v*21+19]==swap16(C_BLACK));
  cv_begin(240,BAND_H,C_BLACK);draw_drum_mix(1,kit);
  for(int y=BAND_H;y<112;y++)
   CHECK(cv_px[(y-BAND_H)*240+13]==swap16((111-y)%8<6?ink:level_ink));
  CHECK(cv_px[(111-BAND_H)*240+13]==swap16(ink));
  CHECK(cv_px[(113-BAND_H)*240+13]!=swap16(ink));
 }
 test_distinct_maps(); /* Both kits, both banks: level and pan mapping unchanged. */
 printf("drum_mix_material PASS (%u checks)\n",checks);return 0;
}
