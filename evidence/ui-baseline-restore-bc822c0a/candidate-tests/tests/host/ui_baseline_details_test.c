/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#ifndef UI_RESTORE_CAPTURE_ONLY
static uint16_t donor_pixels[240 * BAND_H];
static void donor_header_commit(int region,uint32_t x,uint32_t y)
{ (void)region;(void)x;(void)y;memcpy(donor_pixels,cv_px,240*HDR_H*2); }
#include "reference/ui_d808_restore.inc"
static void compare_header_region(int full)
{
 ui_t saved=ui;donor_draw_header();ui=saved;draw_header();
 for(int y=0;y<HDR_H;y++)for(int x=0;x<240;x++)
  if(full || x<130 || (x>=217 && x<228))CHECK(donor_pixels[y*240+x]==fb[y*240+x]);
}
static void compare_list(void)
{
 for(int band=0;band<2;band++) {
  cv_begin(240,BAND_H,C_BLACK);donor_draw_list(band);memcpy(donor_pixels,cv_px,240*BAND_H*2);
  cv_begin(240,BAND_H,C_BLACK);draw_list(band);CHECK(!memcmp(donor_pixels,cv_px,240*BAND_H*2));
 }
}
#else
static void compare_header_region(int full){(void)full;draw_header();}
static void compare_list(void){}
#endif
static void header_shot(const char *name,int full)
{compare_header_region(full);shot(name);}
int main(void)
{
 setup(V_HOME,PART_909);button_tap(B_GLO);
 for(int g=0;g<6;g++) {
  global_index();ui.list_sel=g;list_enter();ui.msg_until=0;ui_frame();compare_list();
  char name[32];snprintf(name,sizeof name,"section-%d",g);header_shot(name,0);
 }
 /* Physical action cue is unavailable in clip mode; About fitting follows it. */
 clip_grid_mode=1;global_index();ui.list_sel=5;list_enter();ui_frame();compare_list();shot("actions-clip");clip_grid_mode=0;
#ifndef UI_RESTORE_CAPTURE_ONLY
 for(int clip=0;clip<2;clip++)for(int sel=0;sel<2;sel++) {
  clip_grid_mode=clip;int32_t x1=0,w1=0,x2=0,w2=0;
  CHECK(!strcmp(action_key_cue(),donor_action_key_cue()));
  CHECK(release_label_box("0.0.25",sel,&x1,&w1)==donor_release_label_box("0.0.25",sel,&x2,&w2));
  CHECK(x1==x2 && w1==w2);
 }clip_grid_mode=0;
#endif
 setup(V_SEQ,PART_303B);ui.freeplay[bidx()]=1;ui_frame();header_shot("long-part-freeplay",0);
 setup(V_HOME,PART_909);
 CHECK(tw(&FONT_B,"MMMMMMMMMMMMMMMMMMMMMMMMMMM")>232);
 say("MMMMMMMMMMMMMMMMMMMMMMMMMMM",0);ui_frame();header_shot("long-toast",1);
 for(int p=1;p<=NPAT;p+=NPAT-1) {
  char msg[28],name[32];snprintf(msg,sizeof msg,"NEXT: PAT %d",p);say(msg,0);ui_frame();
  snprintf(name,sizeof name,"next-pattern-%d",p);header_shot(name,1);
 }
 say("NEXT: PAT 999",0);ui_frame();header_shot("next-pattern-invalid",1);
 for(int part=PART_909;part<=PART_303B;part+=PART_303B-PART_909) {
  setup(V_PART,part);seq.pcue[part]=0xff;ui.msg_until=0;
  for(int phase=0;phase<9;phase++) {
   seq.metro_view=phase;seq.playing=phase!=0;char name[40];
   snprintf(name,sizeof name,"part-%d-phase-%d",part,phase);header_shot(name,1);
  }
  seq.pcue[part]=(seq.ppat[part]+1)%NPAT;header_shot(part==PART_909?"drum-pending-cue":"synth-pending-cue",1);
 }
 setup(V_HOME,PART_909);ui.msg_until=0;
 for(int phase=0;phase<9;phase++){seq.metro_view=phase;compare_header_region(0);}
 printf("PASS baseline UI details %u checks\n",checks);return 0;
}
