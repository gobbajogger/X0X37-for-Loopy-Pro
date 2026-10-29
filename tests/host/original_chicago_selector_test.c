/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_selector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned release,spawn;int32_t note;uint32_t velocity;chicago_arp_control *keys;original_chicago_gate *gate;} capture;
static void release(void *p){capture *c=p;c->release++;}
static void spawn(void *p,int32_t n,uint32_t v){capture *c=p;assert(c->release==1);c->spawn++;c->note=n;c->velocity=v;assert(c->keys->current_note==n);}
static uint32_t velocity(unsigned c,unsigned k,unsigned lat){
 unsigned pattern=(c+lat)%7;
 int active=(pattern==1&&k==0)||(pattern==2&&k==127)||(pattern==3&&(k==48||k==52||k==55))||pattern==4||(pattern==5&&(k+c)%13==0)||(pattern==6&&(k*7+c)%17==0);
 return active?(k*37+c*13+lat*7)%127+1:0;
}
int main(int argc,char **argv){
 assert(argc==3);int32_t in[15],expected[11];unsigned rows=0;
 for(unsigned file=1;file<=2;file++){FILE *f=fopen(argv[file],"rb");assert(f);size_t fields=file==1?13:15;
 while(fread(in,fields*sizeof *in,1,f)==1){
  assert(fread(expected,sizeof expected,1,f)==1);chicago_arp_control keys;memset(&keys,0,sizeof keys);
  keys.committed_enable=1;keys.effective_latch=(uint32_t)in[3];keys.current_note=in[12];
  for(unsigned i=0;i<128;i++){keys.physical[i]=velocity((unsigned)in[0],i,0);keys.latched[i]=velocity((unsigned)in[0],i,1);keys.physical_count+=(keys.physical[i]!=0);keys.latched_count+=(keys.latched[i]!=0);}
  if(file==2){keys.physical_count=(uint32_t)in[13];keys.latched_count=(uint32_t)in[14];}
  original_chicago_selector s={in[4],in[5],in[6],in[7],(uint16_t)in[8]};original_chicago_gate g={in[9],in[10],60};
  original_chicago_selector_controls c={(uint32_t)in[1],(uint32_t)in[2],(uint32_t)in[11]};capture got={0,0,0,0,&keys,&g};original_chicago_selector_callbacks cb={release,spawn};
  assert(original_chicago_selector_step(&s,&keys,&g,&c,&cb,&got));
  int32_t actual[]={s.cursor,s.last_key,s.octave,s.direction,s.random,keys.current_note,g.remaining,(int32_t)got.release,(int32_t)got.spawn,got.note,(int32_t)got.velocity};
  if(memcmp(actual,expected,sizeof expected)){
   for(unsigned i=0;i<11;i++)if(actual[i]!=expected[i])fprintf(stderr,"case%u mode%d latch%d range%d cursor%d oldoct%d direction%d: field%u actual%d expected%d\n",rows,in[1],in[3],in[2],in[4],in[6],in[7],i,actual[i],expected[i]);
   assert(0);
  }
  rows++;
 }
 assert(!ferror(f));fclose(f);}
 original_chicago_gate g={17,2,60};original_chicago_gate_tick(&g,187);assert(g.remaining==1&&g.last_note==60);original_chicago_gate_tick(&g,187);assert(g.remaining==0&&g.last_note==187);original_chicago_gate_tick(&g,0);assert(g.remaining==0&&g.last_note==187);
 chicago_arp_control keys;memset(&keys,0,sizeof keys);original_chicago_selector s={-1,60,1,0,0x6d70},saved=s;original_chicago_selector_controls c={0,1,1};capture got={0,0,0,0,&keys,&g};original_chicago_selector_callbacks cb={release,spawn};
 assert(original_chicago_selector_step(&s,&keys,&g,&c,&cb,&got)&&!memcmp(&s,&saved,sizeof s)&&!got.release&&!got.spawn);
 keys.committed_enable=1;keys.physical_count=1;keys.latched_count=1;
 for(unsigned latch=0;latch<2;latch++)for(unsigned range=1;range<=6;range++)for(unsigned mode=1;mode<=2;mode++){keys.effective_latch=latch;c.mode=mode;c.range=range;assert(original_chicago_selector_step(&s,&keys,&g,&c,&cb,&got)==-1);assert(!memcmp(&s,&saved,sizeof s)&&!got.release&&!got.spawn);}
 printf("PASS %u actual8ff4 static selection/gate/callback cases, all4modes, physical/latched, range1..6, note0..187, RNG/count mismatches;24 source-proven nontermination states explicitly rejected; producer proof PENDING_REVIEW\n",rows);
 return 0;
}
