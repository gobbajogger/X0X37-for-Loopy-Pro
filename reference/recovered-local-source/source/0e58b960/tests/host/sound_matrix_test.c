/* SPDX-License-Identifier: GPL-3.0-only */
/* Production UI, independent DSP-law expectations, actual 240px framebuffer. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void near(float a,float b) { if(fabsf(a-b)>=0.025f)fprintf(stderr,"near: got %.9g expected %.9g\n",a,b); CHECK(fabsf(a-b)<0.025f); }
static void audio(void) { int32_t out[512]; engine_render(out,256); }
static int sound_page(void) { for(int p=0;p<pg.n;p++) if(pg.r[p][0].kind==R_BENGINE)return p; return -1; }
static void fixture(int bank)
{
    ui.held_step=-1;ui.rec=0;ui.overlay=O_NONE;held_keys=held_btn=0;seq.playing=0;
    engine_bass_off(0);engine_bass_off(1);
    for(int i=0;i<600;i++)audio();
    memset(proj.arr.lane,0,sizeof proj.arr.lane);
    CHECK(engine_bass_select_preset(0,bank,0,&proj.sound)==0);
    for(int i=0;i<600 && engine_bass_preset_pending(0);i++)audio();
    CHECK(!engine_bass_preset_pending(0));CHECK(engine_bass_bank(0)==bank);
    ui.part=PART_303A;set_view(V_PART);*page_of(V_PART)=0;
    ui.touched=-1;ui.msg_until=0;ui_input_only();build_pages();
}
static void knob(int k,int delta)
{ now_ms+=1000;enc_acc[EN_K1+k]=delta;ui_input_only(); }
static void select_row(int delta)
{ now_ms++;enc_acc[EN_SELECT]=delta;ui_input_only(); }
static void render(void) { ui.msg_until=0;draw_header();draw_main();draw_knobs(); }
static void shot(const char *dir,const char *name)
{ char path[512];render();snprintf(path,sizeof path,"%s/%s.png",dir,name);png_write(path); }
static void value(int param,int val) { pref_set(PR(R_ENG,T_303,0,param),val); }
static void touch(int k)
{
    pref_t r=pg.r[cur_page()][k];ui.touched=k;ui.touch_until=now_ms+900;
    ui.touch_view=ui.view;ui.touch_part=ui.part;ui.touch_page=cur_page();
    ui.touch_kind=r.kind;ui.touch_a=r.a;ui.touch_b=r.b;ui.touch_c=r.c;
}
static void format(int param,int val,const char *expected)
{ char n[16],u[8],s[24];pref_value_of(PR(R_ENG,T_303,0,param),val,n,u);put_s(put_s(s,n),u);CHECK(!strcmp(s,expected)); }
static void mapping(int bank)
{
    static const int expected[2][16]={
      {BASS303_CUTOFF,BASS303_RESO,BASS303_ENVMOD,BASS303_WAVE,BASS303_ACCENT,BASS303_DRIVE,BASS303_GNAW,BASS303_BITE,
       BASS303_FATTACK,BASS303_DECAY,BASS303_FSUSTAIN,BASS303_FRELEASE,BASS303_AATTACK,BASS303_ADECAY,BASS303_ASUSTAIN,BASS303_ARELEASE},
      {W_CARRIER_WAVE,W_CARRIER_LEVEL,W_SUB_LEVEL,W_CARRIER_PITCH,W_XMOD_WAVE,W_XMOD_LEVEL,W_XMOD_TUNE,W_XMOD_DEPTH,
       W_WOBBLE_SHAPE,W_WOBBLE_DEPTH,W_WOBBLE_RATE,W_CUTOFF,W_ATTACK,W_DECAY,W_SUSTAIN,W_RELEASE}};
    unsigned seen[NPARAMS_MAX]={0};
    fixture(bank);CHECK(pg.n==7);CHECK(sound_page()==pg.n-1);
    for(int i=0;i<factory_bank(bank)->npresets;i++) {
        const char *name=factory_preset_name(bank,i);
        preset_text_t text;preset_text(&text,name,!bank);
        CHECK(text.lines>=1 && text.lines<=2);
        for(int line=0;line<text.lines;line++) CHECK(tw(text.font,text.line[line])<=54);
    }
    for(int p=0;p<4;p++)for(int k=0;k<4;k++)CHECK(pg.r[p][k].c==expected[bank][p*4+k]);
    for(int p=0;p<pg.n;p++) {
        *page_of(V_PART)=p;
        for(int k=0;k<4;k++) {
            pref_t r=pg.r[p][k];const x0x_param_t *d=pref_desc(r);
            if(r.kind==R_NONE) { sound_t before=proj.sound;knob(k,1);CHECK(!memcmp(&before,&proj.sound,sizeof before));continue; }
            CHECK(tw(&FONT_T,matrix_name(r))<=54);
            if(r.kind==R_ENG) {
                seen[r.c]++;
                pref_set(r,d->max/2);int before=pref_get(r);knob(k,1);
                if(bank==FACTORY_BANK_CHICAGO && r.a==T_303 && r.c==CHI_DAMPER) {
                    CHECK(pref_get(r)==127);knob(k,-1);CHECK(pref_get(r)==0);
                } else CHECK(pref_get(r)==before+1);
            }
            for(int v=0;v<=d->max;v++) {
                char n[16],u[8],s[24];pref_value_of(r,v,n,u);put_s(put_s(s,n),u);
                CHECK(tw(&FONT_T,s)<=54); /* smallest existing face bounds worst case */
            }
        }
    }
    for(int i=0;i<pg.nmore;i++) {
        pref_t r=pg.more[i];CHECK(r.kind==R_ENG);seen[r.c]++;
        const x0x_param_t *d=pref_desc(r);pref_set(r,d->max/2);int before=pref_get(r);
        open_list_of_pages();int found=-1;
        for(int j=0;j<ui.list_n;j++) if(!memcmp(&list_rows[j],&r,sizeof r))found=j;
        CHECK(found>=0);ui.list_sel=found;now_ms+=1000;enc_acc[EN_ALGO]=1;ui_input_only();
        CHECK(pref_get(r)==before+1);ui.overlay=O_NONE;
    }
    for(int i=0;i<engine_nparams(T_303,0);i++) CHECK(seen[i]==(i==(bank?W_PITCH_BEND:CHI_PBEND)?0:1)); /* performance-only physical wheel */
    *page_of(V_PART)=0;select_row(-1);CHECK(cur_page()==pg.n-1);select_row(1);CHECK(cur_page()==0);
    for(int p=1;p<pg.n;p++){select_row(1);CHECK(cur_page()==p);CHECK(cur_page()/4==p/4);}
    select_row(1);CHECK(cur_page()==0);*page_of(V_PART)=pg.n-1;button_tap(B_EDIT);CHECK(cur_page()==0);
    button_tap(B_EDIT);CHECK(cur_page()==1);select_row(-1);CHECK(cur_page()==0);
    *page_of(V_PART)=sound_page();int n=engine_bass_requested_preset(0);now_ms++;enc_acc[EN_PRESET]=1;ui_input_only();
    CHECK(engine_bass_requested_preset(0)==n+1);CHECK(engine_bass_preset_pending(0));
    knob(2,1);CHECK(engine_bass_preset_pending(0)); /* former K7 cancel slot is inert */
    held_btn=1u<<B_HOME;ui_input_only();held_btn=0;ui_input_only();CHECK(!engine_bass_preset_pending(0));
    puts(bank?"PASS Miami all controls, blanks, widths and navigation":"PASS Chicago all controls, blanks, widths and navigation");
}
static void geometry(const char *dir,int bank)
{
    fixture(bank);
    for(int row=0;row<4;row++) {
        char name[40];*page_of(V_PART)=row;ui.touched=-1;render();
        uint16_t col=swap16(matrix_col());
        for(int r=0;r<4;r++)for(int k=0;k<4;k++) {
            int x=7+k*59,y=24+r*50+24;
            CHECK((fb[y*240+x]==col)==(r==row));
        }
        snprintf(name,sizeof name,"%s-row%d",bank?"miami":"chicago",row+1);shot(dir,name);
    }
    *page_of(V_PART)=4;shot(dir,bank?"miami-page2":"chicago-page2");
    *page_of(V_PART)=sound_page();shot(dir,bank?"miami-presets":"chicago-presets");
}
/* Independent DSP states prove both stock amp release constants and the
 * Gate-mode recalc on each control block (not merely a renderer self-test). */
/* The header indicators describe screen banks, not rows or musical divisions. */
static void page_dots(const char *dir)
{
    for (int bank = 0; bank < 2; bank++) {
        fixture(bank);
        int rows = pg.n, banks = (rows + 3) / 4;
        CHECK(banks == 2 && rows == 7);
        CHECK(7 + tw(&FONT_B, bank ? "MIAMI A" : "CHICAGO A") < 175);
        for (int page = 0; page < rows; page++) {
            *page_of(V_PART) = page; ui.msg_until = 0;
            sound_t before = proj.sound;
            for (int state = SAVE_SAVED; state <= SAVE_FAILED; state++) {
                ui.dirty = state == SAVE_UNSAVED; save_wait = state == SAVE_QUEUED;
                ui.save_failed = state == SAVE_FAILED;
                draw_header();
                for (int i = 0; i < banks; i++) {
                    int x = 232 - (banks - 1 - i) * 10;
                    CHECK(x - 3 > 194 && x + 3 < 236); /* after REC, before dirty marker */
                    int night=!bank && page>=4;
                    CHECK(fb[9 * 240 + x] == swap16(i == page / 4 ? (night?RGB(196,155,69):C_HI) : (night?RGB(16,26,31):C_BLACK)));
                    CHECK(fb[9 * 240 + x + 2] != swap16(C_BLACK)); /* outlined inactive remains visible */
                }
                uint16_t dirty = state == SAVE_SAVED ? (!bank && page>=4?RGB(16,26,31):C_BLACK) : state == SAVE_FAILED ? RGB(255,70,70) : C_AMB;
                CHECK(fb[3 * 240 + 237] == swap16(dirty));
                CHECK(cur_page() == page && pg.n == rows);
                CHECK(!memcmp(&before, &proj.sound, sizeof before));
            }
            ui.dirty = save_wait = ui.save_failed = 0;
        }
        for (int screen = 0; screen < banks; screen++) {
            char name[48]; *page_of(V_PART) = screen ? rows - 1 : 0;
            snprintf(name, sizeof name, "%s-page-dots-%d", bank ? "miami" : "chicago", screen + 1);
            shot(dir, name);
        }
    }
    /* Unknown retained identity exposes one safe SOUND row, hence one bank. */
    fixture(FACTORY_BANK_CHICAGO);
    sound_t before = proj.sound;
    memset(proj.sound.bass[0].bytes, 0, sizeof proj.sound.bass[0].bytes);
    build_pages(); *page_of(V_PART) = 0; ui.dirty = save_wait = ui.save_failed = 0;
    CHECK(pg.n == 1); draw_header();
    CHECK(7 + tw(&FONT_B, "CHOOSE ENGINE") < 175);
    CHECK(fb[9 * 240 + 232] == swap16(C_HI));
    CHECK(fb[9 * 240 + 222] == swap16(C_BLACK));
    shot(dir, "unknown-single-page-dot");
    proj.sound = before; build_pages();
    puts("PASS screen-bank dots: both banks/engines, partial bank, unknown single bank, dirty marker and bounds");
}

static void dsp_release(void)
{
    bass303_t b;float out[BASS303_CTRL];
    for(int accent=0;accent<2;accent++) {
        bass303_init(&b);bass303_set(&b,BASS303_AMPMODE,BASS303_AMP_ENV);
        bass303_set(&b,BASS303_ARELEASE,0);bass303_note_on(&b,48,accent,0);
        bass303_render(&b,out,BASS303_CTRL); /* consume note-on jump before note-off */
        b.amp_y=1; bass303_note_off(&b);bass303_render(&b,out,BASS303_CTRL);
        near(b.amp_y,expf(-BASS303_CTRL/(BASS303_SR*(accent?.05f:.001f))));
    }
    for(int pot=1;pot<=31;pot+=30) {
        bass303_init(&b);bass303_set(&b,BASS303_AMPMODE,BASS303_AMP_GATE);
        bass303_set(&b,BASS303_ARELEASE,pot);bass303_note_on(&b,48,0,0);
        bass303_render(&b,out,BASS303_CTRL);b.fenv_y=.4f;bass303_note_off(&b);
        ui_adsr_t e={0};e.sustain=.4f;e.release=4000.0f*pot*pot/(127*127);e.release_curve=2;
        float n=e.release*BASS303_SR/(1000*BASS303_CTRL),base=1-1/n;
        for(int chunk=1;chunk<=8;chunk++) {
            bass303_render(&b,out,BASS303_CTRL);
            float expected=.4f*powf(base,chunk);
            near(b.fenv_y,expected);near(adsr_release(&e,chunk/(5*n)),expected);
        }
    }
}
/* Independent formula expectations for the actual production breakpoint helper.
 * Readouts are rounded nominal values; geometry uses unrounded float durations. */
static void miami_time_geometry(const char *dir)
{
    static const int pots[3]={0,64,127};
    ui_adsr_t e;int x[5];
    fixture(FACTORY_BANK_MIAMI);*page_of(V_PART)=3;
    CHECK(tw(&FONT_T,"AUTO TIME / HOLD 250ms")<=214);
    for(int a=0;a<3;a++)for(int d=0;d<3;d++)for(int su=0;su<3;su++)for(int r=0;r<3;r++) {
        int ap=pots[a],dp=pots[d],sp=pots[su],rp=pots[r];
        value(W_ATTACK,ap);value(W_DECAY,dp);value(W_SUSTAIN,sp);value(W_RELEASE,rp);
        adsr_values(&e);CHECK(e.timed && !e.curved);adsr_time_points(&e,x);
        double attack=2.+1998.*ap*ap/(127.*127.);
        double decay=sp==127?0.:.1+2499.9*dp*dp/(127.*127.);
        double release=(4.75+2995.25*rp*rp/(127.*127.))*sp/127.;
        double total=attack+decay+250.+release;
        CHECK(x[0]==13 && x[4]==227);
        CHECK(x[1]==13+(int)floor(214.*attack/total+.5));
        CHECK(x[2]==13+(int)floor(214.*(attack+decay)/total+.5));
        CHECK(x[3]==13+(int)floor(214.*(attack+decay+250.)/total+.5));
        for(int i=1;i<5;i++)CHECK(x[i]>=x[i-1] && x[i]<=227);
        if(sp==127)CHECK(x[1]==x[2]);
        if(sp==0)CHECK(x[3]==x[4]);
        touch((a+d+su+r)%4);
        char name[80];snprintf(name,sizeof name,"miami-time-a%d-d%d-s%d-r%d",ap,dp,sp,rp);
        shot(dir,name); /* actual maintained renderer, including both display bands */
    }
    value(W_ATTACK,0);value(W_DECAY,127);value(W_SUSTAIN,64);value(W_RELEASE,0);
    adsr_values(&e);adsr_time_points(&e,x);int short_peak=x[1];
    value(W_ATTACK,127);adsr_values(&e);adsr_time_points(&e,x);CHECK(x[1]>short_peak);
    value(W_ATTACK,64);value(W_DECAY,0);adsr_values(&e);adsr_time_points(&e,x);int short_decay=x[2]-x[1];
    value(W_DECAY,127);adsr_values(&e);adsr_time_points(&e,x);CHECK(x[2]-x[1]>short_decay);
    fixture(FACTORY_BANK_CHICAGO);*page_of(V_PART)=3;adsr_values(&e);CHECK(e.timed==2);
    puts("PASS Miami actual renderer: 81 time/sustain corners, monotonic geometry, duration response, ASCII caption; Chicago separate windows");
}

static void envelopes(const char *dir)
{
    ui_adsr_t e;
    fixture(FACTORY_BANK_MIAMI);*page_of(V_PART)=3;
    value(W_ATTACK,32);value(W_DECAY,60);value(W_SUSTAIN,51);value(W_RELEASE,31);
    adsr_values(&e);CHECK(!e.curved && !e.gate && e.enabled);
    near(e.attack,2.0f+1998.0f*32*32/(127*127));near(e.decay,0.1f+2499.9f*60*60/(127*127));
    near(e.sustain,51.0f/127);near(e.release,(4.75f+2995.25f*31*31/(127*127))*51/127);
    format(W_ATTACK,32,"129ms");format(W_DECAY,60,"558ms");format(W_RELEASE,31,"183ms");
    format(W_ATTACK,0,"2.0ms");format(W_DECAY,0,"0.1ms");format(W_RELEASE,0,"4.8ms");
    format(W_ATTACK,127,"2000ms");format(W_DECAY,127,"2500ms");format(W_RELEASE,127,"3000ms");
    touch(2);CHECK(adsr_touch_active());shot(dir,"miami-envelope-mid");
    int points[5];adsr_time_points(&e,points);
    int decay_mid=points[1]+(points[2]-points[1])*12/24;
    CHECK(fb[100*240+decay_mid]==swap16(matrix_col())); /* linear decay midpoint */
    uint16_t saved[240*240];memcpy(saved,fb,sizeof saved);now_ms+=500;render();CHECK(!memcmp(saved,fb,sizeof saved));
    now_ms+=401;CHECK(!adsr_touch_active());shot(dir,"miami-envelope-restored");
    value(W_ATTACK,0);value(W_DECAY,0);value(W_SUSTAIN,0);value(W_RELEASE,0);touch(0);adsr_values(&e);
    near(e.attack,2);near(e.decay,.1f);near(e.release,0);shot(dir,"miami-envelope-min");
    value(W_ATTACK,127);value(W_DECAY,127);value(W_SUSTAIN,127);value(W_RELEASE,127);touch(3);adsr_values(&e);
    near(e.release,3000);shot(dir,"miami-envelope-max");
    select_row(-1);CHECK(!adsr_touch_active()); /* no stale row detent */
    fixture(FACTORY_BANK_CHICAGO);*page_of(V_PART)=3;
    value(BASS303_AMPMODE,BASS303_AMP_ENV);value(BASS303_AATTACK,32);value(BASS303_ADECAY,64);value(BASS303_ASUSTAIN,51);value(BASS303_ARELEASE,31);
    adsr_values(&e);CHECK(e.curved && !e.gate && e.enabled);near(e.attack,2000.0f*32*32/(127*127));near(e.decay,1230);
    near(e.release,1.0f+3999.0f*31*31/(127*127));format(BASS303_ARELEASE,0,"1/50ms");
    for(int i=0;i<=10;i++){float u=(float)i/10;near(adsr_fall(1,1,.4f,u),.4f+.6f*expf(-5*u));near(adsr_fall(0,1,.4f,u),1-.6f*u);}
    touch(2);shot(dir,"chicago-envelope-rc");
    adsr_time_points(&e,points);int cx=points[1]+(points[2]-points[1])*12/24;
    CHECK(fb[118*240+cx]==swap16(matrix_col())); /* independent exp(-2.5) midpoint */
    CHECK(fb[100*240+cx]!=swap16(matrix_col()));
    value(BASS303_AMPMODE,BASS303_AMP_GATE);adsr_values(&e);CHECK(!e.curved && e.gate && e.release_curve==2);
    near(e.decay,200.0f*powf(10.0f,64.0f/127));near(e.release,4000.0f*31*31/(127*127));
    format(BASS303_ADECAY,64,"638ms");format(BASS303_ARELEASE,0,"0ms");shot(dir,"chicago-envelope-gate");
    adsr_time_points(&e,points);cx=points[1]+(points[2]-points[1])*12/24;
    CHECK(fb[100*240+cx]==swap16(matrix_col()));
    cx=points[3]+(points[4]-points[3])*12/24;
    CHECK(fb[148*240+cx]==swap16(matrix_col())); /* geometric Gate release, not linear */
    *page_of(V_PART)=2;touch(1);adsr_values(&e);CHECK(!e.enabled);shot(dir,"chicago-filter-inactive-gate");
    value(BASS303_AMPMODE,BASS303_AMP_ENV);value(BASS303_FENV,0);adsr_values(&e);CHECK(!e.enabled);
    shot(dir,"chicago-filter-off");value(BASS303_FENV,1);adsr_values(&e);CHECK(e.enabled && !e.curved);
    shot(dir,"chicago-filter-active");
    *page_of(V_PART)=3;value(BASS303_AATTACK,0);value(BASS303_ADECAY,0);value(BASS303_ASUSTAIN,0);value(BASS303_ARELEASE,0);touch(0);
    shot(dir,"chicago-envelope-min");
    value(BASS303_AATTACK,127);value(BASS303_ADECAY,127);value(BASS303_ASUSTAIN,127);value(BASS303_ARELEASE,127);touch(3);
    shot(dir,"chicago-envelope-max");
    ui.part=PART_303B;build_pages();CHECK(!adsr_touch_active());ui.part=PART_303A;build_pages();
    /* A held step resolves to a distinct value without mutating the saved sound. */
    *page_of(V_PART)=3;ui.held_step=0;value(BASS303_AATTACK,51);int lane=motion_alloc(proj.arr.lane,seq.ppat[ui.part],ui.part,T_303,0,BASS303_AATTACK);
    CHECK(lane>=0);proj.arr.lane[lane].val[0]=61;touch(0);adsr_values(&e);near(e.attack,2000.0f*61*61/(127*127));
    CHECK(pref_get(pg.r[3][0])==51);CHECK(matrix_value(pg.r[3][0])==61);shot(dir,"chicago-envelope-locked");
    proj.arr.lane[lane].val[1]=91;ui.held_step=1;adsr_values(&e);near(e.attack,2000.0f*91*91/(127*127));
    ui.held_step=-1;adsr_values(&e);near(e.attack,2000.0f*51*51/(127*127));
    for(int i=0;i<NSTEPS;i++)proj.arr.lane[lane].val[i]=71;
    engine_motion_enable(1);seq_start(&seq);run_ms(40);
    CHECK(engine_motion_value(lane)==71);CHECK(matrix_value(pg.r[3][0])==71);
    touch(0);adsr_values(&e);near(e.attack,2000.0f*71*71/(127*127));shot(dir,"chicago-envelope-motion");
    seq_stop(&seq);run_ms(40);CHECK(pref_get(pg.r[3][0])==51);
    puts("PASS exact envelope laws, geometry function, modes, timeout and held values");
}
static void chicago_time_geometry(const char *dir)
{
    const int pots[3]={0,64,127};ui_adsr_t e;int x[5];
    CHECK(tw(&FONT_T,"AUTO / 5xRC / HOLD 250ms")<=214);
    CHECK(tw(&FONT_T,"AUTO / 5xR / HOLD 250ms")<=214);
    CHECK(tw(&FONT_S,"AMP ENV / NORMAL 1ms")<=232);
    for(int mode=0;mode<3;mode++) {
        fixture(FACTORY_BANK_CHICAGO);value(BASS303_AMPMODE,mode==2?BASS303_AMP_GATE:BASS303_AMP_ENV);
        value(BASS303_FENV,1);*page_of(V_PART)=mode==1?2:3;
        for(int a=0;a<3;a++)for(int d=0;d<3;d++)for(int su=0;su<3;su++)for(int r=0;r<3;r++) {
            int ap=pots[a],dp=pots[d],sp=pots[su],rp=pots[r];
            pref_set(pg.r[cur_page()][0],ap);pref_set(pg.r[cur_page()][1],dp);
            pref_set(pg.r[cur_page()][2],sp);pref_set(pg.r[cur_page()][3],rp);
            adsr_values(&e);adsr_time_points(&e,x);CHECK(e.timed==2);
            double A=2000.*ap*ap/(127.*127.);
            double D=sp==127?0.:(mode==0?fmax(1.,1230.*dp/64.)*5.:200.*pow(10.,dp/127.));
            double R=sp==0?0.:mode==0?(rp?1.+3999.*rp*rp/(127.*127.):1.)*5.:
                (4000.*rp*rp/(127.*127.))*(mode==2?5.:1.);
            double total=A+D+250.+R;
            CHECK(x[1]==13+(int)floor(214.*A/total+.5));
            CHECK(x[2]==13+(int)floor(214.*(A+D)/total+.5));
            CHECK(x[3]==13+(int)floor(214.*(A+D+250.)/total+.5));
            for(int j=1;j<5;j++)CHECK(x[j]>=x[j-1] && x[j]<=227);
            if(sp==0)CHECK(x[3]==x[4]);if(sp==127)CHECK(x[1]==x[2]);
            touch((a+d+su+r)%4);char name[80];snprintf(name,sizeof name,"chicago-time-m%d-a%d-d%d-s%d-r%d",mode,ap,dp,sp,rp);shot(dir,name);
        }
    }
    puts("PASS Chicago actual renderer: 243 window corners, distinct RC/linear/Gate laws and ASCII labels");
}

static void transitions(const char *dir)
{
    fixture(FACTORY_BANK_MIAMI);*page_of(V_PART)=sound_page();
    knob(0,-1);CHECK(engine_bass_bank(0)==FACTORY_BANK_MIAMI);
    CHECK(engine_bass_requested_bank(0)==FACTORY_BANK_CHICAGO);
    CHECK(engine_bass_preset_pending(0));CHECK(pg.r[0][0].c==W_CARRIER_WAVE);
    shot(dir,"preset-pending-chicago");knob(2,1);CHECK(engine_bass_preset_pending(0));
    held_btn=1u<<B_HOME;ui_input_only();held_btn=0;ui_input_only();CHECK(!engine_bass_preset_pending(0));
    CHECK(engine_bass_bank(0)==FACTORY_BANK_MIAMI);shot(dir,"preset-cancelled");
    knob(0,-1);audio();ui_input_only();
    CHECK(engine_bass_bank(0)==FACTORY_BANK_CHICAGO);CHECK(cur_page()==sound_page());CHECK(cur_page()==6);
    CHECK(pg.r[0][0].c==BASS303_CUTOFF);shot(dir,"preset-applied-chicago");
    int preset=engine_bass_preset_index(0);knob(1,1);audio();ui_input_only();
    CHECK(engine_bass_preset_index(0)==preset+1);CHECK(cur_page()==sound_page());
    /* Engine commit must not reinterpret an old amp-control touch on the new bank. */
    *page_of(V_PART)=3;touch(0);CHECK(adsr_touch_active());
    CHECK(!engine_bass_select_preset(0,FACTORY_BANK_MIAMI,0,&proj.sound));audio();ui_input_only();
    CHECK(engine_bass_bank(0)==FACTORY_BANK_MIAMI);CHECK(!adsr_touch_active());
    *page_of(V_PART)=sound_page();
    bass_sound_tag_t tag=proj.sound.bass[0];memset(&proj.sound.bass[0],0,sizeof tag);
    build_pages();CHECK(pg.n==1 && sound_page()==0 && cur_page()==0);
    CHECK(engine_bass_sound_bank(&proj.sound,0)<0);
    sound_t before=proj.sound;shot(dir,"unknown-engine-safe-selector");CHECK(!memcmp(&before,&proj.sound,sizeof before));
    CHECK(pref_get(pg.r[0][0])<0); /* no guessing an active engine from legacy bytes */
    proj.sound.bass[0]=tag;build_pages();CHECK(cur_page()==sound_page());
    puts("PASS pending/cancel/apply, engine row identity, stale touch and unknown-engine safety");
}
static void drums(const char *dir)
{
    for(int kit=0;kit<2;kit++)for(int voice=0;voice<NDRUM;voice++) {
        ui.part=kit;ui.sel[kit]=voice;set_view(V_PART);ui.touched=-1;
        int banks = (pg.n + 3) / 4;
        CHECK(banks >= 1 && 232 - (banks - 1) * 10 - 3 > 194);
        for (int page = 0; page < pg.n; page++) {
            *page_of(V_PART) = page; ui.msg_until = 0; draw_header();
            for (int i = 0; i < banks; i++) {
                int x = 232 - (banks - 1 - i) * 10;
                CHECK(fb[9 * 240 + x] == swap16(i == page / 4 ? C_HI : C_BLACK));
                CHECK(fb[9 * 240 + x + 2] != swap16(C_BLACK));
            }
        }
        for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++) {
            pref_t r=pg.r[p][k];if(r.kind==R_NONE)continue;
            CHECK(tw(&FONT_T,matrix_name(r))<=54);
        }
        if(voice==1)shot(dir,kit?"808-snare":"909-snare");
    }
    puts("PASS both drum-kit control labels");
}
int main(int argc,char **argv)
{
    const char *dir=argc>1?argv[1]:"build/ui-b-evidence";boot();
    if(getenv("X0X_ENVELOPE_ONLY")) {
        envelopes(dir);miami_time_geometry(dir);chicago_time_geometry(dir);
        printf("focused envelope: %u checks PASS\n",checks);return 0;
    }
    CHECK(!strcmp(btn_label(B_EDIT),"PARA"));CHECK(!strcmp(btn_label(B_HOME),"PATCH"));
    CHECK(!strcmp(btn_label(B_SEL),"SCALE"));CHECK(!strcmp(btn_label(B_SAVE),"BT"));
    CHECK(UI_FIRST_KNOB==5);CHECK(help_fits());
    CHECK(tw(&FONT_T,"STEP 64 " UI_CAP_ARROWS ": CHANCE " UI_CAP_EDIT ":ROW")+tw(&FONT_T,"KEYS/MIDI")<222);
    dsp_release();mapping(0);mapping(1);geometry(dir,0);geometry(dir,1);page_dots(dir);envelopes(dir);miami_time_geometry(dir);chicago_time_geometry(dir);transitions(dir);drums(dir);
    printf("sound matrix B: %u checks PASS\n",checks);return 0;
}
