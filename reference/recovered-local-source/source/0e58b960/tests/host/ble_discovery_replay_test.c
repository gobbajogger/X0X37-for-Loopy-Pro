#ifndef TEST_LEGACY
#define TEST_LEGACY 0
#endif
#ifndef TEST_HANDLE_BASE
#define TEST_HANDLE_BASE 0x40u
#endif
/* SPDX-License-Identifier: GPL-3.0-only */
/* Production dual-peer HCI, ACL ownership and lifecycle; no physical device. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.c"
#if TEST_LEGACY
static void blec_pair_bind(blec_t*a,blec_t*b){(void)a;(void)b;}
static void blec_pair_rx(blec_t*a,blec_t*b,uint8_t t,const uint8_t*p,uint16_t n,uint32_t now){(void)b;blec_rx(a,t,p,n,now);}
static void blec_pair_poll(blec_t*a,blec_t*b,uint32_t now,int want){(void)b;(void)want;blec_poll(a,now);}
#endif
static unsigned checks, cmds, packets[2], received[2], blocked;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int send_h4(uint8_t h4,const uint8_t *p,uint16_t n,void *ctx)
{ (void)n; if(blocked) return -1; if(h4==H4_CMD) cmds++; else { assert((p[0]&0xff)==TEST_HANDLE_BASE+(unsigned)(uintptr_t)ctx); packets[(unsigned)(uintptr_t)ctx]++; } return 0; }
static void midi(const uint8_t *p,uint8_t n,void *ctx)
{ assert(n==3 && p[0]==0x90); received[(unsigned)(uintptr_t)ctx]++; }
static void setup(blec_t *a,blec_t *b)
{
 const uint8_t addr[6]={1,2,3,4,5,0xc0};
 blec_init(a,send_h4,midi,0,addr); blec_init(b,send_h4,midi,(void*)1,addr); blec_pair_bind(a,b);
 a->state=b->state=BLEC_CONNECTED; a->step=b->step=S_READY;
 a->conn=TEST_HANDLE_BASE;b->conn=(TEST_HANDLE_BASE+1u);a->credits=a->acl_total=5;
 a->midi_handle=b->midi_handle=6; a->found=b->found=1;
 memset(a->target,1,6); memset(b->target,2,6);
 cmds=packets[0]=packets[1]=received[0]=received[1]=blocked=0;
}
static void done(blec_t *a,blec_t *b,unsigned handle,unsigned count)
{ uint8_t p[]={0x13,5,1,handle,handle>>8,count,count>>8};blec_pair_rx(a,b,4,p,sizeof p,1); }
static void down(blec_t *a,blec_t *b,unsigned handle)
{ uint8_t p[]={5,4,0,handle,handle>>8,0x13};blec_pair_rx(a,b,4,p,sizeof p,2); }
static void complete(blec_t *a,blec_t *b,unsigned op,unsigned status)
{ uint8_t p[]={0x0e,4,1,op,op>>8,status};blec_pair_rx(a,b,4,p,sizeof p,3); }
int main(void){blec_t a,b;
 /* Full fake discovery of both peers through production event routing. */
 setup(&a,&b); a.state=b.state=BLEC_OFF;a.conn=b.conn=0;a.credits=a.acl_total=0;
 blec_start(&a,0);
 complete(&a,&b,COP_RESET,0);complete(&a,&b,COP_EV_MASK,0);complete(&a,&b,COP_LE_MASK,0);
 {uint8_t p[]={0x0e,7,1,2,0x20,0,27,0,5};blec_pair_rx(&a,&b,4,p,sizeof p,1);}
 complete(&a,&b,COP_RAND,0);complete(&a,&b,COP_SCAN_PARAM,0);complete(&a,&b,COP_SCAN_ENABLE,0);
 CHECK(a.state==BLEC_SCANNING && b.state==BLEC_OFF);
 for(unsigned slot=0;slot<(TEST_LEGACY?1u:2u);slot++) {
  blec_t *c=slot?&b:&a;
  uint8_t adv[]={0x3e,0x17,0x02,1,0,1,1,2,3,4,5,6,11,10,9,'S','M','C','-','M','i','x','e','r',0xc0};
  if(slot) {
   blec_pair_poll(&a,&b,10,1);CHECK(b.waiting && a.state==BLEC_CONNECTED);
   complete(&a,&b,COP_SCAN_ENABLE,0);
   /* Do not reconnect to the already connected address. */
   blec_pair_rx(&a,&b,4,adv,sizeof adv,11);CHECK(!b.found && b.state==BLEC_SCANNING);
   adv[6]=2;
  }
  blec_pair_rx(&a,&b,4,adv,sizeof adv,12);CHECK(c->found);
  complete(&a,&b,COP_SCAN_ENABLE,0);
  {uint8_t p[]={0x0f,4,0,1,0x0d,0x20};blec_pair_rx(&a,&b,4,p,sizeof p,13);}
  {uint8_t p[]={0x3e,0x13,1,0,TEST_HANDLE_BASE+slot,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};blec_pair_rx(&a,&b,4,p,sizeof p,14);}
  CHECK(c->state==BLEC_DISCOVERING);
  const uint8_t svc[]={0x11,20,4,0,8,0,0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
  const uint8_t chr[]={0x09,21,5,0,0x16,6,0,0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77};
  const uint8_t info[]={0x05,1,7,0,2,0x29};const uint8_t wr[]={0x13};
  const uint8_t *responses[]={svc,chr,info,wr};unsigned sizes[]={sizeof svc,sizeof chr,sizeof info,sizeof wr};
  for(unsigned step=0;step<4;step++) {
   unsigned len=sizes[step];uint8_t p[64]={TEST_HANDLE_BASE+slot,0,len+4,0,len,0,4,0};
   done(&a,&b,TEST_HANDLE_BASE+slot,1);memcpy(p+8,responses[step],len);
   blec_pair_rx(&a,&b,2,p,len+8,15+step);
  }
  CHECK(c->state==BLEC_CONNECTED && c->midi_handle==6 && c->cccd_handle==7);
 }
 CHECK(a.state==BLEC_CONNECTED && (TEST_LEGACY || b.state==BLEC_CONNECTED));
 CHECK(cmds==(TEST_LEGACY?9u:12u)); /* one reset sequence plus two scan-off/create pairs and second scan-on */
 blec_pair_bind(0,0);
 printf("full HCI/ATT discovery: legacy=%u base=%u, %u checks passed\n",TEST_LEGACY,TEST_HANDLE_BASE,checks);return 0;
}
