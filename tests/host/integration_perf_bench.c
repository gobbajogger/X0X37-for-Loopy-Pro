/* SPDX-License-Identifier: GPL-3.0-only */
/* Matched host CPU/wall measurements; not a device-cycle or dropout test. */
#define AUDIO_PATH_IDENTITY_LIBRARY
#include "audio_path_identity.c"
#include <time.h>
static double cpu_ns(void) {struct timespec t;assert(!clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t));return (double)t.tv_sec*1e9+t.tv_nsec;}
static int find_param(const char *name) {
 for(int i=0;i<engine_nparams(T_303,0);i++)if(!strcmp(engine_param(T_303,0,i)->name,name))return i;
 return -1;
}
static void clear_notes(void) {
 pattern_t *p=&proj.pat[0];
 for(int k=0;k<NKIT;k++)memset(p->drum[k].hit,0,sizeof p->drum[k].hit);
 for(int b=0;b<NBASS;b++)for(int s=0;s<NSTEPS;s++)p->bass[b].step[s].flags=G_REST;
 p->brk.steps=0;
}
static void drum_pattern(int k,int voice,int stride) {
 dpart_t *d=&proj.pat[0].drum[k];d->len=16;
 for(int s=0;s<16;s+=stride)sm_set(d->hit[voice],s);
}
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 int c=atoi(argv[1]);if(c<0||c>=30)return 2;
 boot();engine_guard_enable(0);master=4096;engine_usb_full(1);
 assert(load_slot(0,0,argv[2])==0);engine_brk_loops();clear_notes();
 int bank0=FACTORY_BANK_CHICAGO,bank1=FACTORY_BANK_MIAMI,preset0=0,preset1=0;
 if(c==0||c==7)bank1=FACTORY_BANK_CHICAGO;
 if(c==2||c==5||c==8)bank0=FACTORY_BANK_MIAMI;
 if(c==4)bank1=FACTORY_BANK_CHICAGO;
 if(c==10)preset0=20;
 if(c==11){bank0=FACTORY_BANK_MIAMI;preset0=10;}
 assert(engine_bass_select_preset(0,bank0,preset0,&proj.sound)>=0);
 assert(engine_bass_select_preset(1,bank1,preset1,&proj.sound)>=0);
 int32_t lr[512];
 for(int k=0;k<64;k++)engine_render(lr,256);
 if(c>=3&&c<=13) {
  if(c!=4&&c!=6)engine_bass_on(0,48,0,0);
  if(c==4||c==6||c==7||c==8||c==9)engine_bass_on(1,60,0,0);
  if(c==12){int p=find_param("ArpOn");assert(p>=0);engine_set(T_303,0,p,127);}
 }
 if(c==14)drum_pattern(0,0,4);
 if(c==15)drum_pattern(0,0,1);
 if(c==16)drum_pattern(0,1,1);
 if(c==17){drum_pattern(0,0,4);drum_pattern(0,7,1);}
 if(c==18||c==23)for(int v=0;v<NDRUM;v++)drum_pattern(0,v,1);
 if(c==19)drum_pattern(1,0,4);
 if(c==20){drum_pattern(1,1,4);drum_pattern(1,7,1);}
 if(c==21||c==23)for(int v=0;v<NDRUM;v++)drum_pattern(1,v,1);
 if(c==22){proj.pat[0].brk.steps=0xffff;proj.pat[0].brk.slot_a=proj.pat[0].brk.slot_b=0;engine_brk_loops();}
 if(c>=14&&c<=23)seq_start(&seq);
 if(c==24||c==25){engine_bass_on(0,48,0,0);engine_bass_on(1,60,0,0);for(int b=0;b<NBASS;b++)engine_set(T_MIX,PART_303A+b,c==24?MX_REV:MX_DLY,100);}
 if(c>=26){fill_pattern(1);engine_brk_loops();seq_start(&seq);}
 if(c==27)engine_set(T_MST,0,MST_COMP1,80);
 for(int k=0;k<32;k++)engine_render(lr,256);
 if(c==13){engine_bass_off(0);engine_bass_off(1);}
 double w0=now_ns(),p0=cpu_ns();
 for(int k=0;k<128;k++) {
  now_ms=(uint32_t)((uint64_t)k*256*1000/44100);
  if(c==28&&!(k&7))for(int p=0;p<NPARTS;p++){engine_set(T_MIX,p,MX_PAN,(k*3+p*17)%128);engine_set(T_MIX,p,MX_LEVEL,(k+p*13)%128);}
  engine_render(lr,256);engine_volume(lr,256);
  if(c==29&&!(k&3))ui_frame();
 }
 double p=cpu_ns()-p0,w=now_ns()-w0;
 printf("{\"case\":%d,\"frames\":32768,\"cpu_ns\":%.0f,\"wall_ns\":%.0f}\n",c,p,w);
 return 0;
}
