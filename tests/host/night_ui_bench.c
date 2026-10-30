/* SPDX-License-Identifier: GPL-3.0-only */
/* Draw-only benchmark: the production renderer, same fixture and flags on both
 * source trees. Host CPU timings are comparisons, not device frame-time claims. */
#define SMC_UI_MAIN unused_smc_ui_main
#ifndef NIGHT_UI_SMC_FIXTURE
#define NIGHT_UI_SMC_FIXTURE "smc_ui_test.c"
#endif
#include NIGHT_UI_SMC_FIXTURE

static double cpu_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);
    return (double)t.tv_sec*1000.0+(double)t.tv_nsec/1000000.0;
}
static void paint(void) { draw_header();draw_main();draw_knobs(); }
static void measure(const char *name)
{
    enum { N=200 };
    project_t *p=malloc(sizeof proj);
    CHECK(p!=NULL);memcpy(p,&proj,sizeof proj);
    seq_t s=seq;
    paint();
    uint32_t b=blits;
    paint();
    CHECK(blits==b); /* a static page must still send no SPI rectangle */
    double t=cpu_ms();
    for(int i=0;i<N;i++) { memset(blit_hash,0,sizeof blit_hash);paint(); }
    double full=(cpu_ms()-t)/N;
    t=cpu_ms();b=blits;
    for(int i=0;i<N;i++)paint();
    double steady=(cpu_ms()-t)/N;
    CHECK(blits==b);
    CHECK(!memcmp(p,&proj,sizeof proj) && !memcmp(&s,&seq,sizeof seq));
    printf("DRAW_COST %s full_ms=%.6f steady_ms=%.6f static_blits=%u frames=%u\n",name,full,steady,blits-b,N);
    free(p);
}
int main(void)
{
    setup(V_HOME,PART_909);measure("HOME");
    for(int p=0;p<NPARTS;p++) {
        setup(V_PART,p);measure(part_s(p));
        if(p==PART_303A || p==PART_303B) {
            *page_of(V_PART)=pg.n-1;build_pages();measure(p==PART_303A?"MIAMI_SOUND":"CHICAGO_SOUND");
        }
    }
    setup(V_FX,PART_303B);measure("FX");
    setup(V_MIX,PART_303B);measure("MIX");
    *page_of(V_MIX)=4;build_pages();measure("WAVE");
    setup(V_SONG,PART_303B);song_fill(0);measure("SONG");
    setup(V_SEQ,PART_909);measure("DRUM_SEQ");
    setup(V_SEQ,PART_303B);measure("SYNTH_SEQ");
    setup(V_HOME,PART_303B);open_global();measure("GLOBAL");
    printf("night_ui_bench: PASS (%u cache/nonmutation checks)\n",checks);
    return 0;
}
