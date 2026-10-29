/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_decimator_adapter.h"
#include "miami_decimator_tables_private.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
static void fill(int16_t *l,int16_t *r,unsigned n){for(unsigned i=0;i<n;i++){l[i]=(int16_t)((int)i*31-16000);r[i]=(int16_t)(15000-(int)i*17);}}
int main(int argc,char **argv){
 (void)argv;
 if(argc>1){
  miami_decimator_adapter_t s;assert(miami_decimator_adapter_init(&s));int16_t l[128],r[128];volatile int64_t sum=0;clock_t t=clock();
  for(unsigned i=0;i<78125;i++){fill(l,r,128);assert(miami_decimator_adapter_process(&s,l,r,128,NULL,0));sum+=l[0];}
  printf("adapter_state=%zu frames=10000000 block=128 cpu_seconds_with_input_fill=%.9f checksum=%" PRId64 "\n",sizeof s,(double)(clock()-t)/CLOCKS_PER_SEC,sum);return 0;
 }

 miami_decimator_adapter_t s,before;memset(&s,0,sizeof s);assert(miami_decimator_adapter_init(&s));
 assert(s.kernel.phase==0&&s.kernel.held_left==0&&s.kernel.held_right==0);
 assert(s.kernel.step==0&&s.kernel.pending_step==6041&&s.kernel.pending==1&&s.wet_gain==32767);
 assert(s.frequency_integer==64&&s.wet_integer==127&&s.bit_integer==0);
 before=s;assert(miami_decimator_adapter_process(&s,NULL,NULL,0,NULL,0));assert(!memcmp(&before,&s,sizeof s));
 int16_t l[257],r[257];fill(l,r,257);assert(miami_decimator_adapter_process(&s,l,r,1,NULL,0));assert(s.kernel.step==6041&&s.kernel.phase==6041&&!s.kernel.pending&&l[0]==0&&r[0]==0);
 for(int p=0;p<128;p++){
  uint16_t active=s.kernel.step,phase=s.kernel.phase;int16_t hl=s.kernel.held_left,hr=s.kernel.held_right;
  assert(miami_decimator_adapter_set(&s,1,p));assert(s.frequency_integer==p&&s.kernel.pending_step==miami_decimator_frequency[127-p]&&s.kernel.pending&&s.kernel.step==active);
  before=s;assert(miami_decimator_adapter_set(&s,2,p));assert(s.wet_integer==p&&s.wet_gain==miami_decimator_wet[p]);assert(!memcmp(&s.kernel,&before.kernel,sizeof s.kernel));
  assert(miami_decimator_adapter_set(&s,3,p));assert(s.bit_integer==p&&s.kernel.pending_step==miami_decimator_frequency[127-p]);
  assert(s.kernel.phase==phase&&s.kernel.held_left==hl&&s.kernel.held_right==hr);
 }
 assert(miami_decimator_adapter_set(&s,1,-1)&&s.frequency_integer==127);assert(miami_decimator_adapter_set(&s,2,128)&&s.wet_integer==0);assert(miami_decimator_adapter_set(&s,3,INT32_MIN)&&s.bit_integer==0);
 before=s;assert(!miami_decimator_adapter_set(&s,4,0));assert(!memcmp(&before,&s,sizeof s));
 assert(!miami_decimator_adapter_set(NULL,1,0)&&!miami_decimator_adapter_init(NULL)&&!miami_decimator_adapter_reset(NULL));
 /* Actual call boundary restarts local latch0 even after a non64 prior length. */
 assert(miami_decimator_adapter_init(&s));fill(l,r,257);assert(miami_decimator_adapter_process(&s,l,r,17,NULL,0));
 assert(miami_decimator_adapter_set(&s,1,0));assert(s.kernel.step==6041&&s.kernel.pending_step==65535);
 uint16_t phase=s.kernel.phase;assert(miami_decimator_adapter_process(&s,l,r,1,NULL,0));assert(s.kernel.step==65535&&s.kernel.phase==(uint16_t)((uint32_t)phase+65535));
 /* Wet is immediate on next sample, not pending until64. */
 before=s;assert(miami_decimator_adapter_set(&s,2,0));assert(!memcmp(&before.kernel,&s.kernel,sizeof s.kernel));l[0]=32767;r[0]=-32768;
 assert(miami_decimator_adapter_process(&s,l,r,1,NULL,0));assert(l[0]==32766&&r[0]==-32767);
 /* Reset retains controls/dirty/active/pending, clears phase/holds only. */
 assert(miami_decimator_adapter_set(&s,1,30));before=s;assert(miami_decimator_adapter_reset(&s));
 before.kernel.phase=0;before.kernel.held_left=before.kernel.held_right=0;assert(!memcmp(&before,&s,sizeof s));
 /* Stable parameters: invocation partitions preserve phase/hold and audio. */
 int16_t bl[257],br[257];fill(bl,br,257);miami_decimator_adapter_t base;memset(&base,0,sizeof base);assert(miami_decimator_adapter_init(&base));assert(miami_decimator_adapter_set(&base,2,64));
 assert(miami_decimator_adapter_process(&base,bl,br,257,NULL,0));
 for(unsigned block=1;block<=256;block++){
  fill(l,r,257);memset(&s,0,sizeof s);assert(miami_decimator_adapter_init(&s));assert(miami_decimator_adapter_set(&s,2,64));
  for(unsigned pos=0;pos<257;pos+=block){unsigned n=257-pos;if(n>block)n=block;assert(miami_decimator_adapter_process(&s,l+pos,r+pos,n,NULL,0));}
  assert(!memcmp(bl,l,sizeof l)&&!memcmp(br,r,sizeof r));assert(!memcmp(&base,&s,sizeof s));
 }
 /* Bit setter restages frequency but selected processing applies no bitmask. */
 fill(l,r,257);memcpy(bl,l,sizeof l);memcpy(br,r,sizeof r);assert(miami_decimator_adapter_init(&s));base=s;
 assert(miami_decimator_adapter_set(&s,3,127));assert(miami_decimator_adapter_process(&s,l,r,257,NULL,0));
 assert(miami_decimator_adapter_process(&base,bl,br,257,NULL,0));assert(!memcmp(l,bl,sizeof l)&&!memcmp(r,br,sizeof r));
 /* Explicit reset events happen before current sample, without new global clock. */
 const uint32_t events[]={0,63,64,128,128,256};fill(l,r,257);memcpy(bl,l,sizeof l);memcpy(br,r,sizeof r);
 assert(miami_decimator_adapter_init(&s));base=s;
 assert(miami_decimator_adapter_process(&s,l,r,257,events,6));unsigned ev=0;
 for(unsigned i=0;i<257;i++){
  while(ev<6&&events[ev]==i){assert(miami_decimator_adapter_reset(&base));ev++;}
  int16_t o[2];assert(miami_decimator_frame(&base.kernel,i,bl[i],br[i],base.wet_gain,o));bl[i]=o[0];br[i]=o[1];
 }
 assert(!memcmp(l,bl,sizeof l)&&!memcmp(r,br,sizeof r)&&!memcmp(&s,&base,sizeof s));
 uint32_t bad[]={3,257};before=s;fill(l,r,257);memcpy(bl,l,sizeof l);memcpy(br,r,sizeof r);
 assert(!miami_decimator_adapter_process(&s,l,r,257,bad,2));assert(!memcmp(&s,&before,sizeof s)&&!memcmp(l,bl,sizeof l)&&!memcmp(r,br,sizeof r));
 bad[0]=4;bad[1]=3;assert(!miami_decimator_adapter_process(&s,l,r,257,bad,2));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_adapter_process(&s,l,r,0,bad,1));assert(!memcmp(&s,&before,sizeof s));
 assert(!miami_decimator_adapter_process(&s,l,l,257,NULL,0));assert(!miami_decimator_adapter_process(&s,NULL,r,1,NULL,0));
 assert(!miami_decimator_adapter_process(&s,l,r,257,NULL,1));assert(!memcmp(&s,&before,sizeof s));
 printf("PASS original-state projection,256coefflookups,integer masks,reset/localcall/wet timing,partitions1..256,events;sizeof=%zu\n",sizeof s);return 0;
}
