/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact differential checks against the separately compiled unchanged decoder.
 * The private payload is consumed locally; this test emits no table bytes. */
#define _POSIX_C_SOURCE 200809L
#include "lossless_table.h"
#include "chicago_packed_private.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int baseline_lossless_second_lookup(const original_lossless_table *,uint32_t,uint32_t *);
int baseline_lossless_second16_lookup(const original_lossless_table16 *,uint32_t,uint32_t *);
static uint32_t rng=UINT32_C(0x147b02e9);
static uint32_t next_random(void){rng=rng*1664525u+1013904223u;return rng;}
static void put32(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));}
static uint64_t generic_checks,negative_checks,authentic_checks;

static void exact(const original_lossless_table *t,const original_lossless_table16 *t16,uint32_t index,uint32_t expected){
 uint32_t a=0,b=0,c=0,d=0;
 assert(original_lossless_second_lookup(t,index,&a));
 assert(baseline_lossless_second_lookup(t,index,&b));
 assert(original_lossless_second16_lookup(t16,index,&c));
 assert(baseline_lossless_second16_lookup(t16,index,&d));
 assert(a==expected&&b==expected&&c==expected&&d==expected);
}
static void refused(const original_lossless_table *t,const original_lossless_table16 *t16,uint32_t index){
 uint32_t a=UINT32_C(0xa5a51357),b=a,c=a,d=a;
 assert(!original_lossless_second_lookup(t,index,&a));
 assert(!baseline_lossless_second_lookup(t,index,&b));
 assert(!original_lossless_second16_lookup(t16,index,&c));
 assert(!baseline_lossless_second16_lookup(t16,index,&d));
 assert(a==UINT32_C(0xa5a51357)&&b==a&&c==a&&d==a);negative_checks++;
}
static void generic(void){
 for(unsigned width=0;width<=32;width++)for(unsigned count=1;count<=32;count++)for(unsigned trial=0;trial<64;trial++){
  uint32_t expected[32],offset=0,delta=next_random();uint16_t offset16=0;
  size_t residuals=count>1?count-2:0,nbytes=9+(residuals*width+7)/8;
  /* Exact-sized allocations make any payload overread visible to ASan. */
  uint8_t *data=calloc(nbytes,1);assert(data);expected[0]=next_random();
  put32(data,expected[0]);put32(data+4,delta);data[8]=(uint8_t)width;
  size_t bit=0;if(count>1)expected[1]=expected[0]+delta;
  for(unsigned i=2;i<count;i++){
   uint32_t z=next_random();if(width<32)z&=(1u<<width)-1u;
   for(unsigned k=0;k<width;k++,bit++)data[9+bit/8]|=(uint8_t)(((z>>k)&1u)<<(bit%8));
   delta+=(z>>1)^(0u-(z&1u));expected[i]=expected[i-1]+delta;
  }
  original_lossless_table t={data,&offset,nbytes,count,1};
  original_lossless_table16 t16={data,&offset16,nbytes,count,1};
  for(unsigned i=0;i<count;i++){exact(&t,&t16,i,expected[i]);generic_checks++;}
  refused(&t,&t16,count);
  t.bytes=t16.bytes=nbytes-1;refused(&t,&t16,0);t.bytes=t16.bytes=nbytes;
  data[8]=33;refused(&t,&t16,0);data[8]=(uint8_t)width;
  offset=offset16=1;refused(&t,&t16,0);offset=offset16=0;
  t.blocks=t16.blocks=2;refused(&t,&t16,0);
  free(data);
 }
 uint8_t data[18]={0};uint32_t offsets[2]={0,9},sentinel=42;uint16_t offsets16[2]={0,9};
 put32(data,UINT32_C(0xffffffff));put32(data+4,UINT32_C(0x80000000));put32(data+9,UINT32_C(0x80000001));
 original_lossless_table t={data,offsets,sizeof data,33,2};
 original_lossless_table16 t16={data,offsets16,sizeof data,33,2};
 exact(&t,&t16,31,UINT32_C(0x7fffffff));exact(&t,&t16,32,UINT32_C(0x80000001));
 offsets[1]=offsets16[1]=10;refused(&t,&t16,0);offsets[1]=offsets16[1]=9;
 t.bytes=t16.bytes=8;refused(&t,&t16,0);t.bytes=t16.bytes=18;
 t.count=t16.count=0;refused(&t,&t16,0);t.count=t16.count=33;
 refused(NULL,NULL,0);
 assert(!original_lossless_second_lookup(&t,0,NULL)&&!baseline_lossless_second_lookup(&t,0,NULL));
 assert(!original_lossless_second16_lookup(&t16,0,NULL)&&!baseline_lossless_second16_lookup(&t16,0,NULL));
 t.data=t16.data=NULL;refused(&t,&t16,0);t.data=t16.data=data;
 t.offset=NULL;t16.offset=NULL;refused(&t,&t16,0);t.offset=offsets;t16.offset=offsets16;
 t16.bytes=(size_t)UINT16_MAX+1u;
 assert(!original_lossless_second16_lookup(&t16,0,&sentinel)&&sentinel==42);
 assert(!baseline_lossless_second16_lookup(&t16,0,&sentinel)&&sentinel==42);
}
static void authentic(void){
 const original_lossless_table16 *tables[2]={&original_chicago_packed_a,&original_chicago_packed_e};
 uint64_t hash=UINT64_C(14695981039346656037);
 for(unsigned j=0;j<2;j++){
  const original_lossless_table16 *t16=tables[j];uint32_t offsets[513];
  assert(t16->blocks==513);for(unsigned i=0;i<t16->blocks;i++)offsets[i]=t16->offset[i];
  const original_lossless_table t={t16->data,offsets,t16->bytes,t16->count,t16->blocks};
  for(uint32_t i=0;i<t16->count;i++){
   uint32_t expected;assert(baseline_lossless_second16_lookup(t16,i,&expected));
   exact(&t,t16,i,expected);authentic_checks++;
   for(unsigned byte=0;byte<4;byte++){hash^=(expected>>(byte*8))&255u;hash*=UINT64_C(1099511628211);}
  }
 }
 printf("PASS authentic=%" PRIu64 " complete coefficient words, fnv64=%016" PRIx64 "\n",authentic_checks,hash);
}

typedef int (*lookup_fn)(const original_lossless_table16 *,uint32_t,uint32_t *);
typedef struct{const original_lossless_table16 *table;uint32_t index;} query;
static volatile uint32_t timed_sink;
static uint64_t now_ns(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000u+(uint64_t)t.tv_nsec;}
static uint64_t timed(lookup_fn lookup,const query *queries,unsigned count,unsigned repeats){
 uint32_t checksum=0;uint64_t start=now_ns();
 for(unsigned r=0;r<repeats;r++)for(unsigned i=0;i<count;i++){
  uint32_t value;if(!lookup(queries[i].table,queries[i].index,&value))abort();checksum+=value;
 }
 uint64_t end=now_ns();timed_sink=checksum;return end-start;
}
static void campaign(const char *name,const query *queries,unsigned count){
 unsigned repeats=1000000u/count;if(!repeats)repeats=1;
 (void)timed(baseline_lossless_second16_lookup,queries,count,1);(void)timed(original_lossless_second16_lookup,queries,count,1);
 for(unsigned round=0;round<11;round++){
  uint64_t baseline,candidate;
  if(round&1){candidate=timed(original_lossless_second16_lookup,queries,count,repeats);baseline=timed(baseline_lossless_second16_lookup,queries,count,repeats);}
  else {baseline=timed(baseline_lossless_second16_lookup,queries,count,repeats);candidate=timed(original_lossless_second16_lookup,queries,count,repeats);}
  printf("TIMING %s %u %u %" PRIu64 " %" PRIu64 "\n",name,round,count*repeats,baseline,candidate);
 }
}
static void benchmark(void){
 const original_lossless_table16 *tables[2]={&original_chicago_packed_a,&original_chicago_packed_e};
 query *queries=malloc(32770u*sizeof *queries);assert(queries);
 unsigned n=0;for(unsigned table=0;table<2;table++)for(uint32_t i=0;i<tables[table]->count;i++)queries[n++]=(query){tables[table],i};
 campaign("authentic-sequential",queries,n);
 for(unsigned i=n-1;i;i--){unsigned j=next_random()%(i+1);query tmp=queries[i];queries[i]=queries[j];queries[j]=tmp;}
 campaign("authentic-shuffled",queries,n);
 n=0;for(unsigned table=0;table<2;table++)for(unsigned block=0;block<512;block++)queries[n++]=(query){tables[table],block*32+31};
 campaign("authentic-entry31",queries,n);
 unsigned widest=0,widest_index=0;
 for(unsigned block=0;block<512;block++){unsigned w=original_chicago_packed_e.data[original_chicago_packed_e.offset[block]+8];if(w>widest){widest=w;widest_index=block*32+31;}}
 assert(widest==23);query worst={&original_chicago_packed_e,widest_index};campaign("authentic-width23-entry31",&worst,1);
 uint8_t data[129]={0};uint16_t offset=0;put32(data,UINT32_C(0x7fffffff));put32(data+4,UINT32_C(0x80000000));
 for(unsigned i=9;i<129;i++)data[i]=(uint8_t)next_random();
 for(unsigned width=24;width<=32;width+=width==24?1:7){
  data[8]=(uint8_t)width;original_lossless_table16 t={data,&offset,9+(30u*width+7)/8,32,1};query q={&t,31};char name[40];
  assert(snprintf(name,sizeof name,"generic-width%u-entry31",width)>0);campaign(name,&q,1);
 }
 free(queries);
}
int main(int argc,char **argv){
 if(argc==2&&!strcmp(argv[1],"--benchmark")){benchmark();return 0;}
 assert(argc==1);generic();authentic();
 printf("PASS generic=%" PRIu64 " words across widths0..32/counts1..32, negatives=%" PRIu64 ", both descriptor widths\n",generic_checks,negative_checks);
 return 0;
}
