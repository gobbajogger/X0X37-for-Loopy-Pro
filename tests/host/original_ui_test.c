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
        CHECK(fabsf((float)v.env_percent/100.0f-c.env_depth)<=.00501f);
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
        for(unsigned i=0;i<18;i++)if(mask&(1u<<i)){
            float x=(float)preview[i]/127.0f;
            if(i==7)x=preview[i]<64?(float)preview[i]/128.0f:.5f+(float)(preview[i]-64)/126.0f;
            CHECK(original_miami_program_set(&p,i,x));
        }
        CHECK(original_miami_voice_compile(&c,&p));CHECK(v.damper==c.damper);
        CHECK(v.shape[0]==c.shape[0] && v.shape[1]==c.shape[1] && v.shape[2]==c.mg_shape[0]);
        CHECK(v.carrier_pitch==c.pitch.semitone[0]);
        CHECK(fabsf((float)v.xmod_tune/1000.0f-c.patch_slave)<=.000501f);
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
    static const unsigned ids[2][10]={{0,2,3,4,11,13,14,15,16,21},{0,3,6,7,9,10,11,13,16,17}};
    for(unsigned kind=0;kind<2;kind++){
        CHECK(original_sound_init(cell,kind,0));
        for(int n=0;n<10;n++)for(unsigned q=0;q<128;q++){
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
    CHECK(sizeof(ui_original_cache)==168 && sizeof(ui_original_view)==108);
}
static void choose(unsigned voice,unsigned kind,unsigned preset)
{
    CHECK(engine_bass_select_preset((int)voice,(int)kind,(int)preset,&proj.sound)==0);settle();
    CHECK(engine_bass_preset_pending((int)voice)==0 && engine_bass_status((int)voice)==1);
}
static void ui_test_frame(int view,int part,int page)
{
    ui.view=view;ui.part=part;ui.overlay=O_NONE;ui.help=0;ui.touched=-1;ui.msg_until=ui.touch_until=0;
    ui.page[view][view==V_PART || view==V_SEQ || view==V_GEN?part:0]=page;build_pages();ui_frame();
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
        uint32_t seen=0;int pages=kind?6:8;CHECK(pg.n==pages);
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
            CHECK(proj.sound.v[T_303][voice][r.c]!=value);
            for(unsigned i=0;i<original_sound_count(kind);i++)if(i!=r.c)CHECK(proj.sound.v[T_303][voice][i]==before[i]);
            CHECK(original_sound_edit_mask(proj.sound.v[T_303][voice])&(1u<<r.c));render_block();
            ui_test_frame(V_PART,part,p);CHECK(original_ui.valid && original_ui.voice==voice && original_ui.view.kind==kind);
        }
        /* Panel membership is independently known: 20 Chicago / 16 Miami.
         * The two following macro IDs are wheel and pedal endpoints only. */
        CHECK(seen==((1u<<(kind?16:20))-1u));
        CHECK(!(seen&(1u<<(kind?16:20))) && !(seen&(1u<<(kind?17:21))));
        CHECK(pg.r[pages-1][0].kind==R_BENGINE && pg.r[pages-1][1].kind==R_BPRESET);
        CHECK(pg.r[pages-1][2].kind==R_NONE && pg.r[pages-1][3].kind==R_NONE);
        for(int p=0;p<pages-1;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_ENG)
            CHECK(tw(&FONT_T,matrix_name(pg.r[p][k]))<=54);
        char caption[24];matrix_knob_caption(caption,4);
#if X0X_SMK_CAPTIONS
        CHECK(!strcmp(caption,kind?"KNOBS 5-7":"KNOBS 5-6"));
#else
        CHECK(!strcmp(caption,kind?"KNOBS 1-3":"KNOBS 1-2"));
#endif
    }
}
/* Expected public order from the original panel and captured selector values,
 * independent of firmware page tables and ui_original_states(). */
static void selector_input_cases(void)
{
    static const struct {unsigned kind,id,n;const char *text[10];} cases[]={
        {0,11,2,{"ENV","GATE"}},{0,13,2,{"OFF","ON"}},
        {0,14,3,{"DOWN","UP","UP/DN"}},{0,15,3,{"1 OCT","2 OCT","FULL"}},
        {0,16,10,{"1/2","1/4D","1/2T","1/4","1/8D","1/4T","1/8","1/8T","1/16","1/32"}},
        {1,0,3,{"TRI","SAW","SQR"}},{1,6,4,{"TRI","SAW","SQR","PUL"}},
        {1,11,4,{"TRI","DOWN","UP","SQR"}},
        {1,13,8,{"1/2","1/4","1/8","1/16","1/3","1/6","1/12","1/24"}},
        {1,3,25,{0}}
    };
    for(unsigned j=0;j<sizeof cases/sizeof cases[0];j++){
        unsigned kind=cases[j].kind,id=cases[j].id;choose(0,kind,0);
        pref_t r=PR(R_ENG,T_303,0,id);pref_set(r,0);render_block();ui_test_frame(V_PART,PART_303A,0);
        int page=-1,knob=-1;
        for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)
            if(pg.r[p][k].kind==R_ENG && pg.r[p][k].c==id){page=p;knob=k;}
        CHECK(page>=0 && knob>=0);
        for(int direction=1;direction>=-1;direction-=2){
            for(unsigned step=0;step<cases[j].n;step++){
                unsigned state=direction>0?step:cases[j].n-1-step;
                ui_test_frame(V_PART,PART_303A,page);
                char actual[16],unit[8],expected[16];
                original_value_text(&original_ui.view,(int)id,pref_get(r),actual,unit);
                if(kind && id==3)snprintf(expected,sizeof expected,"%s%d",state>12?"+":"",(int)state-12);
                else snprintf(expected,sizeof expected,"%s",cases[j].text[state]);
                CHECK(!strcmp(actual,expected) && !unit[0]);CHECK(tw(&FONT_T,actual)<=27);
                char generic[16],generic_unit[8];pref_value_of(r,pref_get(r),generic,generic_unit);
                CHECK(!strcmp(generic,actual) && !generic_unit[0]);
                uint8_t before[32];memcpy(before,proj.sound.v[T_303][0],32);
                now_ms+=500;enc_acc[EN_K1+knob]=direction;ui_input_only();render_block();
                for(unsigned p=0;p<original_sound_count(kind);p++)if(p!=id)CHECK(proj.sound.v[T_303][0][p]==before[p]);
                if(step+1==cases[j].n)CHECK(pref_get(r)==(direction>0?127:0));
            }
        }
    }
    /* A held-step switch turn changes one state, never the saved sound.
     * On release its preview returns to the exact base setting. */
    choose(0,1,0);ui_test_frame(V_PART,PART_303A,3);
    pref_t r=PR(R_ENG,T_303,0,13);pref_set(r,0);render_block();
    uint8_t before[32];memcpy(before,proj.sound.v[T_303][0],32);
    ui.held_step=0;ui.held_step_part=PART_303A;ui.held_step_pat=seq.ppat[PART_303A];ui.rec=0;
    lock_turn(r,1,EN_K1+1);render_block();original_ui_prepare();
    char value[16],unit[8];original_value_text(&original_ui.view,13,matrix_value(r),value,unit);
    CHECK(!strcmp(value,"1/4"));CHECK(!memcmp(before,proj.sound.v[T_303][0],32));
    lock_release();ui.held_step=-1;render_block();original_ui_prepare();
    original_value_text(&original_ui.view,13,matrix_value(r),value,unit);CHECK(!strcmp(value,"1/2"));
    int lane=motion_find(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,13);CHECK(lane>=0);
    engine_motion_clear(lane);render_block();
    /* TUNE's original neutral value must remain reachable after an edit, not
     * just in a factory preset. The adjacent values stay on opposite sides. */
    r=PR(R_ENG,T_303,0,7);ui_test_frame(V_PART,PART_303A,1);
    for(unsigned q=63;q<=65;q++){
        pref_set(r,(int)q);render_block();original_ui_prepare();
        original_value_text(&original_ui.view,7,(int)q,value,unit);
        CHECK(!strcmp(value,q==63?"-0.08":q==64?"0.00":"+0.08") && !unit[0]);
        if(q==64){float exact=-1;CHECK(original_sound_value(proj.sound.v[T_303][0],7,&exact) && exact==.5f);}
    }
    choose(0,0,0);r=PR(R_ENG,T_303,0,4);pref_set(r,64);render_block();
    float exact=-1;CHECK(original_sound_value(proj.sound.v[T_303][0],4,&exact) && exact==.5f);
    original_chicago_program p;original_chicago_voice_controls c;
    CHECK(original_sound_restore_chicago(&p,proj.sound.v[T_303][0]));
    CHECK(original_chicago_voice_compile(&c,&p) && c.env_depth==0.0f);
    static const unsigned env_q[]={0,64,127};static const char *const env_text[]={"-100","0","100"};
    for(unsigned n=0;n<3;n++){
        pref_set(r,(int)env_q[n]);render_block();original_ui_prepare();
        original_value_text(&original_ui.view,4,(int)env_q[n],value,unit);
        CHECK(!strcmp(value,env_text[n]) && !strcmp(unit,"%"));
        put_s(value+strlen(value),unit);CHECK(tw(&FONT_T,value)<=27);
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
    for(int p=0;p<8;p++){char n[64];snprintf(n,sizeof n,"chicago-page-%d",p);capture(n,V_PART,PART_303B,p);}
    static const char *const expected_fx[]={"OFF","CHO/FLG","PHASER","RING MOD","COMPRESSOR","DECIMATOR","EQ"};
    static const felucca_font_t *const expected_fx_font[]={&FONT_B,&FONT_B,&FONT_B,&FONT_XS,&FONT_T,&FONT_T,&FONT_B};
    CHECK(tw(param_value_font(NULL,54),"UNKNOWN")<=54);
    for(int fx=0;fx<7;fx++){
        const char *name=original_sound_fx_name((unsigned)fx);
        CHECK(name && !strcmp(name,expected_fx[fx]));
        /* Check the production choice, full label and unchanged card limit.
         * Expected fonts follow independently measured native advances. */
        const felucca_font_t *font=param_value_font(name,54);
        CHECK(font==expected_fx_font[fx]);CHECK(tw(font,name)<=54);
        pref_set(PR(R_ENG,T_303,1,17),fx);render_block();char n[64];snprintf(n,sizeof n,"chicago-fx-%d",fx);capture(n,V_PART,PART_303B,6);
        CHECK(!strcmp(matrix_name(PR(R_ENG,T_303,1,18)),"EDIT 1"));CHECK(!strcmp(matrix_name(PR(R_ENG,T_303,1,19)),"EDIT 2"));
    }
    for(int d=0;d<3;d++){
        pref_set(PR(R_ENG,T_303,1,14),(d*127+1)/2);render_block();char n[64];snprintf(n,sizeof n,"chicago-arp-direction-%d",d);capture(n,V_PART,PART_303B,5);
    }
    pref_set(PR(R_ENG,T_303,1,15),127);render_block();capture("chicago-arp-full",V_PART,PART_303B,5);
    pref_set(PR(R_ENG,T_303,1,13),0);render_block();capture("chicago-arp-off",V_PART,PART_303B,5);
    pref_set(PR(R_ENG,T_303,1,11),0);render_block();capture("chicago-envelope-amp-env",V_PART,PART_303B,3);
    choose(1,0,54);capture("chicago-last-factory",V_PART,PART_303B,7);
    choose(0,1,29);capture("miami-last-factory",V_PART,PART_303A,5);
    for(int s=0;s<4;s++){
        pref_set(PR(R_ENG,T_303,0,0),s<3?(s*127+1)/2:127);
        pref_set(PR(R_ENG,T_303,0,6),(s*127+1)/3);pref_set(PR(R_ENG,T_303,0,11),(s*127+1)/3);render_block();
        char n[64];snprintf(n,sizeof n,"miami-shapes-%d",s);capture(n,V_PART,PART_303A,0);
    }
    for(int s=0;s<8;s++){
        pref_set(PR(R_ENG,T_303,0,13),(s*127+3)/7);render_block();
        char n[64];snprintf(n,sizeof n,"miami-rate-%d",s);capture(n,V_PART,PART_303A,3);
    }
    /* Explicit final control-law renders. These are intentional edited
     * endpoint/neutral states, separate from the factory/page examples. */
    static const unsigned bipolar_q[]={0,64,127};
    static const char *const bipolar_state[]={"negative","zero","positive"};
    choose(1,0,0);
    for(unsigned i=0;i<3;i++){
        pref_set(PR(R_ENG,T_303,1,4),(int)bipolar_q[i]);render_block();
        char n[64];snprintf(n,sizeof n,"chicago-env-%s",bipolar_state[i]);capture(n,V_PART,PART_303B,1);
    }
    choose(0,1,0);
    for(unsigned i=0;i<3;i++){
        pref_set(PR(R_ENG,T_303,0,7),(int)bipolar_q[i]);render_block();
        char n[64];snprintf(n,sizeof n,"miami-tune-%s",bipolar_state[i]);capture(n,V_PART,PART_303A,1);
        char value[16],unit[8];original_value_text(&original_ui.view,7,(int)bipolar_q[i],value,unit);
        CHECK(!strcmp(value,i==0?"-5.00":i==1?"0.00":"+5.00") && !unit[0]);
        CHECK(tw(&FONT_T,value)<=27);
    }
    /* Retain the original pending-preset test's distinct starting program. */
    choose(1,0,54);
    choose(0,1,0);engine_bass_on_velocity(0,60,0,0,100);settle();
    int nonzero=0;for(unsigned i=0;i<SCOPE_N;i++)nonzero|=eng_scope[i]!=0;CHECK(nonzero);now_ms+=200;
    capture("mix-wave-sounding",V_MIX,PART_303A,4);engine_bass_off(0);settle();
    engine_bass_on_velocity(1,60,0,0,100);settle();CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==0);render_block();
    CHECK(engine_bass_preset_pending(1)==2);capture("chicago-sound-waiting",V_PART,PART_303B,7);engine_bass_off(1);
}
#include "original_ui_restore_test.h"

int main(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:NULL;projection_cases();load_force=0;boot();settle();
    page_and_input_cases();selector_input_cases();preview_cases();restored_ui_cases();image_cases();
    printf("PASS original_ui checks=%u cache=%zu view=%zu waveform_reference=1/128 max_pixel_error=1\n",checks,sizeof(ui_original_cache),sizeof(ui_original_view));return 0;
}
