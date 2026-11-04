/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole-app ownership regression; no calibration entry or padcal test case. */
static unsigned checks, failures;
static void block(void) { int32_t out[512];engine_render(out,256); }
static void scan(void) { ui_input_only();block();ui_input_only();block(); }
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
}
static void edge(unsigned p,unsigned velocity) { physical(p,velocity);scan(); }
#define VERIFY(c) do { checks++;if(!(c)){ failures++;fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c); } } while(0)
static void state(int voice,const char *phase)
{
    printf("  %s held_step=%d preview=%d token=%u gate=%u keys=%d original=%u bassheld=%u\n",
           phase,ui.held_step,ui.step_preview,engine_live_key_probe(voice,0),
           engine_live_key_probe(voice,2),engine_bass_keys_held(),
           engine_live_key_probe(voice,3),engine_live_key_probe(voice,5));
}
static void fresh(int bank,int voice)
{
    for(unsigned p=0;p<37;p++)physical(p,0);
    ui.freeplay[0]=ui.freeplay[1]=0;ui.rec=0;ui.overlay=O_NONE;ui.help=0;
    held_btn=ui.btn=0;ui.held_step=-1;seq_stop(&seq);scan();
    engine_bass_off(0);engine_bass_off(1);tick(200);
    VERIFY(!engine_bass_select_preset(voice,bank,0,&proj.sound));tick(200);
    VERIFY(engine_bass_bank(voice)==bank);
    ui.part=PART_303A+voice;set_view(V_SEQ);build_pages();ui.oct=0;scan();
}
static void live_is(int voice,unsigned key)
{
    VERIFY(engine_live_key_probe(voice,2)==1);
    VERIFY(engine_live_key_probe(voice,0)==live_token(key));
    VERIFY(engine_live_key_probe(voice,1)==36+key);
    VERIFY(engine_bass_keys_held()&(1<<voice));
    VERIFY(engine_live_key_probe(voice,3)>0);
}
static void released(int voice)
{
    VERIFY(!engine_live_key_probe(voice,2));
    VERIFY(!engine_live_key_probe(voice,0));
    VERIFY(!(engine_bass_keys_held()&(1<<voice)));
    VERIFY(!engine_live_key_probe(voice,3));
}
static void preview_owned(int voice)
{
    VERIFY(engine_bass_keys_held()&(1<<voice));
    VERIFY(engine_live_key_probe(voice,3)==1);
}
int main(int argc,char **argv)
{
    if(argc!=5) return 2;
    int bank=atoi(argv[1]),voice=atoi(argv[2]),key=atoi(argv[3]);
    const char *mode=argv[4];
    if((bank!=FACTORY_BANK_CHICAGO && bank!=FACTORY_BANK_MIAMI) || voice<0 || voice>1 || key<0 || key>36) return 2;
    boot();tick(300);fresh(bank,voice);
    unsigned held=(unsigned)WHITE_KEY[2]+5u;
    bstep_t *step=&proj.pat[seq.ppat[ui.part]].bass[voice].step[2];
    step->flags=G_NOTE;
    step->note=!strcmp(mode,"samepitch") ? 60+key : 48;
    printf("CASE bank=%d voice=%d physical=%d heldphysical=%u mode=%s\n",bank,voice,key,held,mode);
    edge(held,100);VERIFY(ui.held_step==2);VERIFY(ui.step_preview);preview_owned(voice);
    tick(600);scan();
    if(!strcmp(mode,"edits")) {
        int oldnote=step->note;
        enc_acc[EN_K1]=1;scan();VERIFY(step->note==oldnote+1);preview_owned(voice);
        unsigned oldvelocity=bstep_velocity(step);
        enc_acc[EN_ALGO]=-7;scan();VERIFY(bstep_velocity(step)==oldvelocity-7);preview_owned(voice);
        enc_acc[EN_K1]=-1;scan();VERIFY(step->note==oldnote);preview_owned(voice);
        enc_acc[EN_ALGO]=3;scan();VERIFY(bstep_velocity(step)==oldvelocity-4);preview_owned(voice);
        VERIFY(ui.held_step==2);VERIFY(ui.step_preview);
    }
    set_view(V_PART);build_pages();scan();VERIFY(ui.held_step==2);preview_owned(voice);
    if(!strcmp(mode,"preview")) {
        state(voice,"preview before step release");edge(held,0);state(voice,"preview after step release");
        VERIFY(ui.held_step==-1);VERIFY(!ui.step_preview);released(voice);
    } else {
        edge(key,100);live_is(voice,key);
        if(!strcmp(mode,"retrigger")) { edge(key,0);edge(key,110);live_is(voice,key); }
        if(!strcmp(mode,"early")) {
            edge(key,0);
            VERIFY(!engine_live_key_probe(voice,2));VERIFY(!engine_live_key_probe(voice,0));
            preview_owned(voice);
            /* The step preview may still sound until its own physical key-up. */
            edge(held,0);VERIFY(ui.held_step==-1);released(voice);
        } else {
            state(voice,"before step release");edge(held,0);state(voice,"after step release");
            VERIFY(ui.held_step==-1);VERIFY(!ui.step_preview);live_is(voice,key);
            edge(key,0);state(voice,"after physical release");released(voice);
        }
    }
    /* A complete release must admit the next strike and release cleanly. */
    state(voice,"before fresh physical strike");
    edge(key,100);live_is(voice,key);state(voice,"after fresh physical strike");
    edge(key,0);released(voice);state(voice,"after fresh physical release");
    VERIFY(step->flags==G_NOTE);VERIFY(!irq_depth);
    printf("%s checks=%u failures=%u\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
