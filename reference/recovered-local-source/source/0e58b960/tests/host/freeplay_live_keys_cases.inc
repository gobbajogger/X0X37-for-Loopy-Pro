/* SPDX-License-Identifier: GPL-3.0-only */
extern unsigned engine_live_key_probe(int voice,int field);
static unsigned checks,accepted;static int last_pitch,last_velocity,last_slide;
void engine_live_key_accepted(int voice,int token,int pitch,int velocity,int slide)
{(void)voice;(void)token;accepted++;last_pitch=pitch;last_velocity=velocity;last_slide=slide;}
extern void engine_live_key_motion_step(int voice,int step);
extern void engine_live_key_pattern_note(int voice,int note);
#define T(c) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);exit(1);}}while(0)
static void audio_block(void){int32_t out[512];engine_render(out,256);}
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
}
static void scan(void){ui_input_only();audio_block();ui_input_only();}
static void edge(unsigned p,unsigned velocity){physical(p,velocity);scan();}
static void fresh_keys(int bank,int arp)
{
    for(unsigned p=0;p<37;p++)physical(p,0);
    ui.freeplay[0]=ui.freeplay[1]=0;ui.rec=0;ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;held_btn=ui.btn=0;ui.held_step=-1;
    seq_stop(&seq);scan();engine_bass_off(0);engine_bass_off(1);tick(1000);
    memset(proj.arr.lane,0,sizeof proj.arr.lane);engine_motion_reset();
    T(!engine_bass_select_preset(0,bank,0,&proj.sound));tick(1000);
    T(engine_bass_bank(0)==bank);
    ui.part=PART_303A;set_view(V_PART);build_pages();ui.oct=0;
    if(bank==FACTORY_BANK_CHICAGO)pref_set(PR(R_ENG,T_303,0,CHI_ARP_SWITCH),arp);
    audio_block();ui.freeplay[0]=1;scan();scan();T(live_owner==0);T(!live_wait);
}
static void all_keys_and_fallback(void)
{
    for(int bank=0;bank<2;bank++) {
        fresh_keys(bank,0);
        unsigned char patterns[sizeof proj.pat];memcpy(patterns,proj.pat,sizeof patterns);
        for(unsigned p=0;p<37;p++) {
            edge(p,31+p);T(engine_live_key_probe(0,1)==36+p);T(engine_live_key_probe(0,4)==31+p);
            T(engine_live_key_probe(0,0)==live_token(p));edge(p,0);T(!engine_live_key_probe(0,0));
        }
        edge(0,41);edge(12,99);edge(12,0);T(engine_live_key_probe(0,1)==36);T(engine_live_key_probe(0,4)==41);
        edge(36,71);edge(0,0);T(engine_live_key_probe(0,1)==72);edge(36,0);T(!engine_live_key_probe(0,0));
        for(unsigned p=0;p<37;p++)edge(p,40+p);
        T(live_keys[0].n==37);
        for(int p=36;p>=0;p--){edge((unsigned)p,0);if(p)T(engine_live_key_probe(0,1)==35+(unsigned)p);}
        T(!memcmp(patterns,proj.pat,sizeof patterns));T(!live_keys[0].n);
        /* Complete outer tap under stable publication survives the queue. */
        physical(1,93);physical(1,0);scan();T(!engine_live_key_probe(0,0));
    }
}
static void arp_membership(void)
{
    fresh_keys(FACTORY_BANK_CHICAGO,1);
    for(unsigned p=0;p<8;p++)edge(p,100);
    T(live_keys[0].n==8);T(engine_live_key_probe(0,3)==8);
    edge(36,100);T(live_keys[0].n==8);edge(0,0);edge(36,0);T(engine_live_key_probe(0,3)==7);
    for(unsigned p=1;p<8;p++)edge(p,0);T(!engine_live_key_probe(0,3));
    ui.oct=1;scan();edge(0,50);ui.oct=0;scan();edge(12,80);
    T(live_keys[0].n==2);T(engine_live_key_probe(0,3)==1);
    edge(0,0);T(engine_live_key_probe(0,3)==1);edge(12,0);T(!engine_live_key_probe(0,3));
    edge(12,80);ui.oct=1;scan();edge(0,50);edge(12,0);T(engine_live_key_probe(0,3)==1);edge(0,0);T(!engine_live_key_probe(0,3));
    fresh_keys(FACTORY_BANK_MIAMI,0);proj.sound.v[T_303][0][CHI_ARP_SWITCH]=1;
    scan();T(!live_arp);edge(0,100);edge(1,100);edge(1,0);T(engine_live_key_probe(0,1)==36);edge(0,0);
}
static void epochs_and_cancel(void)
{
    fresh_keys(FACTORY_BANK_CHICAGO,0);edge(0,100);
    uint32_t old;int arp;engine_live_key_state(0,&old,&arp,0);
    engine_set(T_303,0,CHI_ARP_SWITCH,1);audio_block();
    T(engine_live_key(0,old,28,36,0,0,0));scan();T(!live_keys[0].n);T(!engine_live_key_probe(0,0));
    edge(0,0);edge(2,80);T(engine_live_key_probe(0,3)==1);
    engine_set(T_303,0,CHI_ARP_SWITCH,0);audio_block();scan();edge(3,77);edge(2,0);
    T(engine_live_key_probe(0,1)==39);edge(3,0);
    /* Pending key cannot overwrite a later accepted MIDI or legacy CQ gate. */
    engine_live_key_state(0,&old,&arp,0);T(engine_live_key(0,old,28,36,100,0,0));
    engine_bass_on_velocity(0,67,0,0,100);audio_block();T(!engine_live_key_probe(0,0));T(engine_live_key_probe(0,5)==67);
    engine_bass_off(0);audio_block();scan();
    engine_live_key_state(0,&old,&arp,0);T(engine_live_key(0,old,28,36,100,0,0));
    midi3(0x90|seq.ch[TRK_BASS0],69,100);audio_block();T(!engine_live_key_probe(0,0));T(engine_live_key_probe(0,5)==69);
    midi3(0x80|seq.ch[TRK_BASS0],69,0);audio_block();scan();
    fresh_keys(FACTORY_BANK_MIAMI,0);edge(0,50);physical(36,100);ui.overlay=O_LIST;scan();
    T(!live_keys[0].n);ui.overlay=O_NONE;scan();T(!live_keys[0].n);edge(36,0);edge(0,0);edge(36,100);T(engine_live_key_probe(0,1)==72);edge(36,0);
    /* Overflow drops partial gestures and suppresses held entry. */
    for(unsigned i=0;i<40;i++)physical(0,(i&1)?0:100);physical(1,100);scan();T(!live_keys[0].n);
    scan();T(!live_keys[0].n);edge(1,0);edge(1,90);T(engine_live_key_probe(0,1)==37);edge(1,0);
    engine_live_key_state(0,&old,&arp,0);int rejected=0;
    for(unsigned i=0;i<40;i++)rejected+=!engine_live_key(0,old,28,36,(i&1)?0:100,0,0);
    T(rejected>0);audio_block();T(!engine_live_key_probe(0,0));scan();T(!live_keys[0].n);
}
static void damper_and_render(const char *dir)
{
    fresh_keys(FACTORY_BANK_CHICAGO,0);pref_t r=PR(R_ENG,T_303,0,CHI_DAMPER);
    const int vals[]={0,63,64,127};char text[16],unit[8];
    for(unsigned i=0;i<4;i++) {
        pref_set(r,vals[i]);pref_value_of(r,vals[i],text,unit);T(!strcmp(text,vals[i]<64?"OFF":"ON"));
        turn_ref(r,1,EN_K1);T(pref_get(r)==127);turn_ref(r,-1,EN_K1);T(pref_get(r)==0);
    }
    for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_ENG && pg.r[p][k].c==CHI_DAMPER) {
        *page_of(V_PART)=p;ui.touched=-1;
        for(int on=0;on<2;on++) {
            pref_set(r,on?127:0);ui.msg_until=0;draw_header();draw_main();draw_knobs();
            char path[512];snprintf(path,sizeof path,"%s/chicago-damper-%s.png",dir,on?"on":"off");png_write(path);
        }
    }
    fresh_keys(FACTORY_BANK_MIAMI,0);r=PR(R_ENG,T_303,0,W_DAMPER);
    turn_ref(r,1,EN_K1);T(pref_get(r)==1);turn_ref(r,-1,EN_K1);T(pref_get(r)==0);
}

static void motion_and_pattern_owners(void)
{
    for(int damper=0;damper<2;damper++) {
        fresh_keys(FACTORY_BANK_CHICAGO,0);
        pref_set(PR(R_ENG,T_303,0,CHI_DAMPER),damper?127:0);audio_block();
        edge(0,70);edge(12,90);
        uint32_t old;int mode;engine_live_key_state(0,&old,&mode,0);
        int lane=motion_alloc(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,CHI_ARP_SWITCH);T(lane>=0);
        proj.arr.lane[lane].val[0]=1;proj.arr.lane[lane].val[1]=0;
        engine_live_key_motion_step(0,0);T(engine_motion_value(lane)==1);T(engine_live_key_probe(0,8)==1);
        T(proj.sound.v[T_303][0][CHI_ARP_SWITCH]==0); /* effective, not saved */
        T(engine_live_key(0,old,8,48,0,0,0));scan();T(!live_keys[0].n);
        edge(0,0);edge(12,0);edge(2,75);T(engine_live_key_probe(0,3)==1);
        engine_live_key_state(0,&old,&mode,0);engine_live_key_motion_step(0,1);
        T(engine_motion_value(lane)==0);T(engine_live_key_probe(0,8)==0);scan();edge(3,85);
        edge(2,0);T(engine_live_key_probe(0,1)==39);
        engine_live_key_pattern_note(0,65);T(!engine_live_key_probe(0,0));T(engine_live_key_probe(0,7));
        T(engine_live_key(0,old,30,38,0,0,0));audio_block();T(engine_live_key_probe(0,7));
        edge(3,0);T(engine_live_key_probe(0,7));engine_live_key_pattern_note(0,-1);
        engine_motion_clear(lane);audio_block();
    }
    /* Real sequencer motion playback also reaches the same effective hook. */
    fresh_keys(FACTORY_BANK_CHICAGO,0);
    int lane=motion_alloc(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,CHI_ARP_SWITCH);T(lane>=0);
    for(int i=0;i<NSTEPS;i++)proj.arr.lane[lane].val[i]=1;
    engine_motion_enable(1);seq_start(&seq);tick(40);
    T(engine_motion_value(lane)==1);T(engine_live_key_probe(0,8)==1);
    seq_stop(&seq);tick(10);engine_motion_clear(lane);audio_block();
}
static void cutoff_and_services(void)
{
    fresh_keys(FACTORY_BANK_MIAMI,0);
    unsigned before=accepted;physical(0,51);physical(0,0);held_btn=1u<<B_OCTUP;scan();
    T(accepted==before+1);T(last_pitch==36);held_btn=0;scan();
    /* Simultaneous central release cannot create fallback to an absent key. */
    edge(20,60);edge(6,90);before=accepted;physical(20,0);physical(6,0);scan();T(accepted==before);
    /* Queue press before external octave change is cut off, not relabelled. */
    before=accepted;physical(36,72);ui.oct=0;scan();T(accepted==before);edge(36,0);
    for(int m=0;m<9;m++) {
        fresh_keys(FACTORY_BANK_MIAMI,0);edge(0,50);physical(36,90);
        if(m==0)ui.freeplay[0]=0; if(m==1)ui.rec=1; if(m==2)ui.part=PART_303B;
        if(m==3)test_clip_mode=1; if(m==4)ui.overlay=O_LIST; if(m==5)test_padcal=1;
        if(m==6)test_smcui=1; if(m==7){held_btn=ui.btn=1u<<B_HOME;} if(m==8)project_busy=1;
        scan();T(!live_keys[0].n);T(!engine_live_key_probe(0,0));T(!irq_depth);
        project_busy=0;ui.freeplay[0]=0;ui.rec=0;ui.part=PART_303A;test_clip_mode=test_padcal=test_smcui=0;
        ui.overlay=O_NONE;held_btn=ui.btn=0;scan();edge(0,0);edge(36,0);
    }
    fresh_keys(FACTORY_BANK_CHICAGO,0);
    pref_t r=PR(R_ENG,T_303,0,CHI_DAMPER);pref_set(r,127);audio_block();
    edge(0,100);edge(0,0);T(engine_live_key_probe(0,7));pref_set(r,0);audio_block();T(!engine_live_key_probe(0,7));
    /* Direct lock editing shares raw binary endpoint semantics. */
    ui.held_step=0;ui.held_step_part=ui.part;ui.held_step_pat=seq.ppat[ui.part];
    lock_turn(r,1,EN_K1);int lane=motion_find(proj.arr.lane,seq.ppat[ui.part],ui.part,T_303,0,CHI_DAMPER);
    T(lane>=0);T(proj.arr.lane[lane].val[0]==127);lock_turn(r,-1,EN_K1);T(proj.arr.lane[lane].val[0]==0);ui.held_step=-1;
}


static void retained_inventory_new_intent(void)
{
    for(int bank=0;bank<2;bank++) {
        fresh_keys(bank,0);edge(0,41);unsigned before=accepted;
        engine_live_key_pattern_note(0,65);scan();
        T(live_keys[0].n==1);T(kbd_held_key[0]==28);T(accepted==before);T(!engine_live_key_probe(0,0));
        edge(0,0);T(engine_live_key_probe(0,7));T(accepted==before);
        edge(0,41);engine_live_key_pattern_note(0,65);scan();
        edge(12,99);T(last_pitch==48 && last_slide==0); /* fresh acquisition, not old-envelope continuation */
        edge(12,0);T(last_pitch==36 && last_velocity==41 && last_slide==1);T(engine_live_key_probe(0,1)==36);
        edge(0,0);T(!engine_live_key_probe(0,0));
    }
    fresh_keys(FACTORY_BANK_CHICAGO,1);ui.oct=1;scan();edge(0,42);
    engine_live_key_pattern_note(0,66);scan();T(live_keys[0].n==1);
    ui.oct=0;scan();edge(12,96);T(engine_live_key_probe(0,3)==1);T(last_pitch==48 && last_velocity==96);
    edge(0,0);T(engine_live_key_probe(0,3)==1);edge(12,0);T(!engine_live_key_probe(0,3));
    /* Coalesced destructive changes must not masquerade as an ordinary gate change. */
    fresh_keys(FACTORY_BANK_CHICAGO,0);edge(0,60);
    engine_set(T_303,0,CHI_ARP_SWITCH,1);engine_set(T_303,0,CHI_ARP_SWITCH,0);audio_block();scan();
    T(!live_keys[0].n);T(live_keys[0].suppressed&1u);edge(0,0);
}

static void midi_link_end_precedence(void)
{
    for(int bank=0;bank<2;bank++) {
        fresh_keys(bank,0);midi3(0x90|seq.ch[TRK_BASS0],69,100);audio_block();scan();
        uint32_t g;int arp;engine_live_key_state(0,&g,&arp,0);
        T(engine_live_key(0,g,28,36,99,0,0));
        min_q[mi_w++ % MQ]=0; /* USB input-ended record, not a NoteOff */
        unsigned count=accepted;audio_block();T(!engine_live_key_probe(0,0));T(accepted==count);
        scan();T(!live_keys[0].n);edge(0,75);T(engine_live_key_probe(0,0)==28);edge(0,0);
    }
}

static void break_press_owner_release(void)
{
    for(int bank=0;bank<2;bank++) {
        fresh_keys(bank,0);ui.freeplay[0]=0;ui.part=PART_BRK;set_view(V_SEQ);scan();
        int k=0;while(k<NKEYS && KEY_BLACK[k]!=0)k++;T(k<NKEYS);
        edge((unsigned)k+5,75);T(ui.brk_localkeys&1u);T(engine_brk_held()!=0);
        ui.part=PART_303A;set_view(V_PART);ui.freeplay[0]=1;scan();
        T(!live_keys[0].n);T(ui.brk_localkeys&1u);
        edge((unsigned)k+5,0);T(!ui.brk_localkeys);T(!ui.brk_pinkeys);T(!engine_brk_held());
        T(!engine_live_key_probe(0,0));
    }
}

/* Serial exhaustion is forced, not tested by billions of simulated events. */
extern void engine_live_key_force_serial(int,uint32_t,uint32_t);
static void serial_exhaustion(void)
{
    fresh_keys(FACTORY_BANK_CHICAGO,0);edge(0,61);
    uint32_t g,c;int arp;engine_live_key_state(1,&g,&arp,&c);
    T(engine_live_key(1,g,1,60,80,0,0));audio_block();T(engine_live_key_probe(1,0)==1);
    T(engine_live_key(1,g,1,60,0,0,0)); /* Must survive lane0 exhaustion. */
    engine_live_key_force_serial(0,UINT32_MAX,50);
    T(engine_live_key(0,UINT32_MAX,28,36,0,0,0));
    engine_live_key_pattern_note(0,66);audio_block();
    T(!engine_live_key_probe(1,0));T(engine_live_key_probe(0,7));
    engine_live_key_state(0,&g,&arp,&c);T(g==UINT32_MAX && arp==-1);
    T(!engine_live_key(0,UINT32_MAX,28,36,90,0,0));
    scan();T(live_owner==-1);T(!live_keys[0].n);T(engine_live_key_probe(0,7));
    /* Independent context exhaustion also refuses stale capabilities. */
    engine_live_key_state(1,&g,&arp,&c);engine_live_key_force_serial(1,g,UINT32_MAX);
    T(engine_live_key(1,g,1,60,90,0,0));audio_block();T(engine_live_key_probe(1,0)==1);
    engine_live_key_cancel(1);audio_block();
    engine_live_key_state(1,&g,&arp,&c);T(c==UINT32_MAX && arp==-1);
    T(!engine_live_key_probe(1,0));T(!engine_live_key(1,g,1,60,90,0,0));
    engine_live_key_cancel(0);audio_block();T(engine_live_key_probe(0,7)); /* No foreign off. */
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    for(int v=0;v<NBASS;v++){engine_live_key_state(v,&g,&arp,&c);T(arp==-1);T(!engine_live_key(v,g,1,60,90,0,0));}
}

int main(int argc,char **argv)
{
    boot();all_keys_and_fallback();arp_membership();epochs_and_cancel();motion_and_pattern_owners();cutoff_and_services();retained_inventory_new_intent();damper_and_render(argc>1?argv[1]:".");midi_link_end_precedence();break_press_owner_release();serial_exhaustion();
    unsigned ui_bytes=sizeof live_keys+sizeof live_outer+sizeof live_epoch+sizeof live_generation+sizeof live_context+sizeof live_intent_generation+sizeof live_intent+sizeof live_wait_generation+sizeof live_owner+sizeof live_octave+sizeof live_arp+sizeof live_wait;
    unsigned hal_bytes=sizeof smk_extq-64+sizeof smk_ext_epoch+sizeof smk_ext_oct+sizeof smk_ext_enabled+sizeof smk_extq_overflow;
    unsigned engine_bytes=engine_live_key_probe(0,6);
    T(ui_bytes+hal_bytes+engine_bytes<=1536);T(!irq_depth);T(!fails);
    printf("PASS %u checks; fixed incremental state UI=%u HAL=%u engine=%u total=%u bytes (host sizeof; native proof pending)\n",checks,ui_bytes,hal_bytes,engine_bytes,ui_bytes+hal_bytes+engine_bytes);
    return 0;
}
