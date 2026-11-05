/* SPDX-License-Identifier: GPL-3.0-only */
/* Free Play on the SMK-37: all 37 physical keys (27 central + 10 outer edge keys)
 * give a note-on on every synth engine, at every octave, also while a settings
 * list or HELP is open on the synth page. Driven through the production
 * smk__key_event -> smk_extq/plat_outer_keys -> ui live route -> engine. */
extern unsigned engine_live_key_probe(int voice,int field);
static unsigned checks;
#define T(c) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);exit(1);}}while(0)
#define TK(c,p,o,b) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s (key %u oct %d bank %d)\n",__LINE__,#c,(unsigned)(p),(int)(o),(int)(b));exit(1);}}while(0)
static void audio_block(void){int32_t out[512];engine_render(out,256);}
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
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
    audio_block();ui.freeplay[0]=1;scan();scan();T(live_owner==0);T(!live_wait);
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
int main(int argc,char **argv)
{
    (void)argc;(void)argv;boot();tick(300);
    for(int bank=FACTORY_BANK_CHICAGO;bank<=FACTORY_BANK_MIAMI;bank++) {
        fresh_keys(bank);every_key(bank,"edit");
        edge_keys_intermittent(bank);
        settings_do_not_swallow(bank);
    }
    T(!irq_depth);
    printf("PASS %u checks: 37 keys x 6 octaves x {Chicago, Miami} x {EDIT, SEQ, list, HELP}, edge keys x50\n",checks);
    return 0;
}
