/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-06/13/14: Loopy Bluetooth MIDI transport in one render block must
 * release old 303 notes before restarting, preserve Continue phase, and
 * leave no ghost NoteOn after Start/Continue followed by Stop.
 * This native regression links actual sequencer/pattern/TB3PO source. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

static unsigned checks, failed, frames, scenario;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failed; if (failed <= 30) \
  fprintf(stderr, "FAIL scenario=%u block=%u line=%d %s\n", scenario,frames,__LINE__,#expr); } } while(0)
typedef struct { unsigned on,off,duplicate,orphan,steps,transport,clock,break_stops,ev_count; char events[128]; uint8_t held[16][128]; int last_step; } monitor_t;
static pattern_t patterns[NPAT];
static void midi_cb(void *ctx,uint8_t st,uint8_t pitch,uint8_t vel){
 monitor_t *m=ctx; unsigned ch=st&15u, kind=st&0xf0u;
 if(st>=0xf8u){ if(st==0xf8u)m->clock++; else m->transport++; return; }
 if(kind==0x90u && vel){ if(m->ev_count < sizeof(m->events))m->events[m->ev_count++]='N'; if(m->held[ch][pitch])m->duplicate++;m->held[ch][pitch]=1;m->on++; }
 else if(kind==0x80u || (kind==0x90u && !vel)){if(m->ev_count < sizeof(m->events))m->events[m->ev_count++]='O'; if(!m->held[ch][pitch])m->orphan++;m->held[ch][pitch]=0;m->off++;}
}
static void break_stop_cb(void *ctx){monitor_t*m=ctx;if(m->ev_count < sizeof(m->events))m->events[m->ev_count++]='B';m->break_stops++;}
static void step_cb(void *ctx,int part,int step){monitor_t*m=ctx;if(part==TRK_BASS0){m->steps++;m->last_step=step;}}
static int drum_cb(void*x,int kit,int voice,float vel){(void)x;(void)kit;(void)voice;(void)vel;return 1;}
static int bass_on_cb(void*x,int part,int pitch,int a,int s,int vel){(void)x;(void)part;(void)pitch;(void)a;(void)s;(void)vel; return 1; }
static void bass_off_cb(void*x,int part){(void)x;(void)part;}
static unsigned held_count(monitor_t*m){unsigned n=0;for(unsigned ch=0;ch<16;ch++)for(unsigned p=0;p<128;p++)n+=m->held[ch][p];return n;}
static void render(seq_t*s,const seq_sink_t*out,uint32_t count){
 while(count){uint32_t n=count<frames?count:frames; while(n){uint32_t k=seq_until_event(s,n);CHECK(k>0&&k<=n);if(k==0||k>n)return;seq_advance(s,k,out);n-=k;}count-=count<frames?count:frames;}
}
static void recv(seq_t*s,uint8_t status){if(seq_clk_take(s,status,1u))seq_midi_rt(s,status);}
static void setup(seq_t*s,monitor_t*m,seq_sink_t*out,unsigned mode){
 for(unsigned p=0;p<NPAT;p++)pattern_init(&patterns[p],p+17u,p+39u);
 for(unsigned p=0;p<16u;p++)patterns[0].bass[0].step[p]=(bstep_t){(uint8_t)(60u+p%3u),G_NOTE|BS_SLIDE,110u};
 patterns[0].bass[0].len=16;
 seq_init(s,patterns);s->clk_src=(uint8_t)mode;s->send_notes=1;s->send_clock=0;
 memset(m,0,sizeof *m);memset(out,0,sizeof *out);
 out->ctx=m;out->brk_stop=break_stop_cb;out->midi=midi_cb;out->step=step_cb;out->bass_on=bass_on_cb;out->bass_off=bass_off_cb;out->drum=drum_cb;
 render(s,out,1);
}
static void start_one(seq_t*s,const seq_sink_t*out,monitor_t*m){
 recv(s,0xfau);recv(s,0xf8u);render(s,out,1);
 CHECK(s->playing&&s->ext);CHECK(m->steps==1&&m->last_step==0);CHECK(m->on==1&&m->off==0);CHECK(held_count(m)==1);CHECK(m->ev_count==1 && m->events[0]=='N');
}
static void stop_then_start(int mode){
 seq_t s;monitor_t m;seq_sink_t out;setup(&s,&m,&out,mode);start_one(&s,&out,&m);
 recv(&s,0xf8u); /* tick on old epoch */
 recv(&s,0xfcu); /* remote Stop ends the sounding 303 note */
 recv(&s,0xfau); /* remote Start later in the same Bluetooth MIDI event */
 recv(&s,0xf8u); /* exactly one F8 in the new epoch */
 render(&s,&out,1);
 CHECK(s.playing);CHECK(m.steps==2&&m.last_step==0);CHECK(m.on==2&&m.off==1);CHECK(held_count(&m)==1);
 CHECK(m.duplicate==0&&m.orphan==0);CHECK(m.clock==0&&m.transport==0);
 CHECK(m.break_stops==1);CHECK(m.ev_count>=4 && m.events[1]=='O' && m.events[2]=='B' && m.events[3]=='N');
 for(unsigned i=0;i<5;i++){recv(&s,0xf8u);render(&s,&out,1);CHECK(m.steps==2);}
 recv(&s,0xf8u);render(&s,&out,1);CHECK(m.steps==3&&m.last_step==1);
 recv(&s,0xfcu);render(&s,&out,1);recv(&s,0xfcu);render(&s,&out,1);
 CHECK(!s.playing);CHECK(m.on==m.off);CHECK(m.break_stops==2);CHECK(held_count(&m)==0);CHECK(!m.orphan&&!m.duplicate);
}
static void stop_then_continue(int mode){
 seq_t s;monitor_t m;seq_sink_t out;setup(&s,&m,&out,mode);start_one(&s,&out,&m);
 recv(&s,0xfcu);recv(&s,0xfbu);recv(&s,0xf8u);render(&s,&out,1);
 CHECK(s.playing);CHECK(m.steps==2&&m.last_step==1);CHECK(m.on==2&&m.off==1);
 CHECK(held_count(&m)==1);CHECK(!m.duplicate&&!m.orphan);CHECK(m.break_stops==1);CHECK(m.ev_count>=4 && m.events[1]=='O' && m.events[2]=='B' && m.events[3]=='N');
 recv(&s,0xfcu);render(&s,&out,1);
 CHECK(!s.playing&&m.on==m.off&&held_count(&m)==0);
}
static void start_then_stop(int mode){
 seq_t s;monitor_t m;seq_sink_t out;setup(&s,&m,&out,mode);start_one(&s,&out,&m);
 recv(&s,0xfau);recv(&s,0xf8u);recv(&s,0xfcu);recv(&s,0xf8u);render(&s,&out,1);
 CHECK(!s.playing);CHECK(m.break_stops==1);CHECK(m.steps==1);CHECK(m.on==1&&m.off==1);CHECK(held_count(&m)==0);
 recv(&s,0xfcu);render(&s,&out,1);
 CHECK(!s.playing&&m.off==1);CHECK(!m.duplicate&&!m.orphan);
}
int main(void){const unsigned blocks[]={1u,64u,1024u};for(unsigned b=0;b<3;b++){
 frames=blocks[b];for(unsigned mode=SEQ_CLK_AUTO;mode<=SEQ_CLK_EXT;mode+=2){
 scenario=1;stop_then_start((int)mode);scenario=2;stop_then_continue((int)mode);scenario=3;start_then_stop((int)mode);
 }}fprintf(stderr,"seq stop/restart burst: %u/%u checks %s\n",checks-failed,checks,failed?"FAILED":"PASSED");return failed?EXIT_FAILURE:EXIT_SUCCESS;}
