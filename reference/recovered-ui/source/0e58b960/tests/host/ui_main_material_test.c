/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void snap(const char *name){project_t *b=malloc(sizeof proj);CHECK(b);memcpy(b,&proj,sizeof proj);ui_frame();CHECK(!memcmp(b,&proj,sizeof proj));free(b);shot(name);}
int main(void){
 const uint16_t bg=RGB(3,7,11);
 for(int w=1;w<=14;w++)for(int h=4;h<=10;h++)for(int raised=0;raised<2;raised++) {
  cv_begin(240,BAND_H,bg);night_mark(12,10,w,h,C_HI,raised);
  for(int y=0;y<BAND_H;y++)for(int x=0;x<240;x++)
   if(x<12||x>=12+w||y<10||y>=10+h)CHECK(cv_px[y*240+x]==swap16(bg));
  if(w>=4 && h>=5)CHECK(cv_px[(10+h/2)*240+12+w/2]==swap16(C_HI));
 }
 setup(V_HOME,PART_909);open_global();snap("main-global-selected");
 ui.list_sel=ui.list_n-1;ui.list_top=ui.list_sel>5?ui.list_sel-5:0;snap("main-global-last-row");
 ask(ACT_RESET,0,"RESET PROJECT?","SAMPLES ARE KEPT");snap("main-reset-confirmation-no-action");
 setup(V_MIX,PART_909);*page_of(V_MIX)=3;build_pages();snap("main-master-wells");
 setup(V_SEQ,PART_303B);bpart_t *edge=&proj.pat[0].bass[1];edge->len=32;
 edge->step[15].note=60;edge->step[15].flags=G_NOTE;edge->step[16].flags=G_TIE;
 ui.spage=1;CHECK(note_start_of(edge,16)==15);snap("main-synth-page-tie");

 setup(V_SEQ,PART_909);for(int v=0;v<NDRUM;v++)for(int s=v%4;s<16;s+=4)sm_set(proj.pat[0].drum[0].hit[v],s);sm_set(proj.pat[0].drum[0].accent,0);sm_set(proj.pat[0].drum[0].accent,8);seq.playing=1;eng_step[TRK_DRUM]=5;snap("populated-drum-playing");
 setup(V_SEQ,PART_303B);bpart_t *b=&proj.pat[0].bass[1];b->len=32;for(int i=0;i<16;i++){b->step[i].note=48+(i%7);b->step[i].flags=i%4==3?G_TIE:G_NOTE;b->step[i].velocity=60+i*4;if(i%4==0)b->step[i].flags|=BS_ACCENT;if(i%5==0)b->step[i].flags|=BS_SLIDE;}seq.playing=1;eng_step[TRK_BASS0+1]=6;snap("populated-synth-playing");
 setup(V_SONG,PART_909);song_fill(15);for(int i=0;i<16;i++){for(int p=0;p<NPARTS;p++)song()->bar[i].pat[p]=(i+p)%16;song()->bar[i].mute=(i%3==0?(1u<<PART_BRK):0)|((i%4)<<5);}ui.song_sel=10;seq.song_on=seq.playing=1;seq.song_pos=8;snap("populated-song-scroll");
 setup(V_MIX,PART_909);for(int p=0;p<NPARTS;p++)eng_peak[p]=5000+p*5000;seq.mute=1u<<PART_808;snap("active-mix-muted");
 setup(V_MIX,PART_909);*page_of(V_MIX)=1;build_pages();for(int v=0;v<NDRUM;v++)eng_hit[0][v]=20+20*v;snap("active-mix909");
 setup(V_MIX,PART_303B);*page_of(V_MIX)=4;build_pages();now_ms=5000;for(int i=0;i<SCOPE_N;i++)eng_scope[i]=(int16_t)(20000*sinf(i*.12f));eng_scope_w+=SCOPE_N;snap("active-wave-fixture");
 printf("main_material PASS %u checks; synthetic engine telemetry, no hardware\n",checks);return 0;
}
