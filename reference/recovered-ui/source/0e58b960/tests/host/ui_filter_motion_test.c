/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
int main(void){setup(V_HOME,PART_909);for(int cut=0;cut<128;cut++)for(int res=0;res<128;res++){
 cv_begin(240,KNOB_H,C_BLACK);filter_glyph(30,20,0,cut,res,C_HI);
 for(int x=11;x<=49;x++){CHECK(cv_px[28*240+x]==swap16(C_DIM));CHECK(cv_px[38*240+x]==swap16(C_LINE));CHECK(cv_px[30*240+x]==swap16(C_BLACK));}
 }printf("FILTER OFF same pass baseline, unchanged floor, no old dropped line PASS %u assertions\n",checks);return 0;}
