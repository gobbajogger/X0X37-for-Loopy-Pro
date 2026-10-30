/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago ENV glyph follows the live filter EG: peak at CUTOFF at rest, swept
 * right by ENV on a note, decaying back; ENV 0 never moves; ENV 100 > ENV 56. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void drain_ms(int ms){int32_t a[512];for(int i=0;i<ms;i+=6){engine_render(a,256);}audio_due_ms=now_ms;run_ms(ms);}
static int peak_x(int env)
{   /* the column of the glyph's topmost lit pixel */
    pref_t r=PR(R_ENG,T_303,1,4);
    cv_begin(240,KNOB_H,C_BLACK); orig_strip_glyph(r,env,10,10,C_HI);
    for(int y=0;y<KNOB_H;y++) for(int x=10;x<10+OG_W;x++) if(cv_px[y*240+x]==swap16(C_HI)) {
        int x1=x; while(x1+1<10+OG_W && cv_px[y*240+x1+1]==swap16(C_HI)) x1++; return (x+x1)/2; }
    return -1;
}
static int sweep(int env, int *rest, int *top, int *late)
{
    pref_set(PR(R_ENG,T_303,1,4),env); pref_set(PR(R_ENG,T_303,1,2),40); drain_ms(200);
    *rest=peak_x(env);
    engine_bass_on_velocity(1,48,0,0,120); drain_ms(12); *top=peak_x(env);
    for(int k=0;k<40;k++){int x=peak_x(env); if(x>*top)*top=x; drain_ms(12);}
    engine_bass_off(1); drain_ms(3000); *late=peak_x(env);
    return *rest>=0;
}
int main(void)
{
    setup(V_PART,PART_303B);
    CHECK(engine_bass_sound_bank(&proj.sound,1)==ORIGINAL_SOUND_CHICAGO);
    int r0,t0,l0,r5,t5,l5,r9,t9,l9;
    CHECK(sweep(64,&r0,&t0,&l0));            /* ENV 0% (bipolar centre) */
    CHECK(sweep(64+36,&r5,&t5,&l5));         /* ENV ~56% */
    CHECK(sweep(127,&r9,&t9,&l9));           /* ENV 100% */
    printf("rest/top/late: 0%%=%d/%d/%d 56%%=%d/%d/%d 100%%=%d/%d/%d\n",r0,t0,l0,r5,t5,l5,r9,t9,l9);
    CHECK(t0==r0);                           /* no depth: the peak does not move */
    CHECK(t5>r5 && t9>t5-1 && t9>r9);        /* a note sweeps the peak right, more with ENV */
    CHECK(l5<t5 && l9<t9);                   /* and the EG decays it back */
    printf("CHICAGO ENV MOTION PASS %u assertions\n",checks);
    return 0;
}
