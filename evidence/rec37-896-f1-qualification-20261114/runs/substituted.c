#define BASELINE 1

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rec_outer_keymap.h"
#define BOARD_SMK37 1
enum { V_HOME, V_PART, V_GEN, V_FX, V_MIX, V_GLO, V_SONG, V_PERF, V_SEQ, NVIEWS };
enum { O_NONE, O_LIST, O_ASK };
enum { PART_909, PART_808, PART_303A, PART_303B, PART_BRK, NPARTS };
#define NBASS 2                      /* 303 A, 303 B */
enum { B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC,
       B_OCTDN, B_OCTUP, NB };
struct { int rec, part, view, oct, overlay; uint32_t btn; uint8_t freeplay[NBASS]; } ui;
struct { int playing; } seq;
static uint64_t physical_keys;
static int project_busy, perf_active, clip_active;
static unsigned checks, failures, predicate_differences, admission_differences, transition_differences;
static unsigned fresh_cases, transition_cases, ons, offs, intent_seq, intent_audition;
static int active[37], last_pitch[37], last_intent[37];
#define T(x) do { checks++; if (!(x)) { if (failures < 8) fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x); failures++; } } while (0)
static int perf_testing(void) { return perf_active; }
uint8_t smk_clip_mode_on(void) { return (uint8_t)clip_active; }
static uint64_t plat_physical_keys(void) { return physical_keys; }
static int part_is_303(int part) { return part == PART_303A || part == PART_303B; }
static int is_303(void) { return part_is_303(ui.part); }
static int bidx(void) { return ui.part == PART_303B ? 1 : 0; }
static int kbd_on(void) {return ui.view==V_SEQ || ui.view==V_PART;}
static unsigned live_token(unsigned physical)
{ return physical>=5 && physical<32?physical-4:physical<5?physical+28:physical+1; }
static void bass_kbd_at(int identity, int pitch, int step_record, int vel_key)
{
    int p = identity < 33 ? identity - 28 : identity - 1;
    T(p >= 0 && p < 37 && (p < 5 || p >= 32));
    T(!active[p]); T(vel_key == -1);
    active[p] = 1; last_pitch[p] = pitch; last_intent[p] = step_record;
    ons++; if (step_record) intent_seq++; else intent_audition++;
}
static int bass_kbd_release(int identity)
{
    int p = identity < 33 ? identity - 28 : identity - 1;
    T(p >= 0 && p < 37 && active[p]); active[p] = 0; offs++; return 1;
}
static void bass_kbd_physical(int p, int n)
{
    bass_kbd_at((int)live_token((unsigned)p), n, ui.view == V_SEQ, -1);
}
static uint16_t rec_outer_prev, rec_outer_suppress, rec_outer_admitted;
static uint8_t rec_outer_armed;

static void rec_outer_keys(void)
{
    uint16_t phys = rec_outer_bits(plat_physical_keys());
    unsigned i;
    int admit = ui.rec && is_303() && kbd_on() && !project_busy && ui.overlay != O_ASK && !perf_testing();
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
    if (smk_clip_mode_on()) admit = 0;
#endif
    if (!admit) {
        if (rec_outer_admitted) {
            for (i = 0; i < 10; i++) {
                unsigned p = rec_outer_physical(i);
                uint16_t bit = (uint16_t)(1u << i);
                if (rec_outer_admitted & bit)
                    (void)bass_kbd_release((int)live_token(p));
            }
            rec_outer_admitted = 0;
        }
        rec_outer_prev = phys;
        rec_outer_suppress = phys;   /* already-held keys stay quiet at the next REC entry */
        rec_outer_armed = 0;
        return;
    }
    if (!rec_outer_armed) {
        rec_outer_suppress = phys;
        rec_outer_prev = phys;
        rec_outer_armed = 1;
        return;
    }
    for (i = 0; i < 10; i++) {
        unsigned p = rec_outer_physical(i);
        uint16_t bit = (uint16_t)(1u << i);
        int on  = !!(phys & bit);
        int was = !!(rec_outer_prev & bit);
        if (on && (rec_outer_suppress & bit))
            continue;
        if (!on)
            rec_outer_suppress &= (uint16_t)~bit;
        if (on && !was) {
            bass_kbd_physical((int)p, 36 + (int)p + 12 * ui.oct);
            rec_outer_admitted |= bit;
        } else if (!on && was && (rec_outer_admitted & bit)) {
            (void)bass_kbd_release((int)live_token(p));
            rec_outer_admitted &= (uint16_t)~bit;
        }
    }
    rec_outer_prev = phys;
}

static const char *const views[NVIEWS] = {"HOME","PART","GEN","FX","MIX","GLO","SONG","PERF","SEQ"};
static const int parts[] = {PART_303A, PART_303B, PART_909, PART_808, PART_BRK};
static const unsigned keys[] = {0,1,2,3,4,32,33,34,35,36};
static unsigned active_count(void) { unsigned n=0; for(int p=0;p<37;p++) n+=(unsigned)active[p]; return n; }
static int model_admits(int rec, int v) { return rec && (!BASELINE || v==V_SEQ || v==V_PART); }
static void reset(int part, int view, int rec, int playing, unsigned mod, int overlay)
{
    T(active_count()==0);
    memset(&ui,0,sizeof ui); ui.part=part; ui.view=view; ui.rec=rec;
    ui.btn=(mod&1u ? 1u<<B_HOME : 0u) | (mod&2u ? 1u<<B_SAVE : 0u);
    ui.overlay=overlay; seq.playing=playing; physical_keys=0;
    rec_outer_prev=rec_outer_suppress=rec_outer_admitted=rec_outer_armed=0;
    rec_outer_keys();
}
static void key(unsigned p,int down)
{
    if(down) physical_keys|=UINT64_C(1)<<p; else physical_keys&=~(UINT64_C(1)<<p);
    rec_outer_keys();
}
static void predicate_table(void)
{
    for(unsigned part=0;part<5;part++) for(int v=0;v<NVIEWS;v++)
    for(int rec=0;rec<2;rec++) for(unsigned fp=0;fp<4;fp++) {
        ui.part=parts[part]; ui.view=v; ui.rec=rec;
        ui.freeplay[0]=(uint8_t)(fp&1u); ui.freeplay[1]=(uint8_t)((fp>>1)&1u);
        int synth=part<2, slot=part==1;
        T(is_303()==synth); T(bidx()==slot);
        int production=synth && (v==V_PART || rec || (fp & (1u<<slot)));
        int expected=BASELINE ? (v==V_SEQ || v==V_PART) : production;
        int observed=!!kbd_on(); T(observed==expected);
        predicate_differences+=(unsigned)(observed!=production);
        printf("predicate,%s,%d,%d,-1,0,0,%u,-1,%d,%d\n",views[v],parts[part],rec,fp,observed,production);
    }
}
static void fresh_matrix(void)
{
    for(unsigned b=0;b<2;b++) for(int v=0;v<NVIEWS;v++) for(int rec=0;rec<2;rec++)
    for(int run=0;run<2;run++) for(unsigned mod=0;mod<4;mod++) for(int list=0;list<2;list++)
    for(unsigned k=0;k<10;k++) {
        unsigned p=keys[k]; int expected=model_admits(rec,v);
        reset(parts[b],v,rec,run,mod,list?O_LIST:O_NONE);
        unsigned before=ons, released=offs;
        key(p,1); int observed=(int)(ons-before);
        T(observed==expected); T(active[p]==expected);
        T(rec_outer_admitted==(expected?(uint16_t)(1u<<k):0));
        if(expected) { T(last_pitch[p]==36+(int)p); T(last_intent[p]==(v==V_SEQ)); }
        rec_outer_keys(); T(ons==before+(unsigned)expected);
        key(p,0); T(offs==released+(unsigned)expected); T(active_count()==0);
        admission_differences+=(unsigned)(observed!=rec);
        printf("fresh,%s,%d,%d,%d,%u,%d,0,%u,%d,%d\n",views[v],parts[b],rec,run,mod,list,p,observed,rec);
        fresh_cases++;
    }
}
static void held_view_transitions(void)
{
    for(unsigned b=0;b<2;b++) for(int run=0;run<2;run++) for(int v=0;v<NVIEWS;v++) {
        if(v==V_SEQ) continue;
        reset(parts[b],V_SEQ,1,run,0,O_NONE); key(0,1); T(active[0]);
        ui.view=v; rec_outer_keys(); int held=active[0], expected=model_admits(1,v);
        T(held==expected);
        unsigned before=ons; key(36,1); int fresh=(int)(ons-before); T(fresh==expected);
        key(36,0); T(active_count()==(unsigned)expected);
        ui.view=V_SEQ; rec_outer_keys(); T(active[0]==expected); /* baseline rearms with p0 suppressed */
        key(0,0); T(active_count()==0);
        key(0,1); T(active[0]); key(0,0); T(active_count()==0);
        transition_differences+=(unsigned)(held!=1 || fresh!=1);
        printf("transition,%s,%d,1,%d,0,0,0,0,%d,1\n",views[v],parts[b],run,held);
        transition_cases++;
    }
}
int main(void)
{
    puts("suite,view,part,rec,playing,modifier,overlay_list,freeplay_mask,key,observed,production_expected");
    predicate_table(); fresh_matrix(); held_view_transitions();
    T(ons==offs); T(active_count()==0);
    fprintf(stderr,"model=%s checks=%u structural_failures=%u predicate_differences=%u fresh_cases=%u admission_differences=%u transition_cases=%u transition_differences=%u on=%u off=%u seq_intent=%u audition_intent=%u\n",
        BASELINE?"substituted":"production",checks,failures,predicate_differences,fresh_cases,admission_differences,transition_cases,transition_differences,ons,offs,intent_seq,intent_audition);
    return failures || predicate_differences || admission_differences || transition_differences ? 1 : 0;
}
