/* SPDX-License-Identifier: GPL-3.0-only */
/* Capture-only fixture: uses APIs present on both #807's previous head and the
 * candidate. No proposed drawing is embedded here; every pixel is the firmware
 * host renderer's real 240x240 RGB565 framebuffer. Run only on the authorized box. */
#define X0X_SMK_CAPTIONS 1
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main

static const char *capture_dir,*capture_theme;
static void capture_picture(const char *name,int view,int part,int page,int playing)
{
    char path[1024];
    ui.view=(uint8_t)view;ui.part=(uint8_t)part;ui.overlay=O_NONE;ui.help=0;
    ui.touched=-1;ui.touch_until=ui.msg_until=0;ui.msg[0]=0;
    ui.held_step=ui.hold_step=-1;ui.btn=ui.keys=ui.rec=0;
    ui.page[view][view==V_PART||view==V_SEQ||view==V_GEN?part:0]=(uint8_t)page;
    seq.playing=(uint8_t)playing;seq.song_on=0;any_button=1;
    eng_step[TRK_DRUM]=eng_step[TRK_DRUM+1]=eng_step[TRK_BASS0]=eng_step[TRK_BASS0+1]=1;
    build_pages();memset(blit_hash,0,sizeof blit_hash);
    draw_header();draw_main();draw_knobs();
    int n=snprintf(path,sizeof path,"%s/%s-%s.png",capture_dir,capture_theme,name);
    if(n<0 || (size_t)n>=sizeof path)exit(1);
    png_write(path);
}
int main(int argc,char **argv)
{
    if(argc!=2){fprintf(stderr,"usage: ui_picture_capture OUTDIR\n");return 2;}
    capture_dir=argv[1];load_force=0;boot();now_ms=1000;
    proj.set.autosave_off=1;
    if(engine_bass_select_preset(0,ORIGINAL_SOUND_MIAMI,0,&proj.sound) ||
       engine_bass_select_preset(1,ORIGINAL_SOUND_CHICAGO,0,&proj.sound))return 1;
    for(int i=0;i<12;i++){int32_t audio[512];engine_render(audio,256);}
    for(int p=NPAT-2;p<NPAT;p++)pattern_init(&proj.pat[p],1,2);
    for(int p=0;p<NPARTS;p++){seq.ppat[p]=NPAT-1;seq.pcue[p]=NPAT-2;}
    sm_set(proj.pat[NPAT-1].drum[0].hit[0],0);sm_set(proj.pat[NPAT-1].drum[0].hit[0],8);
    sm_set(proj.pat[NPAT-2].drum[0].hit[0],2);seq.chain_a=seq.chain_b=0;
    for(int theme=0;theme<=5;theme+=5){
        palette_set(theme);capture_theme=theme?"light":"dark";
        capture_picture("home-stopped-queued",V_HOME,PART_909,0,0);
        capture_picture("chicago-oscillator-playing-queued",V_PART,PART_303B,0,1);
        capture_picture("miami-carrier-stopped-queued",V_PART,PART_303A,0,0);
        capture_picture("miami-xmod-playing",V_PART,PART_303A,1,1);
        capture_picture("miami-wobble-playing",V_PART,PART_303A,3,1);
        capture_picture("chicago-envelope-playing",V_PART,PART_303B,3,1);
        ui.view=V_SEQ;ui.part=PART_303B;build_pages();
        int page=-1,slot=-1;
        for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_BDIR){page=p;slot=k;}
        if(page<0)return 1;
        for(int direction=0;direction<4;direction++){
            char name[40];pref_set(pg.r[page][slot],direction);
            snprintf(name,sizeof name,"sequencer-direction-%d",direction);
            capture_picture(name,V_SEQ,PART_303B,page,0);
        }
    }
    return 0;
}
