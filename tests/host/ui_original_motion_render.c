/* SPDX-License-Identifier: GPL-3.0-only */
/* Mock renderer (not a pass/fail test): real UI + engine framebuffer frames of
 * the live Chicago/Miami motion glyphs. Frames every 40 ms through run_ms (the
 * device UI tick is 16 ms). Writes $OUT/<name>-fNN.png and <name>.txt (labels). */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static const char *outdir;
static int find_page(pref_t r)
{
    for(int p=0;p<pg.n;p++) for(int k=0;k<4;k++){pref_t q=pg.r[p][k];
        if(q.kind==r.kind&&q.a==r.a&&q.b==r.b&&q.c==r.c) return p;}
    return -1;
}
static int raw_for_text(pref_t r,const char *want)
{
    const x0x_param_t *d=pref_desc(r); char num[24],unit[8];
    for(int v=0;v<=d->max;v++){num[0]=unit[0]=0;pref_value_of(r,v,num,unit);
        char t[40];snprintf(t,sizeof t,"%s%s",num,unit); if(!strcmp(t,want)) return v;}
    int best=-1,bd=999;                       /* else the nearest whole percent */
    for(int v=0;v<=d->max;v++){num[0]=unit[0]=0;pref_value_of(r,v,num,unit);
        int dd=abs(atoi(num)-atoi(want)); if(dd<bd){bd=dd;best=v;}}
    return best;
}
/* notes: on[i] = MIDI note to press at frame i (0 none); off frame releases all */
static void film(const char *name,pref_t r,int val,const int *on,int off,int frames)
{
    char path[1024],num[24]="",unit[8]="";
    pref_set(r,val); build_pages();
    int p=find_page(r); if(p<0) exit(1);
    ui.page[V_PART][ui.part]=(uint8_t)p; ui.touched=-1;
    engine_bass_off(r.b); run_ms(1500);
    pref_value_of(r,val,num,unit);
    snprintf(path,sizeof path,"%s/%s.txt",outdir,name); FILE *f=fopen(path,"w");
    { int k=0; for(;k<4;k++){pref_t q=pg.r[p][k]; if(q.kind==r.kind&&q.c==r.c) break;}
      fprintf(f,"#%d %d %d %d\n",4+k*59,20+4+(p%4)*50,59,49); }  /* the cell on screen */
    for(int i=0;i<frames;i++){
        if(on[i]) engine_bass_on_velocity(r.b,on[i],0,0,120);
        if(i==off) engine_bass_off(r.b);
        run_ms(40);
        snprintf(path,sizeof path,"%s/%s-f%02d.png",outdir,name,i); png_write(path);
        { float eg=0,dd=0; engine_bass_filter_view(r.b,&eg,&dd);
          pref_t cr=PR(R_ENG,T_303,r.b,2); float cq=(float)matrix_value(cr)/pref_desc(cr)->max;
          fprintf(f,"%s%s|%d|%s|eg %.3f|knee %d\n",num,unit,i*40,i<2?"idle":i<off?"note on":"released",eg,
                  r.c==4&&engine_bass_sound_bank(&proj.sound,r.b)==ORIGINAL_SOUND_CHICAGO?env_knee_x(0,cq+eg*dd):-1); }
    }
    fclose(f);
}
int main(void)
{
    outdir=getenv("MOTION_OUT"); if(!outdir) return 1;
    int frames=36, on[64]={0};
    /* Chicago B: ENV */
    setup(V_PART,PART_303B); ui_due=0; audio_due_ms=now_ms;
    CHECK(engine_bass_sound_bank(&proj.sound,1)==ORIGINAL_SOUND_CHICAGO);
    on[2]=48;
    const int pot[]={0,25,50,56,75,100};
    for(int pass=0;pass<2;pass++){
        if(pass==1){ /* demo shape: arp off, ENV amp mode, A~50 ms D~300 ms S~30% R~300 ms */
            pref_set(PR(R_ENG,T_303,1,2),(pref_desc(PR(R_ENG,T_303,1,2))->max+1)/2); pref_set(PR(R_ENG,T_303,1,3),90);
            pref_set(PR(R_ENG,T_303,1,13),0); pref_set(PR(R_ENG,T_303,1,11),0);
            pref_set(PR(R_ENG,T_303,1,7),getenv("QA")?atoi(getenv("QA")):64);
            pref_set(PR(R_ENG,T_303,1,8),getenv("QD")?atoi(getenv("QD")):31);
            pref_set(PR(R_ENG,T_303,1,9),getenv("QS")?atoi(getenv("QS")):38);
            pref_set(PR(R_ENG,T_303,1,10),getenv("QR")?atoi(getenv("QR")):46); }
        for(int k=0;k<6;k++){ pref_t r=PR(R_ENG,T_303,1,4); int mx=pref_desc(r)->max,v=pot[k]==50?raw_for_text(r,"0%"):(pot[k]*mx+50)/100;
            char n[48]; snprintf(n,sizeof n,pass?"chicago-env-pot%03d":"chicago-env-pot%03d-owner",pot[k]); film(n,r,v,on,16,frames); }
    }
    /* Chicago B: GLIDE between two notes */
    { int g[64]={0}; g[2]=36; g[9]=55; g[20]=40; pref_t r=PR(R_ENG,T_303,1,1);
      film("chicago-glide-050",r,raw_for_text(r,"50%")>=0?raw_for_text(r,"50%"):64,g,28,frames); }
    /* Miami on part A */
    setup(V_PART,PART_303A); ui_due=0; audio_due_ms=now_ms; /* setup rewinds now_ms */
    if(engine_bass_sound_bank(&proj.sound,0)!=ORIGINAL_SOUND_MIAMI)
        { CHECK(engine_bass_select_preset(0,ORIGINAL_SOUND_MIAMI,0,&proj.sound)==0); run_ms(500); }
    CHECK(engine_bass_sound_bank(&proj.sound,0)==ORIGINAL_SOUND_MIAMI);
    { int w[64]={0}; w[2]=36; film("miami-wobble-depth",PR(R_ENG,T_303,0,12),100,w,28,frames);
      film("miami-wobble-rate",PR(R_ENG,T_303,0,13),90,w,28,frames);
      int g[64]={0}; g[2]=36; g[9]=55; g[20]=40; film("miami-glide-050",PR(R_ENG,T_303,0,4),64,g,28,frames); }
    puts("MOTION RENDER DONE");
    return 0;
}
