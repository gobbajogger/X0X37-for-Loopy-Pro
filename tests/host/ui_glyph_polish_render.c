/* SPDX-License-Identifier: GPL-3.0-only */
/* Real SMK-37 UI/framebuffer and original engines. Only hardware is stubbed by
 * the existing SMC fixture; time advances through the production audio render. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"

static const unsigned frame_ms[]={0,8,16,24,40,60,90,120,160,220,300,420,580,800,1100,1500,2000,2600};
static const int pots[]={0,32,64,96,127};
static const int pct[]={0,25,50,75,100};
static FILE *manifest;
static void tick(unsigned ms) { run_ms(ms); }
static void mono(void) { proj.set.palette=4;palette_set(4); }
static void draw_frame(void) { ui.msg_until=0; ui_frame(); draw_main(); }
static void select_engine(int kind)
{
    CHECK(engine_bass_select_preset(1,kind,0,&proj.sound)==0);
    for(int i=0;i<20;i++) { int32_t audio[512];engine_render(audio,256); }
    CHECK(!engine_bass_preset_pending(1));
}
static void set_page(int page)
{ *page_of(V_PART)=(uint8_t)page;build_pages();ui.touched=-1;ui.touch_until=0;ui.msg_until=0;draw_frame(); }
static void save_frame(const char *series,int value,int frame)
{
    char name[160];snprintf(name,sizeof name,"%s-v%03d-f%02d",series,value,frame);shot(name);
    fprintf(manifest,"%s,%d,%d,%u,%s.png\n",series,value,frame,frame_ms[frame],name);
}
static void bass_sequence(const char *series,int kind,int page,int control,int pot,int value)
{
    setup(V_PART,PART_303B); mono();audio_due_ms=now_ms;select_engine(kind);
    if(!kind) {
        pref_set(PR(R_ENG,T_303,1,13),0); /* ARP off, direct engine note. */
        pref_set(PR(R_ENG,T_303,1,11),0); /* Real ADSR mode, not factory GATE. */
        pref_set(PR(R_ENG,T_303,1,2),64);pref_set(PR(R_ENG,T_303,1,3),110);
        pref_set(PR(R_ENG,T_303,1,7),32);pref_set(PR(R_ENG,T_303,1,8),48);
        pref_set(PR(R_ENG,T_303,1,9),80);pref_set(PR(R_ENG,T_303,1,10),40);
    }
    pref_set(PR(R_ENG,T_303,1,control),pot);
    tick(30);set_page(page);save_frame(series,value,0);
    engine_bass_on_velocity(1,48,0,0,120);
    unsigned elapsed=0;int released=0;
    for(unsigned f=1;f<sizeof frame_ms/sizeof frame_ms[0];f++) {
        if(!released && frame_ms[f]>=580) { if(elapsed<500)tick(500-elapsed);elapsed=500;engine_bass_off(1);released=1; }
        tick(frame_ms[f]-elapsed);elapsed=frame_ms[f];draw_frame();save_frame(series,value,(int)f);
    }
}
static int param_named(int part,int voice,const char *name)
{
    int type=part==PART_808?T_808:T_909;
    if(voice==NDRUM && !strcmp(name,"Accent")) return 1; /* actual visible KIT cell */
    for(int c=0;c<engine_nparams(type,voice);c++) {
        pref_t r=PR(R_ENG,type,voice,c);const x0x_param_t *d=pref_desc(r);
        if(!d) break;
        if(!strcmp(d->name,name)) return c;
    }
    fprintf(stderr,"missing %s part %d voice %d\n",name,part,voice);exit(2);
}
static int page_for(int type,int voice,int control)
{
    for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++) {
        pref_t r=pg.r[p][k];if(r.kind==R_ENG && r.a==type && r.b==voice && r.c==control)return p;
    }
    return -1;
}
static void drum_sequence(const char *series,int part,int voice,const char *control,int pot,int value)
{
    setup(!strcmp(control,"Level")?V_MIX:V_PART,part);mono();audio_due_ms=now_ms;ui.sel[part]=(uint8_t)(voice==NDRUM?0:voice);build_pages();
    int type=part==PART_808?T_808:T_909,c=param_named(part,voice,control),page=page_for(type,voice,c);
    CHECK(page>=0);
    pref_t ref=PR(R_ENG,type,voice,c);const x0x_param_t *desc=pref_desc(ref);
    pot=(pot*desc->max+63)/127;pref_set(ref,pot);tick(30);
    *page_of(ui.view)=(uint8_t)page;build_pages();ui.touched=-1;ui.touch_until=0;draw_frame();save_frame(series,value,0);
    engine_drum(part,voice==NDRUM?0:voice,1.0f);
    unsigned elapsed=0;
    for(unsigned f=1;f<sizeof frame_ms/sizeof frame_ms[0];f++) {
        tick(frame_ms[f]-elapsed);elapsed=frame_ms[f];draw_frame();save_frame(series,value,(int)f);
    }
}
static void original_atlas(int kind)
{
    setup(V_PART,PART_303B);mono();audio_due_ms=now_ms;select_engine(kind);build_pages();
    for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++) {
        pref_t r=pg.r[p][k];if(!orig_strip_slot(r))continue;
        const x0x_param_t *d=pref_desc(r);char series[96];snprintf(series,sizeof series,"%s-control-%02d",kind?"miami":"chicago",r.c);
        for(unsigned v=0;v<5;v++) {
            int pot=pots[v]*d->max/127;pref_set(r,pot);tick(15);set_page(p);save_frame(series,pct[v],0);
        }
    }
}
int main(void)
{
    const char *dir=getenv("SMC_UI_SHOTS");char path[1024];CHECK(dir);
    snprintf(path,sizeof path,"%s/frames.csv",dir);manifest=fopen(path,"w");CHECK(manifest);
    fprintf(manifest,"series,value,frame,time_ms,file\n");
    for(unsigned v=0;v<5;v++)bass_sequence("chicago-env",0,1,4,pots[v],pct[v]);
    for(unsigned v=0;v<5;v++)bass_sequence("chicago-adsr",0,3,8,pots[v],pct[v]);
    bass_sequence("chicago-level",0,4,12,100,75);
    bass_sequence("chicago-bite",0,2,5,100,75);
    bass_sequence("miami-level",1,4,14,100,75);
    bass_sequence("miami-wobble",1,2,12,100,75);
    const struct {const char *series;int part,voice;const char *control;} drums[]={
        {"909-decay",PART_909,0,"Decay"},{"909-tune",PART_909,0,"Tune"},
        {"909-attack",PART_909,0,"Attack"},{"909-pitch-depth",PART_909,0,"P.Dpth"},{"909-pitch",PART_909,0,"Pitch"},
        {"909-snappy",PART_909,1,"Snappy"},{"909-tone",PART_909,1,"Tone"},
        {"909-level",PART_909,0,"Level"},{"909-accent",PART_909,NDRUM,"Accent"},
        {"808-decay",PART_808,0,"Decay"},{"808-tune",PART_808,0,"Tune"},
        {"808-attack",PART_808,0,"Attack"},{"808-shell-tone",PART_808,1,"Tone"},{"808-sound",PART_808,2,"Sound"},
        {"808-snappy",PART_808,1,"Snappy"},{"808-tone",PART_808,0,"Tone"},
        {"808-level",PART_808,0,"Level"},{"808-accent",PART_808,NDRUM,"Accent"}
    };
    for(unsigned d=0;d<sizeof drums/sizeof drums[0];d++)for(unsigned v=0;v<5;v++)
        drum_sequence(drums[d].series,drums[d].part,drums[d].voice,drums[d].control,pots[v],pct[v]);
    original_atlas(0);original_atlas(1);fclose(manifest);
    printf("GLYPH FRAMEBUFFER RENDER PASS %u checks\n",checks);return 0;
}
