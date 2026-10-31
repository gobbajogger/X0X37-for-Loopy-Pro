#define SMC_UI_MAIN unused_smc_ui_main
#include "/workspace/scratch/f88caed124c4/ui-glyph-semantic-20261027/tests/host/smc_ui_test.c"
static void drain(void){int32_t a[512];engine_render(a,256);audio_due_ms=now_ms;run_ms(20);}
static void ready(int view,int part){setup(view,part);audio_due_ms=now_ms;drain();}
static void bank(int v,int b){CHECK(!engine_bass_select_preset(0,0,0,&proj.sound));drain();CHECK(!engine_bass_select_preset(1,0,0,&proj.sound));drain();CHECK(!engine_bass_select_preset(v,b,0,&proj.sound));drain();CHECK(engine_bass_bank(v)==b);CHECK(!engine_bass_preset_pending(v));CHECK(engine_bass_is_wobble(v)==b);}
static int identity_index(int count,int val){if(count<=0)return -1;if(count<=8)return val;return val*7/(count-1);}
static void identity(int count,int val,int l,uint16_t col){int n=count>8?8:count,selected=identity_index(count,val);for(int j=0;j<n;j++)rbox(l+(j%4)*10,30+j/4*10,7,7,1,j==selected?col:C_LINE);if(!count)cv_rect(l+12,38,13,1,C_LINE);}
static const char* family(pref_t r){
 const x0x_param_t*d=pref_desc(r);
 if(r.kind==R_TEMPO)return "tempo";if(r.kind==R_SWING)return "swing";
 if(r.kind==R_ENG&&r.a==T_MST&&r.c==MST_DJF)return "filter";
 if(r.kind==R_ENG&&r.a==T_MST&&r.c==MST_PUMP)return "pump";
 if(r.kind==R_ENG&&r.a==T_MST&&r.c==MST_COMP1)return "compression";
 if(r.kind==R_ENG&&r.a==T_FX){switch(r.c){case FX_DL_TIME:return "delay-time";case FX_DL_FDBK:return "feedback";case FX_RV_DECAY:return "reverb-decay";case FX_RV_TONE:case FX_DL_TONE:return "tone-lowpass";case FX_RV_HPF:case FX_DL_HPF:return "highpass";case FX_DRIVE:return "drive";}}
 if(r.kind==R_SREP)return "repeat-count";
 if(r.kind==R_ENG&&((r.a==T_FX&&(r.c==FX_RV_LEVEL||r.c==FX_DL_LEVEL))||(r.a==T_MIX&&(r.c==MX_REV||r.c==MX_DLY))||name_is(d,"Rev")||name_is(d,"Dly")))return "send-return-level";
 if(is_pan(r)||r.kind==R_BTRANS||(r.kind==R_ENG&&r.a==T_BRK&&r.c==1))return "bipolar";
 if(r.kind==R_SPAT||r.kind==R_BRKSLOT||r.kind==R_SBAR)return "identity-position";
 if(r.kind==R_DLEN||r.kind==R_BLEN||r.kind==R_SLEN)return "length";
 if(r.kind==R_DRATE||r.kind==R_BRATE)return "step-rate";
 if(r.kind==R_BDIR)return "direction";if(r.kind==R_TAPE)return "tape";
 if(d&&d->names&&d->max<12)return "named-state";return "neutral-amount";
}
static int proposed(pref_t r,int val,int cx,uint16_t col){
 const x0x_param_t*d=pref_desc(r);int q=d&&d->max?(val*127+d->max/2)/d->max:0,l=cx-18;
 if(r.kind==R_ENG&&r.a==T_FX&&r.c==FX_DL_TIME){
  static const int gap[13]={2,3,4,4,5,5,6,6,7,8,8,9,10};int g=gap[val<13?val:0];
  cv_rect(l,48,37,1,C_LINE);cv_rect(l,29,2,18,col);
  for(int x=l+g;x<=cx+18;x+=g)cv_rect(x,35,1,12,col);
  cv_line(l,50,l+g,50,col);
 }else if(r.kind==R_ENG&&r.a==T_FX&&r.c==FX_RV_DECAY){
  float a=18,fb=.2f+.73f*q/127;
  cv_rect(l,48,37,1,C_LINE);
  for(int j=0;j<7;j++){int h=(int)(a+.5f);if(h)cv_rect(l+j*6,47-h,2,h,col);a*=fb;}
 }else if(r.kind==R_SPAT||r.kind==R_BRKSLOT||r.kind==R_SBAR){
  identity(r.kind==R_BRKSLOT?engine_brk_nslots():d->max+1,val,l,col);
 }else if(r.kind==R_TAPE){
  if(val<16){cv_line(l,43,l+8,43,col);cv_line(l+8,43,l+8,33,col);cv_line(l+8,33,l+18,33,col);cv_line(l+18,33,l+18,43,col);cv_line(l+18,43,l+28,43,col);cv_line(l+28,43,l+28,33,col);cv_line(l+28,33,l+36,33,col);}
  else{for(int s=-1;s<=1;s+=2){int x=cx+s*10;rbox(x-7,30,15,15,5,C_LINE);rbox(x-5,32,11,11,4,SOUND_FACE);cv_rect(x-1,36,3,3,col);cv_line(x,37,x+3,33,col);}cv_rect(l+2,48,33,1,C_LINE);int wear=(val-16)*32/111;cv_rect(l+2,47,1+wear,2,col);}
 }else if(r.kind==R_ENG&&r.a==T_FX&&(r.c==FX_RV_HPF||r.c==FX_DL_HPF)){
  int p=l+7+q*20/127;cv_rect(l,47,37,1,C_LINE);cv_line(l,46,p,32,col);cv_line(p,32,cx+18,32,col);
 }else if(r.kind==R_ENG&&r.a==T_FX&&r.c==FX_DRIVE){
  cv_line(l,32,l,46,col);cv_line(l,32,l+12,39,col);cv_line(l+12,39,l,46,col);cv_line(l+12,39,cx+18,39,C_LINE);cv_rect(l+14,38,1+q*21/127,3,col);
 }else return 0;return 1;
}
static void body(pref_t r,int v,int cx,int proposal){
 if(proposal&&proposed(r,v,cx,C_HI))return;
 if(perform_glyph(r,v,cx,C_HI)||control_glyph(r,v,cx,C_HI))return;
 const x0x_param_t*d=pref_desc(r);if(d&&d->names){int n=d->max+1,pw=n>6?3:6,gap=2,w0=n*(pw+gap)-gap;for(int j=0;j<n;j++)box(cx-w0/2+j*(pw+gap),33,pw,9,j==v?C_HI:C_LINE);}
}
int main(void){
 ready(V_FX,PART_909);song_fill(15);
 pref_t refs[]={PR(R_TEMPO,0,0,0),PR(R_SWING,0,0,0),PR(R_ENG,T_MST,0,MST_DJF),PR(R_ENG,T_MST,0,MST_PUMP),PR(R_ENG,T_MST,0,MST_COMP1),PR(R_ENG,T_FX,0,FX_DL_TIME),PR(R_ENG,T_FX,0,FX_DL_FDBK),PR(R_ENG,T_FX,0,FX_RV_DECAY),PR(R_ENG,T_FX,0,FX_RV_TONE),PR(R_ENG,T_FX,0,FX_RV_HPF),PR(R_ENG,T_FX,0,FX_DL_LEVEL),PR(R_ENG,T_FX,0,FX_RV_LEVEL),PR(R_ENG,T_MIX,0,MX_PAN),PR(R_BTRANS,0,0,0),PR(R_SPAT,0,0,0),PR(R_BRKSLOT,0,0,0),PR(R_SBAR,0,0,0),PR(R_DLEN,0,0,0),PR(R_DRATE,0,0,0),PR(R_BDIR,0,0,0),PR(R_TAPE,0,0,0),PR(R_SREP,0,0,0),PR(R_ENG,T_FX,0,FX_DRIVE),PR(R_CLKOUT,0,0,0),PR(R_ENG,T_MIX,0,MX_LEVEL)};
 for(unsigned k=0;k<sizeof refs/sizeof refs[0];k++){
  pref_t r=refs[k];const x0x_param_t*d=pref_desc(r);if(!d)continue;
  for(int proposal=0;proposal<2;proposal++){
   memset(fb,0,sizeof fb);cv_begin(240,KNOB_H,C_BLACK);
   cv_text(3,2,&FONT_T,proposal?"PROPOSAL / NOT IMPLEMENTED":"CURRENT / SOURCE 5F58A199",C_HI);
   for(int e=0;e<4;e++){int v=e==0?0:e==1?1:e==2?(d->max+1)/2:d->max,cx=30+60*e;uint16_t keep[240*KNOB_H];memcpy(keep,cv_px,sizeof keep);pref_set(r,v);drain();cv_begin(240,KNOB_H,C_BLACK);memcpy(cv_px,keep,sizeof keep);body(r,v,cx,proposal);char n[20],u[8];put_i(n,v);text_c(cx,55,&FONT_T,n,C_HI);}
   cv_commit(3,0,0);char n[80];snprintf(n,sizeof n,"family-%02u-%s",k,proposal?"proposed":"current");shot(n);
  }
  printf("FAMILY\t%u\t%s\t%s\tmax%d\n",k,family(r),pref_name(r),d->max);
 }
 const int pv[]={V_HOME,V_SEQ,V_PART,V_FX,V_FX,V_MIX,V_SONG,V_GEN};
 const int pp[]={PART_909,PART_303B,PART_BRK,PART_909,PART_909,PART_909,PART_909,PART_303B};
 const int pgx[]={0,0,0,0,1,3,1,0};
 for(int k=0;k<8;k++)for(int e=0;e<3;e++){
 ready(pv[k],pp[k]);song_fill(15);*page_of(pv[k])=pgx[k];build_pages();
 for(int i=0;i<4;i++){pref_t r=pg.r[cur_page()][i];const x0x_param_t*d=pref_desc(r);if(d)pref_set(r,e==0?0:e==1?(d->max+1)/2:d->max);}
 drain();for(int i=0;i<4;i++){pref_t r=pg.r[cur_page()][i];if(r.kind==R_ENG&&r.a==T_MST)CHECK(engine_master_get(r.c)==pref_get(r));}ui_frame();char name[80];snprintf(name,sizeof name,"context-%d-%d-current",k,e);shot(name);
 cv_begin(240,KNOB_H,C_BLACK);memcpy(cv_px,fb+KNOB_Y*240,KNOB_H*240*sizeof *cv_px);
 for(int i=0;i<4;i++){pref_t r=pg.r[cur_page()][i];int cx=30+60*i;uint16_t saved[42*25];for(int y=26;y<=50;y++)memcpy(saved+(y-26)*42,cv_px+y*240+cx-20,42*sizeof *cv_px);cv_rect(cx-20,26,42,25,SOUND_FACE);if(!proposed(r,pref_get(r),cx,drum_mix_page()>=0?PART_COL[drum_mix_page()]:part_col()))for(int y=26;y<=50;y++)memcpy(cv_px+y*240+cx-20,saved+(y-26)*42,42*sizeof *cv_px);}
 cv_commit(3,0,KNOB_Y);snprintf(name,sizeof name,"context-%d-%d-PROPOSED",k,e);shot(name);
 }
 // Specialized SOUND retains real matrix, never routed through strip dispatcher.
 for(int mode=0;mode<2;mode++){
 ready(V_PART,PART_303B);bank(1,mode);build_pages();int count=pg.n;
 for(int p=0;p<count;p++)for(int edge=0;edge<3;edge++){*page_of(V_PART)=p;build_pages();for(int i=0;i<4;i++){pref_t r=pg.r[p][i];const x0x_param_t*d=pref_desc(r);if(d&&r.kind!=R_BENGINE&&r.kind!=R_BPRESET)pref_set(r,edge==0?0:edge==1?(d->max+1)/2:d->max);}drain();ui_frame();char n[80];snprintf(n,sizeof n,"sound-mode%d-page%d-edge%d-current-REUSE",mode,p,edge);shot(n);}
 }
 ready(V_SEQ,PART_303B);ui.held_step=0;ui_frame();shot("held-step-current-REUSE");
 const int views[]={V_HOME,V_SEQ,V_GEN,V_FX,V_MIX,V_SONG,V_GLO,V_PART};
 for(int mode=0;mode<2;mode++)for(unsigned vi=0;vi<8;vi++)for(int part=0;part<NPARTS;part++){
  ready(views[vi],part);if(is_303()){bank(bidx(),mode);}build_pages();int count=pg.n;
  for(int p=0;p<count;p++){*page_of(views[vi])=p;build_pages();for(int i=0;i<4;i++){pref_t r=pg.r[cur_page()][i];const x0x_param_t*d=pref_desc(r);if(d)printf("CALLER\t%d\t%d\t%d\t%d\t%s\t%d,%d,%d,%d\t%s\t%d\t%s\t%s\n",mode,views[vi],part,p,pg.title[p],r.kind,r.a,r.b,r.c,pref_name(r),d->max,views[vi]==V_PART&&(is_drum()||is_303())?"SOUND-MATRIX":family(r),d->names?"named":"numeric");}}
 }
 for(int kit=0;kit<2;kit++)for(int voice=0;voice<NDRUM;voice++){
 ready(V_PART,kit);ui.sel[kit]=voice;build_pages();int count=pg.n;
 for(int p=0;p<count;p++){*page_of(V_PART)=p;build_pages();for(int i=0;i<4;i++){pref_t r=pg.r[p][i];const x0x_param_t*d=pref_desc(r);if(d)printf("DRUM_CALLER\t%d\t%d\t%d\t%s\t%d,%d,%d,%d\t%s\t%d\tSOUND-MATRIX\n",kit,voice,p,pg.title[p],r.kind,r.a,r.b,r.c,pref_name(r),d->max);}ui_frame();char n[80];snprintf(n,sizeof n,"drum-kit%d-voice%d-page%d-current-REUSE",kit,voice,p);shot(n);}
 }
 ready(V_FX,PART_909);
 for(unsigned k=0;k<sizeof refs/sizeof refs[0];k++){const x0x_param_t*d=pref_desc(refs[k]);if(!d)continue;for(int v=0;v<=d->max;v++){cv_begin(240,KNOB_H,C_BLACK);if(proposed(refs[k],v,30,C_HI))for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(x<10||x>50||y<26||y>50)CHECK(cv_px[y*240+x]==swap16(C_BLACK));}}
 printf("PROPOSAL bounds PASS %u assertions\n",checks);
 for(int n=0;n<=32;n++){if(!n)CHECK(identity_index(n,0)==-1);for(int v=0;v<n;v++){CHECK(identity_index(n,v)>=0);CHECK(identity_index(n,v)<(n>8?8:n));if(n<=8)CHECK(identity_index(n,v)==v);}}
 ready(V_FX,PART_909);*page_of(V_FX)=1;build_pages();
 for(int v=0;v<=12;v++){pref_set(PR(R_ENG,T_FX,0,FX_DL_TIME),v);drain();ui_frame();cv_begin(240,KNOB_H,C_BLACK);memcpy(cv_px,fb+KNOB_Y*240,KNOB_H*240*sizeof *cv_px);cv_rect(10,26,42,25,SOUND_FACE);proposed(PR(R_ENG,T_FX,0,FX_DL_TIME),v,30,part_col());cv_commit(3,0,KNOB_Y);char n[64];snprintf(n,sizeof n,"time-all-%02d-PROPOSED",v);shot(n);}
 for(int v=15;v<=16;v++){pref_set(PR(R_TAPE,0,0,0),v);drain();CHECK(pref_get(PR(R_ENG,T_FX,0,FX_DL_TYPE))==(v>=16));ui_frame();cv_begin(240,KNOB_H,C_BLACK);memcpy(cv_px,fb+KNOB_Y*240,KNOB_H*240*sizeof *cv_px);cv_rect(190,26,42,25,SOUND_FACE);proposed(PR(R_TAPE,0,0,0),v,210,part_col());cv_commit(3,0,KNOB_Y);char n[64];snprintf(n,sizeof n,"tape-boundary-%d-PROPOSED",v);shot(n);}
 ready(V_HOME,PART_909);
 cv_begin(240,KNOB_H,C_BLACK);for(int m=0;m<4;m++){filter_glyph(30+m*60,28,m,64,64,C_HI);text_c(30+m*60,55,&FONT_T,m==0?"OFF":m==1?"LP":m==2?"BP":"HP",C_HI);}cv_commit(3,0,0);shot("filter-all-modes-CURRENT-REUSE");
 printf("V2 edge/backend/applied assertions PASS %u\n",checks);
 puts("DESIGN ONLY; no production mutations; host synthetic render, not hardware");return 0;
}
