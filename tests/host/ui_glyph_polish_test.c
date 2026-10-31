/* SPDX-License-Identifier: GPL-3.0-only */
/* Production engine, direct native glyphs, and the real cached framebuffer path. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#include "app/original_runtime.h"
#include "chicago_fx2_compact.h"
static void drain_ms(int ms){int32_t a[512];for(int i=0;i<ms;i+=6)engine_render(a,256);audio_due_ms=now_ms;run_ms(ms);}
static uint32_t pixels(const uint16_t *px,int x,int y,int w,int h){uint32_t z=2166136261u;for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)z=(z^px[j*240+i])*16777619u;return z;}
static uint32_t polish_glyph(pref_t r,int value,int *px,int *py){
 cv_begin(240,KNOB_H,C_BLACK);orig_strip_glyph(r,value,10,10,C_HI);
 *px=*py=-1;
 for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK)){
  if(!(x>=10 && x<36 && y>=10 && y<=28))fprintf(stderr,"glyph bounds target=%d voice=%d control=%d value=%d pixel=%d,%d\n",r.a,r.b,r.c,value,x,y);CHECK(x>=10 && x<36 && y>=10 && y<=28);
  if(cv_px[y*240+x]!=swap16(C_LINE) && *py<0){*px=x;*py=y;}
 }
 return pixels(cv_px,10,10,26,19);
}
static void chicago(void){setup(V_PART,PART_303B);pref_set(PR(R_ENG,T_303,1,13),0);drain_ms(300);}
static void test_env(void){
 const int pot[]={0,32,64,96,127};const char *text[]={"-100","-50","0","+51","+100"};
 int dx[5];uint32_t shape[5];
 for(int i=0;i<5;i++){
  chicago();pref_t r=PR(R_ENG,T_303,1,4);pref_set(r,pot[i]);pref_set(PR(R_ENG,T_303,1,2),64);pref_set(PR(R_ENG,T_303,1,3),110);drain_ms(40);
  char n[16],u[8];pref_value_of(r,pot[i],n,u);CHECK(!strcmp(n,text[i]));
  int x,y,xx,yy;uint32_t idle=polish_glyph(r,pot[i],&x,&y);uint16_t idle_ink=cv_px[y*240+x];
  engine_bass_on_velocity(1,48,0,0,120);drain_ms(12);shape[i]=polish_glyph(r,pot[i],&xx,&yy);dx[i]=xx-x;CHECK(yy==y);if(i!=2)CHECK(cv_px[yy*240+xx]!=idle_ink);
  for(int f=0;f<10;f++){drain_ms(12);polish_glyph(r,pot[i],&xx,&yy);CHECK(yy==y);if(i==2)CHECK(xx==x);}
  if(i==2)CHECK(shape[i]==idle);
  engine_bass_off(1);drain_ms(6000);CHECK(polish_glyph(r,pot[i],&xx,&yy)==idle);
 }
 CHECK(dx[0]<dx[1] && dx[1]<0 && dx[2]==0 && dx[3]>0 && dx[4]>dx[3]);
 for(int i=0;i<5;i++)for(int j=i+1;j<5;j++)CHECK(shape[i]!=shape[j]);
 /* Reserved headroom preserves signed motion even at cutoff extremes. */
 for(int cutoff=0;cutoff<=127;cutoff+=127)for(int sign=0;sign<2;sign++){
  chicago();pref_t r=PR(R_ENG,T_303,1,4);pref_set(r,sign?127:0);pref_set(PR(R_ENG,T_303,1,2),cutoff);pref_set(PR(R_ENG,T_303,1,3),110);drain_ms(30);
  int x,y,xx,yy;polish_glyph(r,sign?127:0,&x,&y);engine_bass_on_velocity(1,48,0,0,120);drain_ms(12);polish_glyph(r,sign?127:0,&xx,&yy);CHECK(sign?xx>x:xx<x);CHECK(y==yy);
 }
 puts("PASS ENV signed magnitude, exact centre, constant peak, extremes, decay return, bounds");
}
static void test_read_only(void){
 static original_runtime_part live,before;static original_runtime_workspace workspace;
 static original_chicago_fx2_compact fx,fx_before; original_chicago_tuning tuning;
 CHECK(original_chicago_fx2_compact_init(&fx));CHECK(original_chicago_tuning_bind(&tuning,0,0,0,0x6d70));
 original_chicago_fx2_cold_owner owner=original_chicago_fx2_compact_owner(&fx);
 for(int kind=0;kind<2;kind++){
  uint8_t cell[32];float left[256],right[256];CHECK(original_sound_init(cell,kind,0));
  CHECK(original_runtime_init(&live,cell,125,&tuning,&owner)==1);CHECK(original_runtime_key(&live,ORIGINAL_INPUT_KEYS,48,120)==1);
  CHECK(original_runtime_render(&live,&workspace,left,right,256)==1);before=live;fx_before=fx;original_chicago_tuning tb=tuning;
  for(int i=0;i<100;i++){float a,m,e,d;unsigned state,key;CHECK(original_runtime_voice_view(&live,&a,&m,&state,&key));if(!kind){float env[4];unsigned gate;CHECK(original_runtime_filter_view(&live,&e,&d));CHECK(original_runtime_adsr_view(&live,env,&gate));}}
  CHECK(!memcmp(&before,&live,sizeof before));CHECK(!memcmp(&fx_before,&fx,sizeof fx));CHECK(!memcmp(&tb,&tuning,sizeof tb));
  CHECK(original_runtime_retire(&live)==1);
 }
 CHECK(!engine_bass_voice_view(-1,0,0,0,0));
 puts("PASS read-only telemetry leaves complete native DSP, program, owners, queues unchanged");
}
static void test_synth_motion(int part){
 setup(V_PART,part);int voice=part-PART_303A,control=part==PART_303A?1:5;
 if(part==PART_303B)pref_set(PR(R_ENG,T_303,voice,13),0);
 pref_t r=PR(R_ENG,T_303,voice,control);int page=-1,slot=-1;
 for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_ENG && pg.r[p][k].c==control){page=p;slot=k;}
 CHECK(page>=0);*page_of(V_PART)=page;build_pages();drain_ms(300);ui_frame();
 int x,y;uint32_t idle=polish_glyph(r,matrix_value(r),&x,&y),sig=knobs_sig();
 int row=page%4,gy=MAIN_Y+4+row*50+22,gx=4+slot*59+3;
 uint32_t cached=pixels(fb,gx,gy,26,19);
 engine_bass_on_velocity(voice,48,0,0,120);drain_ms(36);ui_frame();
 CHECK(polish_glyph(r,matrix_value(r),&x,&y)!=idle);CHECK(knobs_sig()!=sig);CHECK(pixels(fb,gx,gy,26,19)!=cached);
 uint32_t live=polish_glyph(r,matrix_value(r),&x,&y);ui_frame();CHECK(polish_glyph(r,matrix_value(r),&x,&y)==live);
 engine_bass_off(voice);drain_ms(6000);ui_frame();CHECK(polish_glyph(r,matrix_value(r),&x,&y)==idle);
 printf("PASS synth %d direct glyph and framebuffer cache updates; idle stable\n",part);
}
static void test_synth_level_meter(void){
 for(int part=PART_303A;part<=PART_303B;part++){
  setup(V_PART,part);int voice=part-PART_303A;pref_t r=PR(R_ENG,T_303,voice,part==PART_303A?14:12);pref_set(r,127);drain_ms(30);
  engine_bass_on_velocity(voice,48,0,0,120);drain_ms(36);int x,y;polish_glyph(r,127,&x,&y);
  unsigned lit=0;for(int yy=10;yy<=28;yy++)for(int xx=10;xx<36;xx++)lit+=cv_px[yy*240+xx]==swap16(C_WHITE);
  CHECK(lit>6); /* Actual meter segments pulse, beyond the generic activity pips. */
 }
 puts("PASS original LEVEL/OUTPUT is a live segmented amplitude meter");
}
static uint32_t adsr_frame(void){cv_begin(240,KNOB_H,C_BLACK);draw_adsr_inline(0,49,0);return pixels(cv_px,10,14,220,34);}
static void test_adsr_motion(void){
 chicago();pref_set(PR(R_ENG,T_303,1,11),0);for(int c=7;c<=10;c++)pref_set(PR(R_ENG,T_303,1,c),c==7?80:c==8?40:c==9?80:40);drain_ms(30);
 int p=0;for(;p<pg.n;p++)if(!strcmp(pg.title[p],"ENVELOPE"))break;CHECK(p<pg.n);*page_of(V_PART)=p;build_pages();ui_frame();
 ui_adsr_t env;adsr_values(&env);CHECK(env.attack>.1f && env.attack<.25f);
 float a,m;unsigned state,key;CHECK(engine_bass_voice_view(1,&a,&m,&state,&key));CHECK(state==6);uint32_t idle=adsr_frame();
 for(int y=8;y<22;y++)for(int x=10;x<230;x++)CHECK(cv_px[y*240+x]!=swap16(matrix_col())); /* Plot clears knob labels. */
 engine_bass_on_velocity(1,48,0,0,120);drain_ms(12);CHECK(engine_bass_voice_view(1,&a,&m,&state,&key));CHECK(state==0 && key);uint32_t attack=adsr_frame();CHECK(attack!=idle);test_read_only();
 drain_ms(1000);CHECK(engine_bass_voice_view(1,&a,&m,&state,&key));CHECK(state==1 && key && a>.6f && a<.65f);uint32_t sustain=adsr_frame();CHECK(sustain!=attack);
 engine_bass_off(1);drain_ms(12);CHECK(engine_bass_voice_view(1,&a,&m,&state,&key));CHECK(state==3 && !key);CHECK(adsr_frame()!=sustain);
 drain_ms(6000);CHECK(engine_bass_voice_view(1,&a,&m,&state,&key));CHECK(state==6);CHECK(adsr_frame()==idle);
 puts("PASS inline ADSR actual attack, held sustain, release, silent cursor");
}
/* Actual RGB565 pixels from draw_adsr_inline, compared against four
 * independently specified labels using the same production glyph rasteriser.
 * Header pixels are included so the touch readout cannot hide a stage value. */
static void adsr_expected_header_and_labels(int highlighted)
{
 const char *const labels[4]={"K5 A 63%","K6 D 31%","K7 S 63%","K8 R 31%"};
 const char *const touched[4]={"ATTACK 63%","DECAY 31%","SUSTAIN 63%","RELEASE 31%"};
 uint16_t actual[240*22];
 memcpy(actual,cv_px,sizeof actual);
 cv_begin(240,KNOB_H,C_BLACK);
 if(highlighted>=0)text_r(232,0,&FONT_T,touched[highlighted],SOUND_INK);
 for(int k=0;k<4;k++)text_c(31+k*59,8,&FONT_T,labels[k],
                               k==highlighted?SOUND_INK:SOUND_GRAY);
 CHECK(!memcmp(actual,cv_px,sizeof actual));
}
static void test_adsr_idle_values_and_touch(void)
{
 chicago();pref_set(PR(R_ENG,T_303,1,11),0);
 const int values[4]={80,40,80,40};
 for(int k=0;k<4;k++)pref_set(PR(R_ENG,T_303,1,7+k),values[k]);
 drain_ms(30);
 int p=0;
 for(;p<pg.n;p++)if(!strcmp(pg.title[p],"ENVELOPE"))break;
 CHECK(p<pg.n);
 *page_of(V_PART)=(uint8_t)p;build_pages();
 for(int k=0;k<4;k++) {
  CHECK(pg.r[p][k].kind==R_ENG && pg.r[p][k].c==7+k);
 }
 ui.touched=-1;ui.touch_until=0;
 cv_begin(240,KNOB_H,C_BLACK);draw_adsr_inline(0,49,0);
 adsr_expected_header_and_labels(-1); /* All four native percentages at idle. */
 for(int k=0;k<4;k++) {
  ui.touched=(int8_t)k;ui.touch_until=plat_ms()+900u;
  cv_begin(240,KNOB_H,C_BLACK);draw_adsr_inline(0,49,0);
  adsr_expected_header_and_labels(k); /* K5, K6, K7, K8 highlighted. */
  now_ms+=901;
  cv_begin(240,KNOB_H,C_BLACK);draw_adsr_inline(0,49,0);
  adsr_expected_header_and_labels(-1); /* All four return after timeout. */
 }
 ui.touched=-1;ui.touch_until=0;
 puts("PASS ADSR four idle percentages, K5-K8 touch, every timeout: real RGB565 pixels");
}
static void test_values_and_bounds(void){
 for(int part=PART_303A;part<=PART_303B;part++){
  setup(V_PART,part);for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++){
   pref_t r=pg.r[p][k];if(!orig_strip_slot(r))continue;const x0x_param_t *d=pref_desc(r);
   const int pots[5]={0,32,64,96,127};uint32_t previous=0;
   int kind=engine_bass_sound_bank(&proj.sound,r.b);
   for(int v=0;v<5;v++){
    int value=pots[v]*d->max/127;pref_set(r,value);drain_ms(15);
    int x,y;uint32_t h=polish_glyph(r,value,&x,&y);
    if(v && !ui_original_states(kind,r.c) && d->max==127){
     if(h==previous)fprintf(stderr,"original value collision kind=%d ctrl=%u value=%d\n",kind,r.c,pots[v]);
     CHECK(h!=previous);
    }
    previous=h;
   }
  }
 }
 puts("PASS all original-control glyph native bounds at five representative values");
}
static void test_readout_bounds(void){
 char number[16]="253",unit[8]="ms";
 cv_begin(240,KNOB_H,C_BLACK);strip_value_fit(4,28,number,unit,C_HI);
 for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK))CHECK(x>=34 && x<=61);
 puts("PASS wide drum number/unit stays clear of glyph and divider");
}
int main(void){test_readout_bounds();test_env();test_synth_motion(PART_303A);test_synth_motion(PART_303B);test_adsr_motion();test_adsr_idle_values_and_touch();test_synth_level_meter();test_values_and_bounds();printf("GLYPH POLISH PASS %u assertions\n",checks);return 0;}
