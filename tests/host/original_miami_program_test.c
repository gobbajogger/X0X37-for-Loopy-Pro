/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_program.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
int main(int argc,char **argv) {
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
 unsigned compared=0;
 for(unsigned factory=0;factory<30;factory++) {
  original_miami_program p;assert(original_miami_program_init(&p,factory));
  assert(p.revision==0&&original_miami_program_name(factory));
  for(unsigned id=0;id<109;id++) {
   uint32_t expected;assert(fread(&expected,4,1,f)==1);float actual;
   assert(original_miami_program_native(&p,id,&actual));
   if(bits(actual)!=expected) {
    fprintf(stderr,"factory %u (%s), native %u: %08x != %08x\n",factory,original_miami_program_name(factory),id,bits(actual),expected);
    return 1;
   }
   compared++;
  }
  /* A held note owns the exact old snapshot through subsequent control edits.
   * No UI byte conversion or shared global factory scratch can alter it. */
  original_miami_program held=p,saved=p;
  for(unsigned id=0;id<18;id++)assert(original_miami_program_set(&p,id,1-p.macro[id]));
  assert(!memcmp(&held,&saved,sizeof held));assert(p.revision==18);
  saved=p;float sentinel=123;
  assert(!original_miami_program_set(&p,18,0));
  assert(!original_miami_program_set(&p,0,NAN));
  assert(!original_miami_program_set(&p,0,INFINITY));
  assert(!original_miami_program_set(&p,0,-.01f));
  assert(!original_miami_program_set(&p,0,1.01f));
  assert(!original_miami_program_init(&p,30));
  assert(!memcmp(&p,&saved,sizeof p));
  assert(!original_miami_program_native(&p,109,&sentinel)&&sentinel==123);
  assert(!original_miami_program_native(&p,112,&sentinel)&&sentinel==123);
 }
 assert(fgetc(f)==EOF);fclose(f);
 printf("PASS %u exact original native fields, all30 complete programs, all18 snapshot edits, invalid transactional boundaries; snapshot%zu bytes\n",compared,sizeof(original_miami_program));
 return 0;
}
