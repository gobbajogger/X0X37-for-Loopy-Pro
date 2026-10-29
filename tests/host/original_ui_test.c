/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual original UI + pure presentation projection; no device or native build. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../experiments/original_synth/chicago_voice.h"
#include "../../experiments/original_synth/miami_selected_voice.h"

static unsigned checks;
static const char *image_dir;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void render_block(void){int32_t audio[512];engine_render(audio,256);}
static void settle(void){for(int i=0;i<12;i++)render_block();}
static int native_y(float z,float lo,float hi)
{int y=1+(int)((hi-z)*16.0f/(hi-lo)+.5f);return y<0?0:y>18?18:y;}

/* Independent oracle: full original compiler and actual waveform/MG evaluator,
 * not the UI's integer formulas, native getters or selector helper. */
static void compare_projection(const uint8_t cell[32],const uint8_t preview[22],uint32_t mask)
{
    ui_original_view v;uint8_t before[32];memcpy(before,cell,32);
    CHECK(ui_original_project(&v,cell,preview,mask));CHECK(!memcmp(before,cell,32));
    if(cell[29]==ORIGINAL_SOUND_CHICAGO){
        original_chicago_program p;original_chicago_voice_controls c;
        CHECK(original_sound_restore_chicago(&p,cell));
        for(unsigned i=0;i<22;i++)if(mask&(1u<<i)){float x;CHECK(original_sound_control_value(0,i,preview[i],&x));CHECK(original_chicago_program_set(&p,i,x));}
        CHECK(original_chicago_voice_compile(&c,&p));
        CHECK(v.arp_on==c.routing.arp_enable && v.arp_mode==c.routing.arp_mode && v.arp_range==c.routing.arp_range);
        CHECK(v.amp_gate==c.routing.vca_eg_mode && v.damper==(c.routing.damper>=64));
        for(int i=0;i<29;i++){
            float phase=1.0f-2.0f*i/28.0f,a,b;
            CHECK(original_waveform(c.shape[0],phase,0x02000000u,c.wave_width[0],&a));
            CHECK(original_waveform(c.shape[2],phase,0x02000000u,c.wave_width[2],&b));
            int y=native_y(a*c.gain.level[0]+b*c.gain.level[2],-1,1);
            CHECK(abs((int)v.wave[0][i]-y)<=1);
        }
    }else{
        original_miami_program p;original_miami_voice_controls c;
        CHECK(original_sound_restore_miami(&p,cell));
        for(unsigned i=0;i<18;i++)if(mask&(1u<<i))CHECK(original_miami_program_set(&p,i,(float)preview[i]/127.0f));
        CHECK(original_miami_voice_compile(&c,&p));CHECK(v.damper==c.damper);
        CHECK(v.shape[0]==c.shape[0] && v.shape[1]==c.shape[1] && v.shape[2]==c.mg_shape[0]);
        for(int s=0;s<3;s++){
            unsigned sh=s==2?c.mg_shape[0]:c.shape[s];float width=0,lo=-1,hi=1;
            if(s!=2){CHECK(original_pw_width(sh,c.pw_intensity,0,c.pw_width,0,0,&width));if(sh>=2){lo=-2*width;hi=2*(1-width);}}
            CHECK(abs((int)v.zero[s]-native_y(0,lo,hi))<=1);
            for(int i=0;i<29;i++){
                float phase=1.0f-2.0f*i/28.0f,z,u;
                if(s==2)CHECK(original_miami_mg_shape(sh,-phase,&z,&u));
                else CHECK(original_waveform(sh,phase,0x02000000u,width,&z));
                CHECK(abs((int)v.wave[s][i]-native_y(z,lo,hi))<=1);
            }
        }
    }
}
static void projection_cases(void)
{
    uint8_t cell[32],preview[22]={0};
    for(unsigned kind=0;kind<2;kind++)for(unsigned f=0;f<original_sound_factories(kind);f++){
        CHECK(original_sound_init(cell,kind,f));compare_projection(cell,preview,0);
    }
    static const unsigned ids[2][8]={{0,11,13,14,15,16,20,21},{0,6,11,9,10,13,16,17}};
    for(unsigned kind=0;kind<2;kind++){
        CHECK(original_sound_init(cell,kind,0));
        for(int n=0;n<8;n++)for(unsigned q=0;q<128;q++){
            memset(preview,0,sizeof preview);preview[ids[kind][n]]=(uint8_t)q;
            compare_projection(cell,preview,1u<<ids[kind][n]);
        }
    }
    /* Every excluded macro preview must leave the complete projection intact.
     * The native mapping is factory-independent; exhaust q127 at factory0,
     * then check both endpoints and middle at every admitted factory. */
    for(unsigned kind=0;kind<2;kind++)for(unsigned f=0;f<original_sound_factories(kind);f++){
        uint32_t relevant=kind?UI_ORIGINAL_MIAMI_PREVIEW:UI_ORIGINAL_CHICAGO_PREVIEW;
        CHECK(original_sound_init(cell,kind,f));ui_original_view base,changed;
        memset(preview,0,sizeof preview);CHECK(ui_original_project(&base,cell,preview,0));
        for(unsigned p=0;p<original_sound_count(kind);p++)if(!(relevant&(1u<<p))){
            unsigned max=original_sound_param(kind,p)->max;
            for(unsigned q=0;q<=max;q++)if(!f || q==0 || q==max/2 || q==max){
                memset(preview,0,sizeof preview);preview[p]=(uint8_t)q;
                CHECK(ui_original_project(&changed,cell,preview,1u<<p));CHECK(!memcmp(&base,&changed,sizeof base));
            }
        }
    }
    static const float widths[]={-1,0,.009f,.01f,.5f,.99f,.991f,1,2};
    for(unsigned i=0;i<sizeof widths/sizeof widths[0];i++)for(unsigned shape=0;shape<4;shape++){
        float width;CHECK(original_pw_width(shape,0,0,widths[i],0,0,&width));
        CHECK(abs(ui_original_width(shape,widths[i])-(int)(width*65536.0f+.5f))<=1);
    }
    ui_original_cache cache={0};memset(preview,0,sizeof preview);CHECK(original_sound_init(cell,0,0));
    CHECK(ui_original_update(&cache,1,cell,preview,0)==1);
    ui_original_cache before=cache;
    for(int i=0;i<1024;i++)CHECK(ui_original_update(&cache,1,cell,preview,0)==0);
    CHECK(!memcmp(&before,&cache,sizeof cache));
    CHECK(ui_original_update(&cache,0,cell,preview,0)==1);
    preview[0]=cell[0];CHECK(ui_original_update(&cache,0,cell,preview,1)==1);
    CHECK(ui_original_update(&cache,0,cell,preview,0)==1);
    ui_original_view v;CHECK(!ui_original_project(&v,cell,preview,1u<<22));
    preview[17]=7;CHECK(!ui_original_project(&v,cell,preview,1u<<17));preview[17]=0;
    cell[28]=0;CHECK(ui_original_update(&cache,0,cell,preview,0)==-1 && !cache.valid);
    CHECK(sizeof(ui_original_cache)==164 && sizeof(ui_original_view)==101);
}
static void choose(unsigned voice,unsigned kind,unsigned preset)
{
    CHECK(engine_bass_select_preset((int)voice,(int)kind,(int)preset,&proj.sound)==0);settle();
    CHECK(engine_bass_preset_pending((int)voice)==0 && engine_bass_status((int)voice)==1);
}
static void ui_test_frame(int view,int part,int page)
{
    ui.view=view;ui.part=part;ui.overlay=O_NONE;ui.help=0;ui.touched=-1;ui.msg_until=ui.touch_until=0;
    ui.page[view][view==V_PART?part:0]=page;build_pages();ui_frame();
}
static void capture(const char *name,int view,int part,int page)
{
    ui_test_frame(view,part,page);if(!image_dir)return;
    char path[1024];CHECK(snprintf(path,sizeof path,"%s/%s.png",image_dir,name)>0);png_write(path);
}
static void page_and_input_cases(void)
{
    for(unsigned kind=0;kind<2;kind++){
        unsigned voice=kind?0:1,part=PART_303A+voice;choose(voice,kind,0);ui_test_frame(V_PART,part,0);
        uint32_t seen=0;int pages=kind?6:7;CHECK(pg.n==pages);
        const char *const *titles=kind?ui_original_miami_titles:ui_original_chicago_titles;
        for(int p=0;p<pages-1;p++){
            CHECK(strlen(titles[p])<sizeof pg.title[p]);
            CHECK(!strcmp(matrix_group(p),titles[p]));
        }
        for(int p=0;p<pages-1;p++)for(int k=0;k<4;k++){
            pref_t r=pg.r[p][k];if(r.kind==R_NONE)continue;
            CHECK(r.kind==R_ENG && r.a==T_303 && r.b==voice && r.c<original_sound_count(kind));
            CHECK(!(seen&(1u<<r.c)));seen|=1u<<r.c;
            uint8_t before[32];memcpy(before,proj.sound.v[T_303][voice],32);
            int value=before[r.c],delta=value==pref_desc(r)->max?-1:1;
            ui.page[V_PART][part]=(uint8_t)p;now_ms+=500;enc_acc[EN_K1+k]=delta;ui_input_only();
            CHECK(proj.sound.v[T_303][voice][r.c]==value+delta);
            for(unsigned i=0;i<original_sound_count(kind);i++)if(i!=r.c)CHECK(proj.sound.v[T_303][voice][i]==before[i]);
            CHECK(original_sound_edit_mask(proj.sound.v[T_303][voice])&(1u<<r.c));render_block();
            ui_test_frame(V_PART,part,p);CHECK(original_ui.valid && original_ui.voice==voice && original_ui.view.kind==kind);
        }
        CHECK(seen==((1u<<original_sound_count(kind))-1u));
        CHECK(pg.r[pages-1][0].kind==R_BENGINE && pg.r[pages-1][1].kind==R_BPRESET);
        CHECK(pg.r[pages-1][2].kind==R_NONE && pg.r[pages-1][3].kind==R_NONE);
        for(int p=0;p<pages-1;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_ENG)
            CHECK(tw(&FONT_T,matrix_name(pg.r[p][k]))<=54);
        char caption[24];matrix_knob_caption(caption,kind?4:3);
#if X0X_SMK_CAPTIONS
        CHECK(!strcmp(caption,kind?"KNOBS 5-6,8":"KNOBS 5-6"));
#else
        CHECK(!strcmp(caption,kind?"KNOBS 1-2,4":"KNOBS 1-2"));
#endif
    }
}
static void preview_cases(void)
{
    choose(0,1,0);ui_test_frame(V_PART,PART_303A,0);uint8_t saved[32];memcpy(saved,proj.sound.v[T_303][0],32);
    int lane=motion_alloc(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,0);CHECK(lane>=0);
    proj.arr.lane[lane].val[0]=0;ui.held_step=0;ui.held_step_part=PART_303A;ui.held_step_pat=seq.ppat[PART_303A];ui.rec=0;
    original_ui_prepare();CHECK(original_ui.preview_mask&1);CHECK(original_ui.view.shape[0]==0);CHECK(matrix_value(PR(R_ENG,T_303,0,0))==0);
    CHECK(!memcmp(saved,proj.sound.v[T_303][0],32));
    ui.held_step=-1;original_ui_prepare();CHECK(!(original_ui.preview_mask&1));CHECK(original_ui.view.shape[0]==2);
    for(unsigned i=0;i<NSTEPS;i++)proj.arr.lane[lane].val[i]=0;
    seq_start(&seq);render_block();CHECK(engine_motion_value(lane)==0);original_ui_prepare();
    CHECK(original_ui.preview_mask&1);CHECK(original_ui.view.shape[0]==0);CHECK(!memcmp(saved,proj.sound.v[T_303][0],32));
    seq_stop(&seq);render_block();engine_motion_clear(lane);render_block();original_ui_prepare();
    CHECK(!(original_ui.preview_mask&1));CHECK(original_ui.view.shape[0]==2);CHECK(!memcmp(saved,proj.sound.v[T_303][0],32));
}
static void image_cases(void)
{
    choose(0,1,0);choose(1,0,0);
    capture("home",V_HOME,PART_303A,0);capture("mix-parts",V_MIX,PART_303A,0);capture("mix-master",V_MIX,PART_303A,3);capture("mix-wave-idle",V_MIX,PART_303A,4);
    for(int p=0;p<6;p++){char n[64];snprintf(n,sizeof n,"miami-page-%d",p);capture(n,V_PART,PART_303A,p);}
    for(int p=0;p<7;p++){char n[64];snprintf(n,sizeof n,"chicago-page-%d",p);capture(n,V_PART,PART_303B,p);}
    for(int fx=0;fx<7;fx++){
        pref_set(PR(R_ENG,T_303,1,17),fx);render_block();char n[64];snprintf(n,sizeof n,"chicago-fx-%d",fx);capture(n,V_PART,PART_303B,5);
        const char*e1[] = {"EDIT 1","RATE","RATE","FREQ","SENS","RATE","FREQ"};
        const char*e2[] = {"EDIT 2","FB/WET","MOD DEPTH","MIX","ATTACK","MIX","GAIN"};
        CHECK(!strcmp(matrix_name(PR(R_ENG,T_303,1,18)),e1[fx]));CHECK(!strcmp(matrix_name(PR(R_ENG,T_303,1,19)),e2[fx]));
    }
    for(int d=0;d<3;d++){
        pref_set(PR(R_ENG,T_303,1,14),(d*127+1)/2);render_block();char n[64];snprintf(n,sizeof n,"chicago-arp-direction-%d",d);capture(n,V_PART,PART_303B,4);
    }
    pref_set(PR(R_ENG,T_303,1,13),0);render_block();capture("chicago-arp-off",V_PART,PART_303B,4);
    pref_set(PR(R_ENG,T_303,1,11),0);render_block();capture("chicago-envelope-amp-env",V_PART,PART_303B,2);
    choose(1,0,54);capture("chicago-last-factory",V_PART,PART_303B,6);
    choose(0,1,29);capture("miami-last-factory",V_PART,PART_303A,5);
    for(int s=0;s<4;s++){
        pref_set(PR(R_ENG,T_303,0,0),s<3?(s*127+1)/2:127);
        pref_set(PR(R_ENG,T_303,0,6),(s*127+1)/3);pref_set(PR(R_ENG,T_303,0,11),(s*127+1)/3);render_block();
        char n[64];snprintf(n,sizeof n,"miami-shapes-%d",s);capture(n,V_PART,PART_303A,0);
    }
    choose(0,1,0);engine_bass_on_velocity(0,60,0,0,100);settle();
    int nonzero=0;for(unsigned i=0;i<SCOPE_N;i++)nonzero|=eng_scope[i]!=0;CHECK(nonzero);now_ms+=200;
    capture("mix-wave-sounding",V_MIX,PART_303A,4);engine_bass_off(0);settle();
    engine_bass_on_velocity(1,60,0,0,100);settle();CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==0);render_block();
    CHECK(engine_bass_preset_pending(1)==2);capture("chicago-sound-waiting",V_PART,PART_303B,6);engine_bass_off(1);
}
int main(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:NULL;projection_cases();load_force=0;boot();settle();
    page_and_input_cases();preview_cases();image_cases();
    printf("PASS original_ui checks=%u cache=%zu view=%zu waveform_reference=1/128 max_pixel_error=1\n",checks,sizeof(ui_original_cache),sizeof(ui_original_view));return 0;
}
