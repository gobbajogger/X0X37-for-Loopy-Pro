/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static int active_at(int x,int y){return cv_px[y*240+x]==swap16(C_HI);}
static int column_ink(int x){int n=0;for(int y=26;y<=50;y++)n+=active_at(x,y);return n;}
int main(void){
 setup(V_FX,PART_909);
 for(int n=0;n<=32;n++){
  if(!n)CHECK(glyph_identity_index(n,0)==-1);
  for(int v=0;v<n;v++){
   int idx=glyph_identity_index(n,v);CHECK(idx>=0&&idx<(n>8?8:n));if(n<=8)CHECK(idx==v);
   cv_begin(240,KNOB_H,C_BLACK);glyph_identity(n,v,12,C_HI);for(int j=0;j<(n>8?8:n);j++)CHECK(active_at(15+(j%4)*10,33+(j/4)*10)==(j==idx));
  }
 }
 cv_begin(240,KNOB_H,C_BLACK);glyph_identity(0,0,12,C_HI);for(int i=0;i<240*KNOB_H;i++)CHECK(cv_px[i]!=swap16(C_HI));
 pref_t refs[]={PR(R_ENG,T_FX,0,FX_DL_TIME),PR(R_ENG,T_FX,0,FX_RV_DECAY),PR(R_SPAT,0,0,0),PR(R_BRKSLOT,0,0,0),PR(R_SBAR,0,0,0),PR(R_TAPE,0,0,0),PR(R_ENG,T_FX,0,FX_DRIVE)};
 for(unsigned r=0;r<sizeof refs/sizeof refs[0];r++){
  const x0x_param_t*d=pref_desc(refs[r]);CHECK(d);
  for(int v=0;v<=d->max;v++){
   cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(refs[r],v,30,C_HI));
   for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(x<10||x>50||y<26||y>50)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
  }
 }
 const int gaps[]={2,3,4,4,5,5,6,6,7,8,8,9,10};
 for(int v=0;v<13;v++){cv_begin(240,KNOB_H,C_BLACK);control_glyph(refs[0],v,30,C_HI);CHECK(column_ink(12)==19);CHECK(active_at(12+gaps[v],35));CHECK(active_at(12+gaps[v],50));}
 int tail[2];for(int k=0;k<2;k++){cv_begin(240,KNOB_H,C_BLACK);control_glyph(refs[1],k?127:0,30,C_HI);int last=99;for(int j=0;j<7;j++){int h=column_ink(12+j*6);CHECK(h<=last);last=h;}tail[k]=last;}CHECK(tail[1]>tail[0]);
 uint16_t digital[240*KNOB_H];pref_set(PR(R_TAPE,0,0,0),15);CHECK(pref_get(PR(R_ENG,T_FX,0,FX_DL_TYPE))==0);cv_begin(240,KNOB_H,C_BLACK);control_glyph(PR(R_TAPE,0,0,0),15,30,C_HI);memcpy(digital,cv_px,sizeof digital);
 pref_set(PR(R_TAPE,0,0,0),16);CHECK(pref_get(PR(R_ENG,T_FX,0,FX_DL_TYPE))==1);cv_begin(240,KNOB_H,C_BLACK);control_glyph(PR(R_TAPE,0,0,0),16,30,C_HI);CHECK(memcmp(digital,cv_px,sizeof digital));
 /* BAR identity cell count changes through SONG's real LENGTH list ref even
  * when cursor, visible pattern and repeat values stay unchanged. */
 setup(V_SONG,PART_909);*page_of(V_SONG)=0;build_pages();open_list_of_pages();
 int length_row=-1;for(int i=0;i<ui.list_n;i++)if(list_rows[i].kind==R_SLEN)length_row=i;
 CHECK(length_row>=0);ui.list_sel=(uint8_t)length_row;ui_frame();
 uint16_t empty_strip[240*KNOB_H],full_strip[240*KNOB_H];int visible[4];
 for(int i=0;i<4;i++)visible[i]=pref_get(pg.r[cur_page()][i]);
 uint32_t empty_sig=knobs_sig();memcpy(empty_strip,fb+240*KNOB_Y,sizeof empty_strip);shot("song-empty");
 pref_set(list_rows[length_row],4);ui_frame();CHECK(ui.song_sel==0);CHECK(knobs_sig()!=empty_sig);
 for(int i=0;i<4;i++)CHECK(pref_get(pg.r[cur_page()][i])==visible[i]);
 memcpy(full_strip,fb+240*KNOB_Y,sizeof full_strip);CHECK(memcmp(empty_strip,full_strip,sizeof empty_strip));shot("song-four");
 uint32_t full_sig=knobs_sig();pref_set(list_rows[length_row],0);ui_frame();CHECK(knobs_sig()!=full_sig);
 CHECK(!memcmp(empty_strip,fb+240*KNOB_Y,sizeof empty_strip));shot("song-empty-again");
 printf("all glyph semantics PASS %u checks\n",checks);return 0;
}
