/* SPDX-License-Identifier: GPL-3.0-only */
/* Timing only: reuse the pinned comparison's API declarations and state type. */
#define _GNU_SOURCE
#define main juno6_unused_equivalence_main
#include "juno6_exact_test.c"
#undef main
#include <sched.h>
#include <sys/resource.h>

enum { BLOCKS = 16, QUARTETS = 16, TRIALS = 15 };
typedef struct {
 uint64_t cpu,wall;
 long involuntary,voluntary;
 int cpu_before,cpu_after;
 uint32_t checksum;
} leg_t;

static leg_t leg(render_fn render,chorus_fn chorus,const state_t *seed) {
 state_t s=*seed;float left[256],right[256];struct rusage before,after;
 leg_t result;result.cpu_before=sched_getcpu();
 if(getrusage(RUSAGE_THREAD,&before))exit(2);
 uint64_t wall=now_ns(CLOCK_MONOTONIC),cpu=now_ns(CLOCK_THREAD_CPUTIME_ID);
 for(unsigned block=0;block<BLOCKS;block++){
  render(&s.synth,left,256);memcpy(right,left,sizeof right);
  chorus(&s.chorus,left,right,256);
 }
 result.cpu=now_ns(CLOCK_THREAD_CPUTIME_ID)-cpu;
 result.wall=now_ns(CLOCK_MONOTONIC)-wall;
 if(getrusage(RUSAGE_THREAD,&after))exit(2);
 result.cpu_after=sched_getcpu();
 result.involuntary=after.ru_nivcsw-before.ru_nivcsw;
 result.voluntary=after.ru_nvcsw-before.ru_nvcsw;
 /* Outside timing: require identical work and make its results observable. */
 const unsigned char *bytes=(const unsigned char *)&s;
 uint32_t hash=2166136261u;
 for(size_t i=0;i<sizeof s;i++)hash=(hash^bytes[i])*16777619u;
 bytes=(const unsigned char *)left;
 for(size_t i=0;i<sizeof left;i++)hash=(hash^bytes[i])*16777619u;
 bytes=(const unsigned char *)right;
 for(size_t i=0;i<sizeof right;i++)hash=(hash^bytes[i])*16777619u;
 result.checksum=hash;consume+=left[255]+right[255];return result;
}

static void quartet(const char *name,unsigned trial,unsigned q,int control,
                    int reverse,const state_t *seed) {
 leg_t runs[4];
 for(unsigned i=0;i<4;i++){
  int is_b=(i==1||i==2)^reverse;
  int candidate=is_b&&!control;
  runs[i]=leg(candidate?juno6_render:parent_juno6_render,
              candidate?juno6_chorus_process:parent_juno6_chorus_process,seed);
  if(runs[i].cpu_before!=runs[i].cpu_after)exit(3);
  if(i&&runs[i].checksum!=runs[0].checksum)exit(4);
 }
 printf("{\"kind\":\"quartet\",\"case\":\"%s\",\"trial\":%u,\"quartet\":%u,\"control\":%s,\"order\":\"%s\",\"frames_per_leg\":4096,\"cpu_ns\":[%llu,%llu,%llu,%llu],\"wall_ns\":[%llu,%llu,%llu,%llu],\"involuntary\":[%ld,%ld,%ld,%ld],\"voluntary\":[%ld,%ld,%ld,%ld],\"cpus\":[%d,%d,%d,%d],\"checksum\":%u}\n",
  name,trial,q,control?"true":"false",reverse?"BAAB":"ABBA",
  (unsigned long long)runs[0].cpu,(unsigned long long)runs[1].cpu,
  (unsigned long long)runs[2].cpu,(unsigned long long)runs[3].cpu,
  (unsigned long long)runs[0].wall,(unsigned long long)runs[1].wall,
  (unsigned long long)runs[2].wall,(unsigned long long)runs[3].wall,
  runs[0].involuntary,runs[1].involuntary,runs[2].involuntary,runs[3].involuntary,
  runs[0].voluntary,runs[1].voluntary,runs[2].voluntary,runs[3].voluntary,
  runs[0].cpu_before,runs[1].cpu_before,runs[2].cpu_before,runs[3].cpu_before,
  runs[0].checksum);
}

int main(void) {
 const unsigned voices[]={0,1,6,1,6};
 const char *names[]={"idle","held-1","held-6","release-1","release-6"};
 state_t seeds[5];
 for(unsigned c=0;c<5;c++){
  memset(&seeds[c],0,sizeof seeds[c]);
  parent_juno6_init(&seeds[c].synth,44100);
  parent_juno6_chorus_init(&seeds[c].chorus,44100);
  parent_juno6_set(&seeds[c].synth,8,1);
  for(unsigned i=0;i<voices[c];i++){
   parent_juno6_note_on(&seeds[c].synth,(uint8_t)(48+3*i),100);
   seeds[c].synth.voice[i].envelope=.8f;
   seeds[c].synth.voice[i].stage=c>=3?4:3;
   seeds[c].synth.voice[i].gate=c>=3?0:1;
  }
  for(unsigned warm=0;warm<8;warm++){
   (void)leg(parent_juno6_render,parent_juno6_chorus_process,&seeds[c]);
   (void)leg(juno6_render,juno6_chorus_process,&seeds[c]);
  }
 }
 /* Rotate case order; alternate ABBA/BAAB and control/comparison first. */
 for(unsigned trial=0;trial<TRIALS;trial++)for(unsigned step=0;step<5;step++){
  unsigned c=(step+trial)%5;
  for(unsigned q=0;q<QUARTETS;q++){
   int reverse=(trial+q)&1,first=(trial+q)&1;
   quartet(names[c],trial,q,first,reverse,&seeds[c]);
   quartet(names[c],trial,q,!first,reverse,&seeds[c]);
  }
  fflush(stdout);
 }
 return 0;
}
