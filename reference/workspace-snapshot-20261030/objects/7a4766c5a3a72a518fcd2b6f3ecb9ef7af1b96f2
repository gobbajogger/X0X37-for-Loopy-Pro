/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
int main(void)
{
 const int views[]={V_HOME,V_SEQ,V_GEN,V_FX,V_MIX,V_SONG,V_GLO,V_PART};
 unsigned slots=0,pages=0;
 for(unsigned vi=0;vi<sizeof views/sizeof views[0];vi++)for(int part=0;part<NPARTS;part++) {
  setup(views[vi],part);
  int count=pg.n;
  for(int page=0;page<count;page++) {
   *page_of(views[vi])=page;build_pages();
   for(int i=0;i<4;i++) {
    pref_t r=pg.r[cur_page()][i];const x0x_param_t *d=pref_desc(r);if(!d)continue;
    slots++;
    for(int edge=0;edge<3;edge++) {
     int val=edge==0?0:edge==1?d->max/2:d->max;
     cv_begin(240,KNOB_H,C_BLACK);
     control_glyph(r,val,30,C_HI);
     for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)
      if(x<10 || x>50 || y<26 || y>50)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
    }
   }
   project_t *b=malloc(sizeof proj);CHECK(b);memcpy(b,&proj,sizeof proj);ui_frame();CHECK(!memcmp(b,&proj,sizeof proj));free(b);
   pages++;
   if((part==PART_909 && views[vi]!=V_PART) || (part==PART_BRK && views[vi]==V_PART) || (part==PART_303B && views[vi]==V_SEQ)) {
    char name[80];snprintf(name,sizeof name,"shared-view%d-part%d-page%d",views[vi],part,page);shot(name);
   }
  }
 }
 /* Bipolar centre and endpoints must be exact, including even max=48. */
 pref_t pans[]={PR(R_ENG,T_MIX,0,MX_PAN),PR(R_BTRANS,0,0,0)};
 for(int i=0;i<2;i++)for(int j=0;j<3;j++) {
  const x0x_param_t *d=pref_desc(pans[i]);int v=j==0?0:j==1?(d->max+1)/2:d->max;
  cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(pans[i],v,30,C_HI));
  int x=j==0?12:j==1?30:48;CHECK(cv_px[34*240+x]==swap16(C_HI));
 }
 cv_begin(240,KNOB_H,C_BLACK);CHECK(!control_glyph(PR(R_CLKOUT,0,0,0),0,30,C_HI));
 test_distinct_maps();
 printf("shared_glyph PASS (%u checks, %u mapped slots, %u pages)\n",checks,slots,pages);return 0;
}
