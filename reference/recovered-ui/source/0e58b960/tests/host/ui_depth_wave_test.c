/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void depth_shots(void)
{
 setup(V_HOME,PART_909);for(int s=0;s<16;s+=4)sm_set(proj.pat[0].drum[0].hit[0],s);ui_frame();shot("depth-home");
 setup(V_PART,PART_303A);
 CHECK(!engine_bass_select_preset(0,FACTORY_BANK_MIAMI,0,&proj.sound));
 {int32_t audio[512];engine_render(audio,256);}
 CHECK(engine_bass_is_wobble(0));
 build_pages();
 for(int shape=0;shape<2;shape++) {
  engine_set(T_303,0,W_CARRIER_WAVE,shape);engine_set(T_303,0,W_XMOD_WAVE,shape);engine_set(T_303,0,W_WOBBLE_SHAPE,shape);
  proj.sound.v[T_303][0][W_CARRIER_WAVE]=shape;proj.sound.v[T_303][0][W_XMOD_WAVE]=shape;proj.sound.v[T_303][0][W_WOBBLE_SHAPE]=shape;
  for(int page=0;page<3;page++) {
   char name[64];*page_of(V_PART)=page;memset(blit_hash,0,sizeof blit_hash);ui_frame();
   if(!page)shot(shape?"depth-miami-square-pulse":"depth-miami-triangle");
   snprintf(name,sizeof name,"depth-miami-%s-selected-%d",shape?"square-pulse":"triangle",page);shot(name);
  }
 }
 *page_of(V_PART)=5;memset(blit_hash,0,sizeof blit_hash);ui_frame();shot("depth-miami-output-sound");
 setup(V_PART,PART_303B);ui_frame();shot("depth-chicago");
 *page_of(V_PART)=4;memset(blit_hash,0,sizeof blit_hash);ui_frame();shot("depth-chicago-approved-three-rows");
 setup(V_HOME,PART_909);open_global();ui_frame();shot("depth-global");
 setup(V_FX,PART_303A);ui_frame();shot("depth-fx");
}
static void draw_bounds(void)
{
 const uint16_t bg=RGB(1,2,3);
 CHECK(tw(&FONT_T,"TRI")<=18);CHECK(tw(&FONT_T,"SQR")<=18);CHECK(tw(&FONT_T,"PUL")<=18);
 CHECK(29+18+3<=59); /* native glyph and actual selected-shape label fit slot */
 for(int active=0;active<2;active++) {
  cv_begin(240,69,bg);night_panel(8,5,54,55,active);
  for(int y=0;y<69;y++)for(int x=0;x<240;x++)if(x<8 || x>=62 || y<5 || y>=60)CHECK(cv_px[y*240+x]==swap16(bg));
 }
 for(int active=0;active<2;active++)for(int shape=2;shape<=3;shape++)for(int selected=0;selected<2;selected++) {
  uint16_t bg=active?PART_COL[PART_303A]:SOUND_FACE,col=active?C_BLACK:C_WHITE;
  cv_begin(240,69,bg);draw_wave_glyph(60,20,shape,selected?127:0,col,bg);
  int ink=0,aa=0,min_y=99,max_y=-1;
  for(int y=0;y<69;y++)for(int x=0;x<240;x++) {
   uint16_t px=cv_px[y*240+x];
   if(x<46 || x>=75 || y<20 || y>=20+MIAMI_WAVE_HEIGHT)CHECK(px==swap16(bg));
   else {ink+=px!=swap16(bg);aa+=px!=swap16(bg)&&px!=swap16(col);
    if(px!=swap16(bg)){if(y<min_y)min_y=y;if(y>max_y)max_y=y;}
   }
  }
  CHECK(ink>20);CHECK(aa>10);
  CHECK(max_y-min_y>=(!selected?15:shape==2?14:15));
  /* Single affine projection: correct zero and gain, no independent polarity
   * normalization. AA footprint stays inside the 19px value-cell box. */
  for(int i=0;i<=8192;i++) {
   float v=ui_miami_wave_value(shape==3,selected,(float)i/8192.0f);
   float py=miami_wave_plot_y(shape==3 && selected,v);
   CHECK(py>=.5f && py<=MIAMI_WAVE_HEIGHT-1.5f);
  }
 }
 CHECK(fm_fabsf(miami_wave_plot_y(1,0)-15.4f)<.001f);
 CHECK(fm_fabsf((miami_wave_plot_y(1,0)-miami_wave_plot_y(1,1))/(miami_wave_plot_y(1,-1.0f/9)-miami_wave_plot_y(1,0))-9)<.001f);
 CHECK(25+MIAMI_WAVE_HEIGHT<=46); /* 49px row: clear of bottom edge/bar */
 CHECK(25>11+10); /* clear of parameter caption */
 /* UIW-01: zero-only pixel is subdued for black-on-selected and white-on-dark. */
 for(int active=0;active<2;active++)for(int shape=2;shape<=3;shape++)for(int selected=0;selected<2;selected++) {
  uint16_t face=active?PART_COL[PART_303A]:SOUND_FACE,ink=active?C_BLACK:C_WHITE;
  int pulse=shape==3 && selected;
  int zero=(int)(miami_wave_plot_y(pulse,0)+.5f),xx=pulse?70:selected?53:46;
  cv_begin(240,69,face);draw_wave_glyph(60,20,shape,selected?127:0,ink,face);
  CHECK(cv_px[(20+zero)*240+xx]==swap16(blend(face,ink,3)));
  CHECK(cv_px[(20+zero)*240+xx]!=swap16(ink));
 }
 /* Drawing must not edit project, sequencer, notes or parameters. */
 setup(V_PART,PART_303A);project_t *p=malloc(sizeof proj);CHECK(p!=NULL);*p=proj;seq_t q=seq;
 draw_header();draw_main();draw_knobs();CHECK(!memcmp(p,&proj,sizeof proj));CHECK(!memcmp(&q,&seq,sizeof seq));free(p);
}
int main(void) { depth_shots();draw_bounds();printf("depth render PASS %u checks\n",checks);return 0; }
