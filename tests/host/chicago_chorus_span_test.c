/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_chorus_span.h"
#include <stdio.h>
#include <stdlib.h>
static uint32_t seed=0x6d70;
static unsigned checks;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static int32_t si(uint32_t u){return u<=INT32_MAX?(int32_t)u:-1-(int32_t)(UINT32_MAX-u);}
static int32_t literal(int32_t p,int32_t speed,int32_t y,uint32_t n){
 for(uint32_t i=0;i<n;i++){
  p=si((uint32_t)p+(uint32_t)speed);
  uint32_t mag=p<0?0u-(uint32_t)p:(uint32_t)p;
  int32_t z=y*8160+(int32_t)(mag>>11);
  y=z< -268427264?-32767:z>=268435456?32767:(z>=0?z/8192:-1-(-1-z)/8192);
 }return y;
}
static void compare(int32_t p,int32_t speed,int32_t y,uint32_t n){
 int32_t ref=literal(p,speed,y,n),got=original_chicago_chorus_span(p,speed,y,n);
 if(ref!=got){fprintf(stderr,"FAIL phase %d speed %d y %d n %u: %d != %d\n",p,speed,y,n,ref,got);exit(1);}++checks;
}
int main(void){
 const int32_t phases[]={INT32_MIN,INT32_MIN+1,-208732160,-1,0,1,208732160,INT32_MAX-1,INT32_MAX};
 const int32_t speeds[]={INT32_MIN,INT32_MIN+1,-65536,-1,0,1,32768,65536,INT32_MAX};
 const int32_t values[]={-32767,-1,0,1,32766,32767};
 for(unsigned p=0;p<9;p++)for(unsigned s=0;s<9;s++)for(unsigned v=0;v<6;v++)for(unsigned n=0;n<=256;n++)compare(phases[p],speeds[s],values[v],n);
 for(unsigned i=0;i<200000;i++){
  int32_t speed=i&1?si(random32()):(int32_t)(random32()%1048576u);
  compare(si(random32()),speed,(int32_t)(random32()%65535u)-32767,random32()%4097u);
 }
 printf("Chicago chorus span: %u exact comparisons PASS\n",checks);return 0;
}
