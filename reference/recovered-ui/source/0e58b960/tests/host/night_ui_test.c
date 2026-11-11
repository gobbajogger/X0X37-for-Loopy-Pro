/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual Night renderer: content is not assignment; no invented synth events. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void home_fixture(void)
{
    setup(V_HOME,PART_303B);
    for(int p=0;p<2;p++)for(int s=0;s<16;s+=4)sm_set(proj.pat[0].drum[p].hit[0],s);
    proj.pat[0].brk.steps=0x1111;
    ui_frame();
}
static void mini_content(void)
{
    home_fixture();
    CHECK(pat_part_bits16(&proj.pat[0],PART_909)==0x1111);
    CHECK(pat_part_bits16(&proj.pat[0],PART_808)==0x1111);
    CHECK(pat_part_bits16(&proj.pat[0],PART_BRK)==0x1111);
    for(int p=PART_303A;p<=PART_303B;p++) {
        CHECK(pat_part_bits16(&proj.pat[0],p)==0);
        int coloured=0,y=MAIN_Y+4+part_position(p)*4;
        for(int yy=y;yy<y+3;yy++)for(int x=6;x<23;x++)coloured+=fb[yy*240+x]==swap16(PART_COL[p]);
        printf("EMPTY_MINI part=%d coloured_pixels=%d expected=0\n",p,coloured);
        CHECK(coloured==0);
    }
    shot("night-home-content-corrected");
}
static void centered_cells(void)
{
    home_fixture();
    for(int pos=0;pos<NPARTS;pos++) {
        int p=part_at(pos),y=MAIN_Y+35+pos*20;
        for(int c=0;c<16;c++) {
            int x=58+c*11+(c>>2);
            CHECK(fb[(y+4)*240+x]==swap16(SOUND_FACE));
            CHECK(fb[(y+12)*240+x]==swap16((p<NKIT || p==PART_BRK) && c%4==0?dim(PART_COL[p],6):UI_N_EDGE));
            CHECK(fb[(y+13)*240+x]==swap16(SOUND_FACE));
            CHECK(fb[(y+5)*240+x]==swap16((p<NKIT || p==PART_BRK) && c%4==0?PART_COL[p]:UI_N_EDGE));
        }
    }
    printf("CELL_GEOMETRY: 16 cells per lane, 8px high, equal 5px outer margins\n");
}
static void all_empty(void)
{
    setup(V_HOME,PART_303B);
    project_t *before=malloc(sizeof proj);CHECK(before!=NULL);memcpy(before,&proj,sizeof proj);
    seq_t before_seq=seq;
    ui_frame();
    for(int pos=0;pos<NPARTS;pos++) {
        int p=part_at(pos),y=MAIN_Y+4+pos*4;
        CHECK(!part_used(&proj.pat[0],p));
        for(int yy=y;yy<y+3;yy++)for(int x=7;x<19;x++)
            CHECK(fb[yy*240+x]!=swap16(PART_COL[p]));
    }
    CHECK(!memcmp(before,&proj,sizeof proj));CHECK(!memcmp(&before_seq,&seq,sizeof seq));free(before);
    shot("night-home-empty-corrected");
}
static void page_inventory(void)
{
    static const struct {int view,part;const char *name;} cases[]={
      {V_SEQ,PART_909,"inventory-drum-seq"},{V_SEQ,PART_303B,"inventory-chicago-seq"},
      {V_PART,PART_BRK,"inventory-break"},{V_FX,PART_303B,"inventory-fx"},
      {V_MIX,PART_303B,"inventory-mix"},{V_SONG,PART_303B,"inventory-song"}};
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
        setup(cases[i].view,cases[i].part);ui_frame();shot(cases[i].name);
    }
    setup(V_PART,PART_303B);*page_of(V_PART)=pg.n-1;ui_frame();shot("night-chicago-sound-user-unchanged");
    setup(V_MIX,PART_303B);*page_of(V_MIX)=4;build_pages();ui_frame();shot("inventory-wave");
    setup(V_HOME,PART_303B);open_global();ui_frame();shot("inventory-global");
}
static void applied_filter(int mode,int cut,int res)
{
    engine_set(T_MST,0,MST_MODE,mode);engine_set(T_MST,0,MST_CUTOFF,cut);
    engine_set(T_MST,0,MST_RESO,res);run_ms(10);
}
static void filter_states(void)
{
    setup(V_HOME,PART_909);
    uint8_t mode,cut;int res;
    CHECK(proj.sound.v[T_MST][0][MST_DJF]==64);
    for(int m=0;m<=3;m++)for(int v=0;v<=127;v+=127) {
        applied_filter(m,v,v);
        filter_display(&mode,&cut,&res);CHECK(mode==m);CHECK(cut==v);CHECK(res==v);
        uint32_t sig=knobs_sig();applied_filter(m,v,v^1);
        CHECK(knobs_sig()!=sig);applied_filter(m,v,v);ui_frame();
    }
    applied_filter(1,72,100);ui_frame();shot("new-home-filter-lp");
    applied_filter(3,72,100);ui_frame();shot("new-home-filter-hp");
    applied_filter(2,72,100);ui_frame();shot("new-home-filter-bp");
    CHECK(proj.sound.v[T_MST][0][MST_DJF]==64);
    /* Macro lane first, direct mode/cutoff lanes later: actual HP must win. */
    int prm[]={MST_DJF,MST_MODE,MST_CUTOFF},vals[]={0,3,127};
    for(int i=0;i<3;i++) {
        int k=motion_alloc(proj.arr.lane,seq.ppat[PART_909],PART_909,T_MST,0,prm[i]);CHECK(k>=0);
        for(int j=0;j<NSTEPS;j++)proj.arr.lane[k].val[j]=(uint8_t)vals[i];
    }
    engine_motion_enable(1);seq_start(&seq);run_ms(40);
    filter_display(&mode,&cut,&res);CHECK(mode==3);CHECK(cut==127);
    char actual[32];engine_master_format(MST_MODE,actual);CHECK(!strcmp(actual,"HP"));
    ui_frame();shot("new-home-filter-motion-hp");
}
static void unknown_fx_label(void)
{
    setup(V_FX,PART_909);
    memset(proj.sound.bass[1].bytes,0,sizeof proj.sound.bass[1].bytes);
    CHECK(!strcmp(part_display(PART_303B),"CHOOSE ENGINE"));
    CHECK(tw(&FONT_T,part_s(PART_303B))<=49);
    proj.sound.v[T_MIX][PART_303B][1]=0;ui_frame();
    int y=MAIN_Y+14+part_position(PART_303B)*15;
    for(int yy=y+3;yy<y+10;yy++)for(int x=60;x<63;x++)
        CHECK(fb[yy*240+x]==swap16(PART_COL[PART_303B]));
    shot("new-fx-unknown-no-overlap");
}
static void inactive_fx_labels(void)
{
    setup(V_PART,PART_303B);
    for(int ty=0;ty<CHI_FX_NTYPES;ty++) {
        proj.sound.v[T_303][bidx()][CHI_FX_TYPE]=(uint8_t)ty;
        const char *a=matrix_name(PR(R_ENG,T_303,bidx(),CHI_FX_EDIT1));
        const char *b=matrix_name(PR(R_ENG,T_303,bidx(),CHI_FX_EDIT2));
        CHECK(strcmp(a,"UNUSED")!=0);CHECK(strcmp(b,"UNUSED")!=0);
        if(ty==CHI_FX_OFF){CHECK(!strcmp(a,"EDIT 1"));CHECK(!strcmp(b,"EDIT 2"));}
        if(ty==CHI_FX_COMP || ty==CHI_FX_DECIM)CHECK(!strcmp(b,"EDIT 2"));
    }
    proj.sound.v[T_303][bidx()][CHI_FX_TYPE]=CHI_FX_OFF;
    *page_of(V_PART)=pg.n-1;ui_frame();shot("new-chicago-inactive-fx");
}
static void send_endpoints(void)
{
    setup(V_FX,PART_909);
    for(int p=0;p<NPARTS;p++){proj.sound.v[T_MIX][p][1]=0;proj.sound.v[T_MIX][p][2]=127;}
    ui_frame();
    for(int pos=0;pos<NPARTS;pos++) {
        int p=part_at(pos),y=MAIN_Y+17+pos*15;
        CHECK(fb[y*240+60]==swap16(PART_COL[p]));
        CHECK(fb[y*240+227]==swap16(PART_COL[p]));
        CHECK(fb[y*240+59]!=swap16(PART_COL[p]));
        CHECK(fb[y*240+230]!=swap16(PART_COL[p]));
    }
    shot("new-fx-endpoints");
}
static void wave_latch(void)
{
    setup(V_MIX,PART_303B);*page_of(V_MIX)=4;build_pages();
    now_ms=2000;for(int i=0;i<SCOPE_N;i++)eng_scope[i]=10000;eng_scope_w++;
    draw_main();int y=MAIN_Y+34-10000*26/32768;
    CHECK(fb[y*240+8]==swap16(part_col()));
    now_ms=2050;for(int i=0;i<SCOPE_N;i++)eng_scope[i]=-10000;eng_scope_w++;
    draw_main();CHECK(fb[y*240+8]==swap16(part_col()));
    now_ms=2100;draw_main();y=MAIN_Y+34+10000*26/32768;
    CHECK(fb[y*240+8]==swap16(part_col()));shot("new-wave-latched-negative");
}
static void break_bounds(void)
{
    setup(V_PART,PART_BRK);memset(ui.outline,255,sizeof ui.outline);
    ui.outline_ok=1;ui.outline_slot=cur_pat()->brk.slot_a;
    draw_main();
    for(int slice=0;slice<8;slice++) {
        int x=8+slice*28+1;
        CHECK(fb[(MAIN_Y+6)*240+x]==swap16(part_col()));
        CHECK(fb[(MAIN_Y+48)*240+x]==swap16(part_col()));
        CHECK(fb[(MAIN_Y+5)*240+x]!=swap16(part_col()));
        CHECK(fb[(MAIN_Y+49)*240+x]!=swap16(part_col()));
    }
    for(int c=0;c<16;c++) {
        int x=8+c*14;CHECK(fb[(MAIN_Y+BAND_H+5)*240+x]==swap16(UI_N_EDGE));
        CHECK(x>=8 && x+12<=232);
    }
}
static void mini_height(void)
{
    home_fixture();
    for(int pos=0;pos<3;pos++) {
        int p=part_at(pos),y=MAIN_Y+4+pos*4;
        for(int yy=0;yy<3;yy++)CHECK(fb[(y+yy)*240+9]==swap16(PART_COL[p]));
        CHECK(fb[(y+3)*240+9]!=swap16(PART_COL[p]));
    }
    shot("new-home-taller-mini");
}
static void mini_centering(void)
{
    setup(V_HOME,PART_909);ui_frame();
    int first=-1,last=-1;
    /* Identify full12px cell edges, not the new short chamfer edge which
     * shares their colour. Cell coordinates and equal margins do not change. */
    for(int x=4;x<236;) {
        int start=x;
        while(x<236 && fb[(MAIN_Y+3)*240+x]==swap16(UI_N_EDGE))x++;
        if(x-start>=12) {if(first<0)first=start;last=x-1;}
        if(x==start)x++;
    }
    CHECK(first==7);CHECK(last==232);CHECK(first-3==236-last);
    for(int c=0;c<16;c++) {
        int x=7+c*14+(c>=8?4:0);
        for(int j=0;j<12;j++)CHECK(fb[(MAIN_Y+3)*240+x+j]==swap16(UI_N_EDGE));
    }
    /* Last bank/slot number and flashing cue remain inside the containing panel. */
    ui.pbank=(NPAT-1)/16;seq.ppat[PART_909]=NPAT-1;seq.pcue[PART_909]=NPAT-2;
    ui.frame=16;draw_main();
    for(int y=MAIN_Y+1;y<MAIN_Y+34;y++) {
        CHECK(fb[y*240+2]!=swap16(C_WHITE));CHECK(fb[y*240+237]!=swap16(C_WHITE));
    }
    shot("new-home-pattern-last-bank");
    home_fixture();shot("new-home-centered-patterns");
}
static void knob_ink_centering(void)
{
    home_fixture();
    for(int k=0;k<4;k++) {
        int lo=240,hi=-1;
        for(int y=173;y<=180;y++)for(int x=k*60+4;x<k*60+56;x++)
            if(((swap16(fb[y*240+x])>>5)&63)>15){if(x<lo)lo=x;if(x>hi)hi=x;}
        printf("LABEL_INK %d %d..%d\n",k,lo,hi);CHECK(abs(lo+hi-(k*120+59))<=1);
    }
    for(int v=0;v<=127;v++) {
        char n[16],u[8];engine_set(T_MST,0,MST_PUMP,v);run_ms(10);
        pref_value_of(PR(R_ENG,T_MST,0,MST_PUMP),v,n,u);
        cv_begin(240,82,C_BLACK);cell_value(209,53,n,u,C_HI);
        int lo=240,hi=-1;
        for(int y=50;y<73;y++)for(int x=177;x<237;x++)
            if(cv_px[y*240+x]!=swap16(C_BLACK)){if(x<lo)lo=x;if(x>hi)hi=x;}
        CHECK(lo+hi>=418 && lo+hi<=419); /* unavoidable halfpixel for odd-width ink */
    }
}
int main(void)
{
    mini_content();centered_cells();all_empty();page_inventory();filter_states();unknown_fx_label();inactive_fx_labels();send_endpoints();wave_latch();break_bounds();mini_height();mini_centering();knob_ink_centering();
    printf("night_ui: PASS (%u checks)\n",checks);return 0;
}
