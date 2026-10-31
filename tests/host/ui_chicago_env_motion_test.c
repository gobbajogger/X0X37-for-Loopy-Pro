/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago ENV glyph follows the live filter EG: peak at CUTOFF at rest, swept
 * right by ENV on a note, decaying back; ENV 0 never moves; ENV 100 > ENV 56. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void drain_ms(int ms){int32_t a[512];for(int i=0;i<ms;i+=6){engine_render(a,256);}audio_due_ms=now_ms;run_ms(ms);}
static int last_y=-1, ymin=999, ymax=-1;  /* the bump's top row: must never change */
static int peak_x(int env)
{   /* the column of the glyph's topmost lit pixel (the resonance bump) */
    pref_t r=PR(R_ENG,T_303,1,4);
    cv_begin(240,KNOB_H,C_BLACK); orig_strip_glyph(r,env,10,10,C_HI);
    for(int y=0;y<KNOB_H;y++) for(int x=10;x<10+OG_W;x++) if(cv_px[y*240+x]!=swap16(C_BLACK) && cv_px[y*240+x]!=swap16(C_LINE)) {
        int x1=x; while(x1+1<10+OG_W && cv_px[y*240+x1+1]!=swap16(C_BLACK) && cv_px[y*240+x1+1]!=swap16(C_LINE)) x1++;
        last_y=y; if(y<ymin)ymin=y; if(y>ymax)ymax=y; return (x+x1)/2; }
    return -1;
}
static int lo_x=999;
static int sweep(int env, int *rest, int *top, int *late)
{
    pref_set(PR(R_ENG,T_303,1,4),env); pref_set(PR(R_ENG,T_303,1,2),64); pref_set(PR(R_ENG,T_303,1,3),90); drain_ms(200);
    *rest=peak_x(env); lo_x=*rest;
    engine_bass_on_velocity(1,48,0,0,120); drain_ms(12); *top=peak_x(env);
    for(int k=0;k<40;k++){int x=peak_x(env); if(env<64 ? x<*top : x>*top)*top=x; if(x<lo_x)lo_x=x; drain_ms(12);}
    engine_bass_off(1); drain_ms(3000); *late=peak_x(env);
    return *rest>=0;
}
int main(void)
{
    setup(V_PART,PART_303B);
    CHECK(engine_bass_sound_bank(&proj.sound,1)==ORIGINAL_SOUND_CHICAGO);
    char num[16],unit[8];
    pref_value_of(PR(R_ENG,T_303,1,4),96,num,unit);
    CHECK(num[0]=='+'); /* Positive signed depth is visible. */
    int r0,t0,l0,r5,t5,l5,r9,t9,l9;
    CHECK(sweep(64,&r0,&t0,&l0));            /* pot 50%: ENV 0 (bipolar centre) */
    CHECK(sweep(64+36,&r5,&t5,&l5));         /* pot ~78%: ENV +56% */
    CHECK(sweep(127,&r9,&t9,&l9));           /* pot 100%: ENV +100% */
    printf("rest/top/late: 0%%=%d/%d/%d 56%%=%d/%d/%d 100%%=%d/%d/%d\n",r0,t0,l0,r5,t5,l5,r9,t9,l9);
    CHECK(t0==r0);                           /* no depth: the peak does not move */
    CHECK(t5>r5 && t9>t5-1 && t9>r9);        /* a note sweeps the peak right, more with ENV */
    CHECK(l5<t5 && l9<t9);                   /* and the EG decays it back */
    CHECK(l0==r0 && l5==r5 && l9==r9);       /* release returns it to CUTOFF */
    CHECK(t9-r9>=7);                        /* 100% uses most of the 26 px cell */
    CHECK(t5-r5>=1 && t5<t9);
    int rn,tn,ln,rq,tq,lq,ln0,lq0;
    CHECK(sweep(0,&rn,&tn,&ln)); ln0=lo_x;      /* pot 0%: ENV -100%, sweeps LEFT */
    CHECK(sweep(32,&rq,&tq,&lq)); lq0=lo_x;     /* pot 25%: ENV -50% */
    printf("negative rest/min/late: 0%%=%d/%d/%d 25%%=%d/%d/%d\n",rn,ln0,ln,rq,lq0,lq);
    CHECK(tn<=rn && tq<=rq);                 /* never right of CUTOFF */
    CHECK(ln0<lq0 && lq0<rq && rn-ln0>=6);   /* left, more for -100% */
    CHECK(ln==rn && lq==rq);
    { char n[24]="",u[8]=""; pref_value_of(PR(R_ENG,T_303,1,4),127,n,u); CHECK(!strcmp(n,"+100"));
      pref_value_of(PR(R_ENG,T_303,1,4),0,n,u); CHECK(!strcmp(n,"-100"));
      pref_value_of(PR(R_ENG,T_303,1,4),64,n,u); printf("readout pot0=-100 pot64=%s%s\n",n,u); CHECK(!strcmp(n,"0")); }
    printf("peak row min/max %d/%d\n",ymin,ymax);
    CHECK(ymin==ymax);                       /* the bump height never follows the EG */
    printf("CHICAGO ENV MOTION PASS %u assertions\n",checks);
    return 0;
}
