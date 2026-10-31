/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static int ink(void){int n=0;for(int y=26;y<=50;y++)for(int x=10;x<=50;x++)n+=cv_px[y*240+x]==swap16(C_HI);return n;}
static int bar(int x){int n=0;for(int y=26;y<=50;y++)n+=cv_px[y*240+x]==swap16(C_HI);return n;}
static void draw_test_glyph(pref_t r,int v){cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(r,v,30,C_HI));}
int main(void){
 setup(V_MIX,PART_909);
 pref_t comp=PR(R_ENG,T_MST,0,MST_COMP1);
 draw_test_glyph(comp,0);CHECK(cv_px[48*240+12]==swap16(C_HI));CHECK(cv_px[28*240+48]==swap16(C_HI));
 pref_t sends[]={PR(R_ENG,T_MIX,0,MX_REV),PR(R_ENG,T_MIX,0,MX_DLY),PR(R_ENG,T_FX,0,FX_RV_LEVEL),PR(R_ENG,T_FX,0,FX_DL_LEVEL)};
 for(int i=0;i<4;i++){draw_test_glyph(sends[i],0);CHECK(ink()==0);draw_test_glyph(sends[i],1);CHECK(ink()>0);draw_test_glyph(sends[i],64);int mid=ink();CHECK(mid>0);draw_test_glyph(sends[i],127);CHECK(ink()>mid);}
 for(int v=0;v<8;v++){draw_test_glyph(PR(R_SREP,0,0,0),v);for(int j=0;j<8;j++)CHECK(bar(12+j*5)==(j<=v?9:0));}
 pref_t feedback=PR(R_ENG,T_FX,0,FX_DL_FDBK);
 for(int type=0;type<2;type++){
  pref_set(PR(R_ENG,T_FX,0,FX_DL_TYPE),type);draw_test_glyph(feedback,0);CHECK(bar(15)>0);CHECK(bar(25)==0&&bar(35)==0&&bar(45)==0);
  draw_test_glyph(feedback,127);CHECK(type?bar(45)>bar(15):bar(45)<bar(15));
 }
 setup(V_FX,PART_909);*page_of(V_FX)=1;build_pages();pref_set(feedback,127);
 pref_set(PR(R_ENG,T_FX,0,FX_DL_TYPE),0);uint32_t a=knobs_sig();ui_frame();uint16_t before[240*KNOB_H];memcpy(before,fb+240*KNOB_Y,sizeof before);
 pref_set(PR(R_ENG,T_FX,0,FX_DL_TYPE),1);CHECK(knobs_sig()!=a);ui_frame();CHECK(memcmp(before,fb+240*KNOB_Y,sizeof before));
 pref_t refs[]={PR(R_ENG,T_MST,0,MST_COMP1),PR(R_ENG,T_FX,0,FX_DL_TIME),PR(R_ENG,T_FX,0,FX_DL_FDBK),PR(R_ENG,T_FX,0,FX_DL_LEVEL),PR(R_ENG,T_FX,0,FX_RV_LEVEL),PR(R_ENG,T_MIX,0,MX_DLY),PR(R_ENG,T_MIX,0,MX_REV),PR(R_SREP,0,0,0)};
 setup(V_FX,PART_909);
 for(unsigned r=0;r<sizeof refs/sizeof refs[0];r++)for(int type=0;type<2;type++) {
  pref_set(PR(R_ENG,T_FX,0,FX_DL_TYPE),type);const x0x_param_t*d=pref_desc(refs[r]);CHECK(d);
  for(int v=0;v<=d->max;v++) {
   cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(refs[r],v,30,C_HI));
   for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(x<10||x>50||y<26||y>50)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
  }
 }
 for(int i=3;i<=6;i++) {
  cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(refs[i],0,30,C_HI));int n=0;
  for(int j=0;j<240*KNOB_H;j++)n+=cv_px[j]==swap16(C_HI);CHECK(n==0);
  cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(refs[i],1,30,C_HI));n=0;
  for(int j=0;j<240*KNOB_H;j++)n+=cv_px[j]==swap16(C_HI);CHECK(n>0);
 }

 printf("semantic bounds PASS %u checks\n",checks);
 puts("GLYPH-S1/S2 regression PASS");return 0;
}
