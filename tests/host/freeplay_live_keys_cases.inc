/* SPDX-License-Identifier: GPL-3.0-only */
/* Free Play on the SMK-37: all 37 physical keys (27 central + 10 outer edge keys)
 * give a note-on on every synth engine, at every octave, also while a settings
 * list or HELP is open on the synth page. Driven through the production
 * smk__key_event -> smk_extq/plat_outer_keys -> ui live route -> engine. */
static unsigned checks;
#define T(c) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);exit(1);}}while(0)
#define TK(c,p,o,b) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s (key %u oct %d bank %d)\n",__LINE__,#c,(unsigned)(p),(int)(o),(int)(b));exit(1);}}while(0)
static void audio_block(void){int32_t out[512];engine_render(out,256);}
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
}
/* IRQ interleaving model: run real physical edges immediately after the first
 * outer transport critical section, not merely before the input scan. */
static unsigned test_outer_irq_armed,test_outer_irq_fired,test_outer_irq_key;
static void test_outer_irq_after(int publish)
{
    (void)publish;
    if(!test_outer_irq_armed)return;
    T(!irq_depth);test_outer_irq_armed=0;test_outer_irq_fired++;
    physical(test_outer_irq_key,0);physical(test_outer_irq_key,100);
}
static void scan(void){ui_input_only();audio_block();ui_input_only();audio_block();}
static void edge(unsigned p,unsigned velocity){physical(p,velocity);scan();}
static void fresh_keys(int bank)
{
    for(unsigned p=0;p<37;p++)physical(p,0);
    ui.freeplay[0]=ui.freeplay[1]=0;ui.rec=0;ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;held_btn=ui.btn=0;ui.held_step=-1;
    seq_stop(&seq);scan();engine_bass_off(0);engine_bass_off(1);tick(200);
    T(!engine_bass_select_preset(0,bank,0,&proj.sound));tick(200);
    T(engine_bass_bank(0)==bank);
    ui.part=PART_303A;set_view(V_PART);build_pages();ui.oct=0;
    audio_block();ui.freeplay[0]=1;scan();scan();T(live_owner==0);
    /* Only the admitted voice acknowledges its cancellation here. An inactive
     * voice can retain its wait bit after audio has already released its keys. */
    T(!(live_wait&1u));
}
static void set_oct(int o){ui.oct=(int8_t)o;scan();}
static void every_key(int bank,const char *ctx)
{
    for(int o=-2;o<=3;o++) {
        set_oct(o);
        for(unsigned p=0;p<37;p++) {
            unsigned pitch=(unsigned)(36+(int)p+12*o);
            edge(p,64+(p&31));
            TK(engine_live_key_probe(0,2)==1,p,o,bank);          /* gate on: the note sounds */
            TK(engine_live_key_probe(0,1)==pitch,p,o,bank);
            TK(engine_live_key_probe(0,0)==live_token(p),p,o,bank);
            edge(p,0);
            TK(!engine_live_key_probe(0,0),p,o,bank);
        }
    }
    set_oct(0);(void)ctx;
}
/* Owner report (0.0.13): the lowest/highest keys sometimes do not respond.
 * Fast taps, taps inside one scan, and taps beside held central keys. */
static void edge_keys_intermittent(int bank)
{
    static const unsigned outer[10]={0,1,2,3,4,32,33,34,35,36};
    for(int rep=0;rep<50;rep++) for(unsigned i=0;i<10;i++) {
        unsigned p=outer[i];
        edge(p,100);TK(engine_live_key_probe(0,2)==1 && engine_live_key_probe(0,1)==36+p,p,0,bank);
        edge(p,0);TK(!engine_live_key_probe(0,0),p,0,bank);
        /* held central key, then an edge key on top: the edge key owns the gate */
        edge(17,90);edge(p,100);TK(engine_live_key_probe(0,1)==36+p,p,0,bank);
        edge(p,0);TK(engine_live_key_probe(0,2)==1,p,0,bank);edge(17,0);TK(!engine_live_key_probe(0,2),p,0,bank);
        /* press right after a release of another edge key in the same scan */
        physical(outer[(i+1)%10],100);physical(outer[(i+1)%10],0);physical(p,100);scan();
        TK(engine_live_key_probe(0,1)==36+p,p,0,bank);edge(p,0);
    }
}
static void settings_do_not_swallow(int bank)
{
    open_list_of_pages();T(ui.overlay==O_LIST);scan();
    T(ui_live_key_owner()==1);every_key(bank,"list");
    ui.overlay=O_NONE;ui.help=1;scan();every_key(bank,"help");ui.help=0;
    set_view(V_SEQ);build_pages();scan();every_key(bank,"seq");
    set_view(V_PART);build_pages();scan();
    /* EDIT of a synth plays the keys with FREE PLAY off too (kbd_on): all 37, not only the central 27 */
    ui.freeplay[0]=0;scan();T(ui_live_key_owner()==1);every_key(bank,"edit-nofreeplay");
    open_list_of_pages();scan();every_key(bank,"edit-nofreeplay-list");ui.overlay=O_NONE;
    ui.freeplay[0]=1;scan();
}
/* B3: scan() above deliberately settles two input/render cycles. Check the
 * first cycle separately so a queued key cannot acquire an extra audio block
 * of delay while the all-keys coverage still passes. This observes the real
 * engine gate/identity, not a modeled DAC timestamp or envelope attack. */
static void first_render_edges(int bank)
{
    static const int views[2]={V_PART,V_SEQ};
    for(unsigned v=0;v<2;v++) {
        set_view(views[v]);build_pages();scan();
        T(ui_live_key_owner()==1);
        for(unsigned p=0;p<37;p++) {
            TK(!engine_live_key_probe(0,2),p,0,bank);
            physical(p,100);ui_input_only();audio_block();
            TK(engine_live_key_probe(0,2)==1,p,0,bank);
            TK(engine_live_key_probe(0,1)==36+p,p,0,bank);
            TK(engine_live_key_probe(0,0)==live_token(p),p,0,bank);
            TK(engine_live_key_probe(0,3)==1,p,0,bank);
            physical(p,0);ui_input_only();audio_block();
            TK(!(engine_bass_keys_held()&1),p,0,bank); /* no duplicate FIFO/bitmap membership */
            TK(!engine_live_key_probe(0,2),p,0,bank);
            TK(!engine_live_key_probe(0,0),p,0,bank);
        }
    }
}
/* Page/modal admission is not sounding-note ownership. Accepted physical keys
 * survive presentation changes; actual lifecycle revocation still releases them. */
static void owner_switches(int bank)
{
    for (int path=0;path<21;path++) {
        fresh_keys(bank);edge(17,100);T(engine_live_key_probe(0,2));
        uint32_t generation,context;int arp;
        engine_live_key_state(0,&generation,&arp,&context);
        switch(path) {
        case 0: enc_acc[EN_ALGO]=-1; break;
        case 1: ui.part=PART_909;build_pages();break; /* controller selection */
        case 2: set_view(V_HOME);break;
        case 3: set_view(V_GLO);break;
        case 4: set_view(V_MIX);break;
        case 5: set_view(V_FX);break;
        case 6: set_view(V_GEN);break;
        case 7: set_view(V_SONG);break;
        case 8: ui.overlay=O_ASK;break;
        case 9: ui.rec=1;break;
        case 10: smk_clip_enter_cleanup();test_clip_mode=1;break;
        case 11: run_action(ACT_PADCAL,0);test_padcal=1;break;
        case 12: test_smcui=1;break;
        case 13: held_btn=1u<<B_HOME;break;
        case 14: held_btn=1u<<B_SAVE;break;
        case 15: engine_live_key_force_serial(0,generation+1,context);break;
        case 16: engine_live_key_force_serial(0,generation+1,context+1);break;
        case 17: project_busy=1;break;
        case 18: perf_test_start();break;
        case 19: set_view(V_SEQ);ui.freeplay[0]=0;break;
        case 20: set_view(V_PERF);break;
        }
        scan();
        if(path==0) { T(ui.part==PART_303B);T(live_owner==1); }
        int presentation = path <= 9 || (path >= 12 && path <= 14) || path >= 19;
        if (presentation) {
            T(engine_live_key_probe(0,2));
            T(engine_live_key_probe(0,1)==53); /* original pitch, not current page/part */
            T(engine_bass_keys_held()&1);
            T(ui_project_begin()==UI_PROJECT_BUSY);
        } else {
            T(!engine_live_key_probe(0,2));
            T(!(engine_bass_keys_held()&1));
        }
        edge(17,0);T(!engine_live_key_probe(0,2));
        test_clip_mode=test_padcal=test_smcui=0;held_btn=0;
        project_busy=0;perf.test=-1;perf.phase=0;
    }
    fresh_keys(bank);edge(17,100);enc_acc[EN_ALGO]=-1;scan();
    T(engine_live_key_probe(0,2));
    edge(18,100);T(engine_live_key_probe(1,2));
    edge(17,0);T(!engine_live_key_probe(0,2));
    T(engine_live_key_probe(1,2)); /* A's real release cannot end B */
    edge(18,0);T(!engine_live_key_probe(1,2));
    fresh_keys(bank);edge(17,100);set_view(V_HOME);scan();
    int ch=seq.ch[TRK_BASS0];midi3(0x90|ch,65,100);audio_block();
    T(engine_bass_keys_held()&1);
    edge(17,0);T(engine_bass_keys_held()&1); /* foreign MIDI source stays held */
    midi3(0x80|ch,65,0);audio_block();T(!(engine_bass_keys_held()&1));
}
/* Cancellation acknowledgement belongs to each voice. Exercise the inactive
 * wait bit that a global !live_wait assertion incorrectly rejected, and prove
 * both the active voice and the cancelled voice accept fresh physical edges. */
static void cancelled_voice_reentry(int bank)
{
    fresh_keys(bank);enc_acc[EN_ALGO]=-1;scan();T(live_owner==1);
    edge(20,100);T(engine_live_key_probe(1,2));
    uint32_t before,after,context;int arp;
    engine_live_key_state(1,&before,&arp,&context);
    project_busy=1;scan(); /* actual project ownership, not a menu transition */
    project_busy=0;ui.part=PART_303A;build_pages();scan();T(live_owner==0);
    engine_live_key_state(1,&after,&arp,&context);
    T(after!=before);T(!engine_live_key_probe(1,2));
    T(!(engine_bass_keys_held()&2));T(live_wait&2u);
    /* Selecting A must not need a visit to B to acknowledge B's cancellation. */
    fresh_keys(bank);T(live_wait&2u);
    edge(17,100);T(engine_live_key_probe(0,2));edge(17,0);
    enc_acc[EN_ALGO]=-1;scan();T(live_owner==1);T(!(live_wait&2u));
    edge(20,100);T(engine_live_key_probe(1,2));
    edge(20,0);T(!engine_live_key_probe(1,2));
}
/* Regression: a real held central/outer key survives an SMC presentation and
 * releases its original token/pitch while that overlay still denies new notes. */
static void held_popup_release(int bank)
{
    static const unsigned physical_key[3]={0,17,36};
    for(unsigned i=0;i<3;i++) {
        unsigned p=physical_key[i];fresh_keys(bank);edge(p,100);
        T(engine_live_key_probe(0,2));T(engine_live_key_probe(0,1)==36+p);
        test_smcui=1;scan();
        T(engine_live_key_probe(0,2)); /* popup must not synthesize NoteOff */
        T(ui_project_begin()==UI_PROJECT_BUSY);
        ui.oct=2;scan();T(engine_live_key_probe(0,1)==36+p);
        edge(p,0);T(!engine_live_key_probe(0,2));
        test_smcui=0;scan();
    }
}
/* Release ownership must survive queue epoch changes and modal capture, not
 * merely the quiet-popup case. These are real physical transport edges. */
static void held_release_boundaries(int bank)
{
    static const unsigned key[]={0,17,36};
    for(unsigned i=0;i<sizeof key/sizeof key[0];i++) {
        unsigned p=key[i];fresh_keys(bank);edge(p,100);
        test_smcui=1; /* release/repress arrives before the popup's first scan */
        physical(p,0);physical(p,100);scan();
        T(!engine_live_key_probe(0,2)); /* old release survives epoch publication */
        physical(p,0);scan();test_smcui=0;scan();
        fresh_keys(bank);edge(p,100);test_smcui=1;scan();
        T(engine_live_key_probe(0,2));
        physical(p,0);physical(p,100);scan();
        T(!engine_live_key_probe(0,2)); /* epoch-0 NoteOff still releases its owner */
        physical(p,0);scan();test_smcui=0;scan();
        fresh_keys(bank);edge(p,100);test_smcui=1;scan();
        fm1_in.keys&=~((uint64_t)1u<<p); /* authoritative bitmap repairs a lost edge */
        scan();T(!engine_live_key_probe(0,2));test_smcui=0;scan();
    }
    fresh_keys(bank);edge(0,100);edge(17,100);
    enc_acc[EN_ALGO]=-1;scan();edge(36,100);test_smcui=1;scan();
    T(engine_live_key_probe(0,2));T(engine_live_key_probe(1,2));
    edge(0,0);T(engine_live_key_probe(0,2));T(engine_live_key_probe(1,2));
    edge(17,0);T(!engine_live_key_probe(0,2));T(engine_live_key_probe(1,2));
    edge(36,0);T(!engine_live_key_probe(1,2));test_smcui=0;scan();
    for(int order=0;order<2;order++) {
        fresh_keys(bank);edge(17,100);int ch=seq.ch[TRK_BASS0];
        midi3(0x90|ch,65,100);audio_block();test_smcui=1;scan();
        if(order) { edge(17,0);T(engine_bass_keys_held()&1);midi3(0x80|ch,65,0);audio_block(); }
        else { midi3(0x80|ch,65,0);audio_block();T(engine_live_key_probe(0,2));edge(17,0); }
        T(!(engine_bass_keys_held()&1));test_smcui=0;scan();
    }
    fresh_keys(bank);edge(17,100);test_smcui=1;scan();
    /* seq_stop on an already stopped transport is intentionally a no-op:
     * seq_advance invokes the engine all-off callback only while playing. */
    T(!seq.playing);seq_stop(&seq);audio_block();scan();T(engine_live_key_probe(0,2));
    seq_start(&seq);audio_block();T(seq.playing);T(engine_live_key_probe(0,2));
    seq_stop(&seq);audio_block();scan();T(!seq.playing);T(!(engine_bass_keys_held()&1));
    edge(17,0);test_smcui=0;scan();edge(17,100);T(engine_live_key_probe(0,2));edge(17,0);
    fresh_keys(bank);edge(17,100);test_smcui=1;scan();
    for(unsigned i=0;i<SMK_EXTQ+4;i++){physical(0,100);physical(0,0);}
    scan();T(!engine_live_key_probe(0,2));edge(17,0);test_smcui=0;scan();
    fresh_keys(bank);edge(17,100);
    pref_set(PR(R_FREEPLAY,0,0,0),0);scan(); /* explicit OFF action, not a lost page */
    T(!engine_live_key_probe(0,2));edge(17,0);
}
static void outer_handoff_irq(int bank)
{
    for(unsigned p=0;p<=36;p+=36) {
        fresh_keys(bank);edge(p,100);T(engine_live_key_probe(0,2));
        test_outer_irq_key=p;test_outer_irq_fired=0;test_outer_irq_armed=1;
        test_smcui=1;scan();
        T(test_outer_irq_fired==1);T(!test_outer_irq_armed);
        T(!engine_live_key_probe(0,2)); /* real release inside admission handoff */
        test_smcui=0;scan();T(!engine_live_key_probe(0,2));
        edge(p,0);edge(p,100);T(engine_live_key_probe(0,2));edge(p,0);
    }
}
/* Legacy pad aliases share central display bits, not physical FIFO identity.
 * Model only the existing platform alias bitmap, with rate-pad capture off. */
static void legacy_pad_alias(unsigned key,int down)
{
    uint32_t bit=1u<<key;
    if(down)fm1_in.pnotes|=bit;else fm1_in.pnotes&=~bit;
    fm1_in.notes=fm1_in.knotes|fm1_in.pnotes;held_keys=fm1_in.notes;
    scan();
}
static void central_fifo_qualification(int bank)
{
    fresh_keys(bank);edge(17,100);T(engine_live_key_probe(0,1)==53);
    set_oct(1);T(engine_live_key_probe(0,1)==53); /* held pitch never retunes */
    physical(17,0);physical(17,80);scan();
    T(engine_live_key_probe(0,2));T(engine_live_key_probe(0,1)==65);
    edge(17,0);T(!(engine_bass_keys_held()&1));
    for(int order=0;order<2;order++) {
        fresh_keys(bank);ui.freeplay[0]=0;scan();T(!division_pad_owner());
        legacy_pad_alias(12,1);T(engine_bass_keys_held()&1);
        edge(17,100);T(engine_live_key_probe(0,2));
        if(order) {
            edge(17,0);T(engine_bass_keys_held()&1);legacy_pad_alias(12,0);
        } else {
            legacy_pad_alias(12,0);T(engine_live_key_probe(0,2));edge(17,0);
        }
        T(!(engine_bass_keys_held()&1));T(!engine_live_key_probe(0,2));
        T(!kbd_arp_n[0]);
    }
    fresh_keys(bank);edge(17,100);test_smcui=1;scan();
    for(unsigned i=0;i<SMK_EXTQ+4;i++){physical(18,100);physical(18,0);}
    scan();T(!engine_live_key_probe(0,2));T(!(engine_bass_keys_held()&1));
    edge(17,0);test_smcui=0;scan();
}
/* A legacy pad release must not wait for a different physical key to end. */
static void mixed_alias_release(int bank)
{
    for(int order=0;order<2;order++) {
        fresh_keys(bank);ui.freeplay[0]=0;scan();T(!division_pad_owner());
        legacy_pad_alias(12,1);T(engine_bass_keys_held()&1);
        edge(17,100);edge(18,100);T(engine_live_key_probe(0,2));
        if(order) { legacy_pad_alias(12,0);edge(17,0); }
        else { edge(17,0);legacy_pad_alias(12,0); }
        T(engine_live_key_probe(0,2));T(engine_live_key_probe(0,1)==54);
        T(!kbd_arp_n[0]); /* matching pad owner retired despite another live key */
        T(engine_live_key_probe(0,3)==1);
        edge(18,0);T(!(engine_bass_keys_held()&1));
    }
}
/* FIFO onset admission sees this scan's gestures, not yesterday's ui.btn. */
static void simultaneous_gesture_admission(int bank)
{
    static const int modifier[]={B_HOME,B_SAVE};
    for(unsigned i=0;i<sizeof modifier/sizeof modifier[0];i++) {
        fresh_keys(bank);held_btn=1u<<modifier[i];physical(17,100);scan();
        T(ui.btn&(1u<<modifier[i]));T(ui.btn_used&(1u<<modifier[i]));
        T(!engine_live_key_probe(0,2)); /* pattern/paste press must not also start a synth */
        T(!(engine_bass_keys_held()&1));
        edge(17,0);held_btn=0;scan();
        /* The same modifier denies new ons but keeps an earlier accepted key. */
        fresh_keys(bank);edge(18,100);held_btn=1u<<modifier[i];scan();
        T(engine_live_key_probe(0,2));T(engine_live_key_probe(0,1)==54);
        edge(18,0);T(!(engine_bass_keys_held()&1));held_btn=0;scan();
    }
    fresh_keys(bank);held_btn=1u<<B_REC;physical(17,100);scan();
    T(ui.rec);T(!engine_live_key_probe(0,2)); /* legacy REC, not duplicate live ownership */
    T(engine_bass_keys_held()&1);edge(17,0);T(!(engine_bass_keys_held()&1));
    held_btn=0;scan();ui.rec=0;scan();
}
int main(int argc,char **argv)
{
    (void)argc;(void)argv;boot();tick(300);
    const char *review=getenv("X0X_LIVE_REVIEW_CASE");
    if(review) {
        for(int bank=FACTORY_BANK_CHICAGO;bank<=FACTORY_BANK_MIAMI;bank++) {
            if(!strcmp(review,"outer"))outer_handoff_irq(bank);
            else if(!strcmp(review,"central"))held_release_boundaries(bank);
            else if(!strcmp(review,"alias"))mixed_alias_release(bank);
            else if(!strcmp(review,"gesture"))simultaneous_gesture_admission(bank);
            else T(!"unknown review case");
        }
        printf("PASS %u review release checks (%s)\n",checks,review);return 0;
    }
    for(int bank=FACTORY_BANK_CHICAGO;bank<=FACTORY_BANK_MIAMI;bank++) {
        held_popup_release(bank);
        held_release_boundaries(bank);
        outer_handoff_irq(bank);
        central_fifo_qualification(bank);
        mixed_alias_release(bank);
        simultaneous_gesture_admission(bank);
        fresh_keys(bank);every_key(bank,"edit");
        edge_keys_intermittent(bank);
        settings_do_not_swallow(bank);
        first_render_edges(bank);
        owner_switches(bank);
        cancelled_voice_reentry(bank);
    }
    T(!irq_depth);
    printf("PASS %u checks: 37 keys x 6 octaves x {Chicago, Miami} x {EDIT, SEQ, list, HELP}, edge keys x50; on/off in first render\n",checks);
    return 0;
}
