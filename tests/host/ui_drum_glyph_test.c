/* SPDX-License-Identifier: GPL-3.0-only */
/* Drum part-page glyphs: every visible 909/808 control has a glyph role that
 * matches what it does, BRK and other views keep theirs, and a hit animates the
 * cell from the real voice envelope, then the cell goes back to its idle image. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void drain_ms(int ms) { int32_t a[512]; for (int i = 0; i < ms; i += 6) engine_render(a, 256); audio_due_ms = now_ms; run_ms(ms); }
static int role_of(int p, int k, const char *want)
{
    pref_t r = pg.r[p][k];
    if (r.kind == R_NONE) return 0;
    if (want) { const char *n = matrix_name(r); size_t i = 0;
        for (; want[i] && n[i]; i++) if ((n[i] | 32) != (want[i] | 32)) { fprintf(stderr, "name %s != %s\n", n, want); CHECK(0); }
        if (want[i] || n[i]) fprintf(stderr, "name %s != %s\n", n, want);
        CHECK(!want[i] && !n[i]); }
    return drum_strip_role(r);
}
static uint32_t cell_hash(int k)
{   /* page row 0, control k: the 26x19 glyph box */
    uint32_t h = 2166136261u;
    for (int y = 40; y < 64; y++) for (int x = k * 60 + 2; x < k * 60 + 30; x++) h = (h ^ fb[240 * y + x]) * 16777619u;
    return h;
}
static void test_dispatch(void)
{
    setup(V_PART, PART_909); ui.sel[PART_909] = 0; *page_of(V_PART) = 0; build_pages();
    CHECK(role_of(0, 0, "Tune") == DG_BDSWEEP); CHECK(role_of(0, 1, "Attack") == DG_ATTACK);
    CHECK(role_of(0, 2, "Decay") == DG_DECAY);
    ui.sel[PART_909] = 1; build_pages();
    CHECK(role_of(0, 0, "Tune") == DG_TUNE); CHECK(role_of(0, 1, "Tone") == DG_NTAIL);
    CHECK(role_of(0, 3, "Snappy") == DG_SNAPPY);
    setup(V_PART, PART_808); ui.sel[PART_808] = 0; *page_of(V_PART) = 0; build_pages();
    CHECK(role_of(0, 1, "Tone") == DG_LPF);
    ui.sel[PART_808] = 1; build_pages(); CHECK(role_of(0, 1, "Tone") == DG_SHELLS);
    ui.sel[PART_808] = 2; build_pages(); int found = 0; for (int k = 0; k < 4; k++) if (pg.r[0][k].kind != R_NONE && !strcmp(matrix_name(pg.r[0][k]), "SOUND")) { found = 1; CHECK(drum_strip_role(pg.r[0][k]) == DG_SOUND); } CHECK(found);
    /* every drawn control on every 909/808 voice page has a glyph */
    for (int part = PART_909; part <= PART_808; part++) for (int v = 0; v < 11; v++) {
        setup(V_PART, part); ui.sel[part] = (uint8_t)v; *page_of(V_PART) = 0; build_pages();
        for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++)
            if (pg.r[p][k].kind != R_NONE && strcmp(pg.title[p], "KNOBS 5-8")) CHECK(role_of(p, k, NULL) || orig_strip_slot(pg.r[p][k]));
    }
    setup(V_MIX, PART_909); build_pages();
    CHECK(drum_strip_role(find_ref(T_909,0,"Level")) != DG_NONE);
    CHECK(drum_strip_role(find_ref(T_808,0,"Level")) != DG_NONE);
    setup(V_PART, PART_BRK); build_pages();
    for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++) CHECK(!role_of(p, k, NULL));
    setup(V_FX, PART_909); build_pages();
    for (int p = 0; p < pg.n; p++) for (int k = 0; k < 4; k++) CHECK(!role_of(p, k, NULL));
    puts("PASS dispatch: 909/808 roles by control, BRK and other views untouched");
}
static void test_motion(int part)
{
    setup(V_PART, part); ui.sel[part] = 0; *page_of(V_PART) = 0; build_pages();
    drain_ms(300); ui_frame(); uint32_t idle = cell_hash(3), d0 = cell_hash(part == PART_909 ? 2 : 3);
    uint32_t a; float e; CHECK(!engine_drum_view(part, 0, &a, &e));
    engine_drum(part, 0, 1.0f); drain_ms(20); now_ms++; ui_frame();
    CHECK(engine_drum_view(part, 0, &a, &e)); float e1 = e; CHECK(e1 > 0.2f);
    uint32_t h1 = cell_hash(part == PART_909 ? 2 : 3); CHECK(h1 != d0);
    drain_ms(150); now_ms++; ui_frame();
    CHECK(engine_drum_view(part, 0, &a, &e)); CHECK(e < e1);                 /* the tail shrinks */
    CHECK(cell_hash(part == PART_909 ? 2 : 3) != h1);                         /* the playhead moves */
    drain_ms(6000); now_ms++; ui_frame();
    CHECK(!engine_drum_view(part, 0, &a, &e)); CHECK(cell_hash(3) == idle);   /* idle again */
    printf("PASS motion %s: hit animates the decay cell, then it rests\n", part == PART_909 ? "909" : "808");
}
static uint32_t direct_drum(pref_t r,int value)
{
 cv_begin(240,KNOB_H,C_BLACK);drum_strip_glyph(r,value,10,10,C_HI);
 uint32_t h=2166136261u;
 for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK)){
  if(!(x>=10 && x<36 && y>=10 && y<=28))fprintf(stderr,"drum bounds kit=%u voice=%u control=%u value=%d xy=%d,%d\n",r.a,r.b,r.c,value,x,y);
  CHECK(x>=10 && x<36 && y>=10 && y<=28);
 }
 for(int y=10;y<=28;y++)for(int x=10;x<36;x++)h=(h^cv_px[y*240+x])*16777619u;
 return h;
}
/* Pitch motion occupies rows1..5; a two-row blank gutter keeps the lower
 * amplitude waveform readable at both high TUNE values and throughout a hit. */
static uint32_t tune_wave_band(pref_t r,int value)
{
 direct_drum(r,value);
 for(int y=16;y<=17;y++)for(int x=10;x<36;x++)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
 uint32_t h=2166136261u;
 for(int y=18;y<=26;y++)for(int x=10;x<36;x++)h=(h^cv_px[y*240+x])*16777619u;
 return h;
}
static void test_tune_band_separation(void)
{
 const int pots[2]={96,127};
 for(unsigned i=0;i<2;i++){
  setup(V_PART,PART_808);pref_t r=find_ref(T_808,0,"Tune");CHECK(drum_strip_role(r)==DG_TUNE);
  pref_set(r,pots[i]);drain_ms(300);tune_wave_band(r,pots[i]);
  engine_drum(PART_808,0,1.0f);drain_ms(20);
  uint32_t age;float first,late;CHECK(engine_drum_view(PART_808,0,&age,&first));
  uint32_t early=tune_wave_band(r,pots[i]);drain_ms(150);
  CHECK(engine_drum_view(PART_808,0,&age,&late));CHECK(late<first);
  CHECK(tune_wave_band(r,pots[i])!=early); /* Lower band still follows the real EG. */
 }
 puts("PASS TUNE pitch/wave gutter at75/100%, idle and real hit decay");
}

static void test_value_motion_bounds(void)
{
 for(int part=PART_909;part<=PART_808;part++)for(int voice=0;voice<NDRUM;voice++){
  setup(V_PART,part);ui.sel[part]=(uint8_t)voice;build_pages();if(part==PART_909 && voice==0)pref_set(PR(R_ENG,T_909,0,4),64);drain_ms(300);
  for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++){
   pref_t r=pg.r[p][k];int role=drum_strip_role(r);if(!role)continue;
   uint32_t previous=0;const x0x_param_t *d=pref_desc(r);
   for(int v=0;v<5;v++){uint32_t h=direct_drum(r,v*d->max/4);if(v && role!=DG_SOUND){if(h==previous)fprintf(stderr,"drum value collision kit=%u voice=%u ctrl=%u role=%d value=%d\n",r.a,r.b,r.c,role,v*d->max/4);CHECK(h!=previous);}previous=h;}
  }
  engine_drum(part,voice,1.0f);drain_ms(8);
  for(int f=0;f<14;f++){
   for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++){pref_t r=pg.r[p][k];if(drum_strip_role(r))direct_drum(r,matrix_value(r));}
   drain_ms(7);
  }
 }
 for(int part=PART_909;part<=PART_808;part++){
  setup(V_MIX,part);pref_t r=find_ref(part==PART_909?T_909:T_808,0,"Level");CHECK(drum_strip_role(r)==DG_LEVEL);
  int page=-1;for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_ENG && pg.r[p][k].a==r.a && pg.r[p][k].b==r.b && pg.r[p][k].c==r.c)page=p;
  CHECK(page>=0);*page_of(V_MIX)=(uint8_t)page;build_pages();drain_ms(300);ui_frame();
  uint32_t idle=direct_drum(r,100),sig=knobs_sig(),strip=2166136261u;
  for(int y=KNOB_Y+28;y<KNOB_Y+52;y++)for(int x=10;x<52;x++)strip=(strip^fb[y*240+x])*16777619u;
  engine_drum(part,0,1.0f);drain_ms(12);ui_frame();CHECK(direct_drum(r,100)!=idle);CHECK(knobs_sig()!=sig);
  uint32_t live_strip=2166136261u;
  for(int y=KNOB_Y+28;y<KNOB_Y+52;y++)for(int x=10;x<52;x++)live_strip=(live_strip^fb[y*240+x])*16777619u;
  CHECK(live_strip!=strip); /* Real LEVEL knob cell must use live dispatch too. */
 }
 puts("PASS drum continuous values, native bounds, real live roles and MIX level cache");
}
int main(void) { test_dispatch(); test_motion(PART_909); test_motion(PART_808); test_tune_band_separation(); test_value_motion_bounds(); printf("PASS ui-drum-glyph (%u checks)\n", checks); return 0; }
