/* SPDX-License-Identifier: GPL-3.0-only */
/* Production original Chicago/Miami UI, physical controls and framebuffer.
 * The pre-original matrix fixture's control/navigation/transition checks are
 * retained against the shipping macro map; retired bass303 DSP is not linked. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void near(float a,float b) { CHECK(fabsf(a-b)<0.0001f); }
static void audio(void) { int32_t out[512]; engine_render(out,256); }
static int sound_page(void) { for(int p=0;p<pg.n;p++) if(pg.r[p][0].kind==R_BENGINE)return p; return -1; }
static void fixture(int voice,int bank)
{
    engine_bass_wheel_owners(0);held_keys=held_btn=0;ui.btn=ui.keys=ui.btn_used=0;
    ui.help=ui.rec=0;ui.held_step=-1;seq.playing=0;ui.overlay=O_NONE;
    engine_bass_off(0);engine_bass_off(1);
    for(int i=0;i<600;i++)audio();
    memset(proj.arr.lane,0,sizeof proj.arr.lane);
    CHECK(engine_bass_select_preset(voice,bank,0,&proj.sound)==0);
    for(int i=0;i<600 && engine_bass_preset_pending(voice);i++)audio();
    CHECK(!engine_bass_preset_pending(voice));CHECK(engine_bass_bank(voice)==bank);
    ui.part=PART_303A+voice;set_view(V_PART);*page_of(V_PART)=0;
    ui.touched=-1;ui.msg_until=0;ui_input_only();build_pages();
}
static void knob(int k,int delta)
{ now_ms+=1000;enc_acc[EN_K1+k]=delta;ui_input_only(); }
static void select_row(int delta)
{ now_ms+=1000;enc_acc[EN_SELECT]=delta;ui_input_only(); }
static void patch(void)
{ held_btn=1u<<B_HOME;now_ms+=10;ui_input_only();held_btn=0;now_ms+=10;ui_input_only(); }
static void render(void) { ui.msg_until=0;draw_header();draw_main();draw_knobs(); }
/* Independent expected macro IDs, not an alias of the production row table. */
static const int expected[2][7][4]={
    {{0,1,-1,-1},{2,3,4,-1},{5,6,-1,-1},{7,8,9,10},
     {11,12,-1,-1},{13,15,14,16},{17,18,19,-1}},
    {{0,1,2,3},{6,5,7,8},{9,10,12,-1},{11,13,-1,-1},{4,15,14,-1}}
};
static unsigned states(int bank,int id)
{
    if(!bank) { if(id==11 || id==13)return 2;if(id==14 || id==15)return 3;if(id==16)return 10; }
    else { if(id==0)return 3;if(id==3)return 25;if(id==6 || id==11)return 4;if(id==13)return 8; }
    return 0;
}
static void mapping(int voice,int bank)
{
    unsigned seen[NPARAMS_MAX]={0};int rows=bank?5:7;
    fixture(voice,bank);CHECK(pg.n==rows+1);CHECK(sound_page()==rows);
    for(int i=0;i<factory_bank(bank)->npresets;i++) {
        preset_text_t text;preset_text(&text,factory_preset_name(bank,i),!bank);
        CHECK(text.lines>=1 && text.lines<=2);
        for(int line=0;line<text.lines;line++)CHECK(tw(text.font,text.line[line])<=54);
    }
    for(int p=0;p<rows;p++)for(int k=0;k<4;k++) {
        pref_t r=pg.r[p][k];int id=expected[bank][p][k];
        CHECK(r.kind==(id<0?R_NONE:R_ENG));
        if(id>=0)CHECK(r.a==T_303 && r.b==voice && r.c==id);
    }
    CHECK(pg.r[rows][1].kind==R_BPRESET && pg.r[rows][2].kind==R_BSTATUS && pg.r[rows][3].kind==R_NONE);
    for(int p=0;p<pg.n;p++) {
        *page_of(V_PART)=p;
        for(int k=0;k<4;k++) {
            pref_t r=pg.r[p][k];const x0x_param_t *d=pref_desc(r);
            if(r.kind==R_NONE || r.kind==R_BSTATUS) {
                sound_t before=proj.sound;int pending=engine_bass_preset_pending(voice);
                knob(k,1);knob(k,-1);
                CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(engine_bass_preset_pending(voice)==pending);continue;
            }
            CHECK(d);CHECK(tw(&FONT_T,matrix_name(r))<=54);
            if(r.kind==R_ENG) {
                seen[r.c]++;unsigned n=states(bank,r.c);
                /* Begin at a selector boundary to check real detents as well
                 * as continuous one-byte edits. No mutable expectation helper. */
                pref_set(r,0);sound_t before=proj.sound;knob(k,1);
                int want=n?(127+(int)(n-1)/2)/(int)(n-1):1;
                CHECK(pref_get(r)==want);
                uint8_t cell[ORIGINAL_SOUND_BYTES];memcpy(cell,before.v[T_303][voice],sizeof cell);
                uint32_t mask=(uint32_t)cell[22]|(uint32_t)cell[23]<<8|
                    (uint32_t)cell[24]<<16|(uint32_t)cell[25]<<24;
                mask|=1u<<r.c;cell[r.c]=(uint8_t)want;
                for(int byte=0;byte<4;byte++)cell[22+byte]=(uint8_t)(mask>>(byte*8));
                memcpy(before.v[T_303][voice],cell,sizeof cell);
                CHECK(!memcmp(&before,&proj.sound,sizeof before));
                CHECK(original_sound_edit_mask(proj.sound.v[T_303][voice])&(1u<<r.c));
                knob(k,-1);CHECK(pref_get(r)==0);
                CHECK(engine_bass_bank(voice)==bank && !engine_bass_preset_pending(voice));
            }
            for(int v=0;v<=d->max;v++) {
                char n[16],u[8],s[24];pref_value_of(r,v,n,u);put_s(put_s(s,n),u);
                CHECK(tw(&FONT_T,s)<=54);
            }
        }
    }
    CHECK(pg.nmore==0);
    /* Every patch macro occurs once; both performance inputs stay off pages. */
    CHECK(engine_nparams(T_303,voice)==(bank?18:22));
    for(int i=0;i<engine_nparams(T_303,voice);i++)CHECK(seen[i]==(unsigned)(i<(bank?16:20)));
    sound_t before=proj.sound;
    *page_of(V_PART)=0;select_row(-1);CHECK(cur_page()==pg.n-1);select_row(1);CHECK(cur_page()==0);
    for(int p=1;p<pg.n;p++){select_row(1);CHECK(cur_page()==p);CHECK(cur_page()/4==p/4);}
    select_row(1);CHECK(cur_page()==0);*page_of(V_PART)=pg.n-1;button_tap(B_EDIT);CHECK(cur_page()==0);
    button_tap(B_EDIT);CHECK(cur_page()==1);select_row(-1);CHECK(cur_page()==0);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));
    *page_of(V_PART)=sound_page();engine_bass_wheel_owners(1u<<voice);
    int n=engine_bass_requested_preset(voice);now_ms+=1000;enc_acc[EN_PRESET]=1;ui_input_only();
    CHECK(engine_bass_requested_preset(voice)==n+1);CHECK(engine_bass_preset_pending(voice));
    knob(2,1);knob(3,-1);CHECK(engine_bass_preset_pending(voice));
    patch();CHECK(!engine_bass_preset_pending(voice));engine_bass_wheel_owners(0);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));
    printf("PASS %s %c: all controls, blanks, persisted edits, widths and navigation\n",bank?"Miami":"Chicago",'A'+voice);
}
static void geometry(void)
{
    for(int bank=0;bank<2;bank++) {
        fixture(0,bank);sound_t before=proj.sound;
        for(int page=0;page<pg.n;page++) {
            *page_of(V_PART)=page;render();
            /* The current slab geometry and page dots use the original palette. */
            for(int r=0;r<4 && page/4*4+r<pg.n;r++) {
                int y=24+r*50+24;
                CHECK(fb[y*240+7]==swap16(r==page%4?SOUND_SELECT:SOUND_FACE));
            }
            for(int state=SAVE_SAVED;state<=SAVE_FAILED;state++) {
                ui.dirty=state==SAVE_UNSAVED;save_wait=state==SAVE_QUEUED;ui.save_failed=state==SAVE_FAILED;
                ui.msg_until=0;draw_header();
                for(int i=0;i<2;i++) {
                    int x=222+i*10;CHECK(x-3>194 && x+3<236);
                    CHECK(fb[9*240+x]==swap16(i==page/4?SOUND_AMBER:SOUND_FACE));
                    CHECK(fb[9*240+x+2]!=swap16(SOUND_FACE)); /* antialiased outline */
                }
                if(state!=SAVE_SAVED)CHECK(fb[3*240+237]==swap16(state==SAVE_FAILED?RGB(255,70,70):C_AMB));
                CHECK(cur_page()==page);CHECK(!memcmp(&before,&proj.sound,sizeof before));
            }
            ui.dirty=save_wait=ui.save_failed=0;
        }
    }
    puts("PASS original matrix row geometry, screen-bank dots, save states and render side effects");
}
static void envelopes(void)
{
    static const int pots[]={0,64,127};
    fixture(0,0);*page_of(V_PART)=3;CHECK(adsr_inline_page());
    for(int a=0;a<3;a++)for(int d=0;d<3;d++)for(int s=0;s<3;s++)for(int r=0;r<3;r++) {
        int v[]={pots[a],pots[d],pots[s],pots[r]};
        for(int k=0;k<4;k++)pref_set(pg.r[3][k],v[k]);
        sound_t before=proj.sound;ui_adsr_t e;adsr_values(&e);
        /* #746: the graph uses the compiled native A/D/S/R (macro scaling included). */
        float nv[4];unsigned ng;CHECK(engine_bass_adsr_view(0,nv,&ng));
        double qa=nv[0],qd=nv[1],qr=nv[3];
        near(e.attack,(float)(13*qa*qa*qa*qa+.001));
        near(e.decay,(float)(25*qd*qd*qd*qd+.001));near(e.sustain,nv[2]);
        near(e.release,(float)(25*qr*qr*qr*qr+.002));CHECK(e.curved && e.release_curve==1 && e.enabled==2);
        for(int i=0;i<=10;i++) {
            float t=i*.1f;
            near(adsr_original_level(&e,0,t*e.attack),fminf(1.f,1.3f-1.3f*expf(-t)));
            near(adsr_original_level(&e,1,t*e.decay),e.sustain+(1-e.sustain)*expf(-t));
            near(adsr_original_level(&e,3,t*e.release),e.sustain*expf(-t));
        }
        int x[5];float durations[3];adsr_layout(&e,13,214,x,durations);
        CHECK(x[0]==13 && x[4]==227 && x[3]-x[2]==42);
        CHECK(x[1]-x[0]>=3 && x[2]-x[1]>=3 && x[4]-x[3]>=3);
        near(durations[0],e.attack*1.466337f);near(durations[1],e.decay*4.6051702f);
        near(durations[2],e.sustain>.04f?e.release*logf(e.sustain/.04f):0);
        render();CHECK(!memcmp(&before,&proj.sound,sizeof before));
    }
    /* Held locks and playing motion change the graph without rewriting a patch. */
    pref_t attack=pg.r[3][0];pref_set(attack,51);ui.held_step=0;
    int lane=motion_alloc(proj.arr.lane,seq.ppat[ui.part],ui.part,T_303,0,7);CHECK(lane>=0);
    proj.arr.lane[lane].val[0]=61;CHECK(matrix_value(attack)==61);CHECK(pref_get(attack)==51);
    ui_adsr_t e;adsr_values(&e);float nv[4];unsigned ng;CHECK(engine_bass_adsr_view(0,nv,&ng));float q=nv[0];near(e.attack,13*q*q*q*q+.001f);
    ui.held_step=-1;for(int i=0;i<NSTEPS;i++)proj.arr.lane[lane].val[i]=71;
    engine_motion_enable(1);seq_start(&seq);run_ms(40);CHECK(engine_motion_value(lane)==71);
    CHECK(matrix_value(attack)==71);adsr_values(&e);CHECK(engine_bass_adsr_view(0,nv,&ng));q=nv[0];near(e.attack,13*q*q*q*q+.001f);
    seq_stop(&seq);run_ms(40);CHECK(pref_get(attack)==51);
    *page_of(V_PART)=0;CHECK(!adsr_inline_page());fixture(0,1);
    for(int p=0;p<pg.n;p++){*page_of(V_PART)=p;CHECK(!adsr_inline_page());}
    puts("PASS original Chicago exponential envelope laws, geometry, locks and motion; Miami has no editable ADSR");
}
static void transitions(void)
{
    for(int voice=0;voice<2;voice++)for(int bank=0;bank<2;bank++) {
        fixture(voice,bank);*page_of(V_PART)=sound_page();
        sound_t before=proj.sound;engine_bass_wheel_owners(1u<<voice);knob(0,bank?-1:1);
        CHECK(engine_bass_bank(voice)==bank && engine_bass_requested_bank(voice)==(bank^1));
        CHECK(engine_bass_preset_pending(voice));CHECK(pg.r[0][0].c==0);
        CHECK(pg.n==(bank?6:8));CHECK(!memcmp(&before,&proj.sound,sizeof before));
        knob(2,1);CHECK(engine_bass_preset_pending(voice));patch();
        CHECK(!engine_bass_preset_pending(voice) && engine_bass_bank(voice)==bank);
        CHECK(!memcmp(&before,&proj.sound,sizeof before));
        knob(0,bank?-1:1);engine_bass_wheel_owners(0);
        for(int i=0;i<600 && engine_bass_preset_pending(voice);i++)audio();
        ui_input_only();
        CHECK(engine_bass_bank(voice)==(bank^1));CHECK(cur_page()==sound_page());CHECK(pg.n==(bank?8:6));
        CHECK(engine_bass_sound_bank(&proj.sound,voice)==(bank^1));
        int preset=engine_bass_preset_index(voice);knob(1,1);
        for(int i=0;i<600 && engine_bass_preset_pending(voice);i++)audio();
        ui_input_only();
        CHECK(engine_bass_preset_index(voice)==preset+1);CHECK(cur_page()==sound_page());
        /* Unsupported stored identity must not be repaired by drawing or editing. */
        memset(proj.sound.v[T_303][voice],0,ORIGINAL_SOUND_BYTES);build_pages();
        CHECK(engine_bass_sound_bank(&proj.sound,voice)<0);
        before=proj.sound;render();CHECK(!memcmp(&before,&proj.sound,sizeof before));
        pref_set(PR(R_ENG,T_303,voice,0),64);CHECK(!memcmp(&before,&proj.sound,sizeof before));
    }
    puts("PASS both voices/banks pending/cancel/apply, selector retention and invalid patch side effects");
}
static void drums(void)
{
    for(int kit=0;kit<2;kit++)for(int voice=0;voice<NDRUM;voice++) {
        ui.part=kit;ui.sel[kit]=voice;set_view(V_PART);ui.touched=-1;
        for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++) {
            pref_t r=pg.r[p][k];if(r.kind!=R_NONE)CHECK(tw(&FONT_T,matrix_name(r))<=54);
        }
        sound_t before=proj.sound;render();CHECK(!memcmp(&before,&proj.sound,sizeof before));
    }
    puts("PASS both drum-kit control labels and render side effects");
}
int main(void)
{
    boot();run_ms(50);engine_guard_enable(0);
    CHECK(!strcmp(btn_label(B_EDIT),"PARA"));CHECK(!strcmp(btn_label(B_HOME),"PATCH"));
    CHECK(UI_FIRST_KNOB==5);CHECK(help_fits());
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)mapping(v,bank);
    geometry();envelopes();transitions();drums();
    printf("sound matrix original: %u checks PASS\n",checks);return 0;
}
