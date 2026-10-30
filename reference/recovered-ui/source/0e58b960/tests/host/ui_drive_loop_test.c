/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
int main(void){
 setup(V_MIX,PART_909);int prior=-1;
 for(int val=0;val<=127;val++){
  cv_begin(240,KNOB_H,C_BLACK);drive_amount_glyph(val,30,C_HI);int flat=0;
  for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK)){CHECK(x>=12&&x<=48&&y>=30&&y<=46);CHECK(cv_px[y*240+x]==swap16(C_HI));}
  for(int x=12;x<=48;x++){int ink=0;for(int y=30;y<=46;y++)ink+=cv_px[y*240+x]==swap16(C_HI);CHECK(ink>0);flat+=cv_px[30*240+x]==swap16(C_HI);flat+=cv_px[46*240+x]==swap16(C_HI);}
  CHECK(cv_px[38*240+12]==swap16(C_HI));CHECK(cv_px[38*240+30]==swap16(C_HI));CHECK(cv_px[38*240+48]==swap16(C_HI));CHECK(flat>=prior);prior=flat;
 }
 for(int count=0;count<=40;count++)for(int val=0;val<(count?count:1);val++){
  cv_begin(240,KNOB_H,C_BLACK);loop_identity_glyph(count,val,30,C_HI);
  for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK))CHECK(x>=12&&x<=48&&y>=30&&y<=46);
  if(!count){CHECK(glyph_identity_index(count,val)==-1);for(int i=0;i<240*KNOB_H;i++)CHECK(cv_px[i]!=swap16(C_HI));continue;}
  int n=count>8?8:count,rows=(n+3)/4,active=0;
  for(int row=0;row<rows;row++){
   int nr=n-row*4;if(nr>4)nr=4;int left=30-(nr*10-3)/2,y=rows==1?35:30+10*row;
   CHECK(left+left+(nr-1)*10+6==60);
   for(int k=0;k<nr;k++){int selected=(count<=8?val:val*7/(count-1))==row*4+k;uint16_t pixel=cv_px[(y+3)*240+left+k*10+3];CHECK(pixel==swap16(selected?C_HI:C_LINE));active+=pixel==swap16(C_HI);}
  }
  CHECK(active==1);
 }
 printf("DRIVE/LOOP proposal bounds, plateau progression, exact selection and row centering PASS %u assertions\n",checks);return 0;
}
