#ifndef TEST_HANDLE_BASE
#define TEST_HANDLE_BASE 0x40u
#endif
/* SPDX-License-Identifier: GPL-3.0-only */
/* Production dual-peer HCI, ACL ownership and lifecycle; no physical device. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.c"
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
 /* Publish real Connection Complete into each production peer parser so
  * a zero handle carries exactly the same ownership as a physical event. */
 blec_t *peers[]={a,b};
 for(unsigned slot=0;slot<2;slot++){
  peers[slot]->state=BLEC_CONNECTING;
  uint8_t event[]={0x3e,0x13,1,0,TEST_HANDLE_BASE+slot,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};
  blec_rx(peers[slot],H4_EVT,event,sizeof event,0);
  peers[slot]->att_waiting=0;
 }
 a->state=b->state=BLEC_CONNECTED; a->step=b->step=S_READY;
 a->credits=a->acl_total=5;
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
int main(void)
{
 blec_t a,b;const uint8_t note[]={0x90,0x18,127};
 for(unsigned drop=0;drop<2;drop++) for(unsigned count=1;count<=4;count++) {
  setup(&a,&b);
  for(unsigned i=0;i<count;i++) { CHECK(blec_midi_send(&a,note,3));blec_poll(&a,i); }
  CHECK(blec_midi_send(&b,note,3));blec_poll(&b,1);
  CHECK(a.credits==4-count && a.inflight==count && b.inflight==1);
  down(&a,&b,TEST_HANDLE_BASE+drop);
  CHECK(a.credits==(drop?5-count:4)); CHECK((drop?a.conn:b.conn)==(drop?TEST_HANDLE_BASE:(TEST_HANDLE_BASE+1u)));
  unsigned free=a.credits;done(&a,&b,TEST_HANDLE_BASE+drop,65535); CHECK(a.credits==free);
  done(&a,&b,TEST_HANDLE_BASE+(1-drop),65535); CHECK(a.credits==5 && !a.inflight && !b.inflight);
  done(&a,&b,TEST_HANDLE_BASE+(1-drop),65535);CHECK(a.credits==5);
 }
 setup(&a,&b); blocked=1; CHECK(!blec_acl_cid(&a,4,note,3));CHECK(a.credits==5 && !a.inflight);blocked=0;
 CHECK(blec_acl_cid(&a,4,note,3)); CHECK(blec_acl_cid(&b,4,note,3));
 { uint8_t p[]={0x13,9,2,TEST_HANDLE_BASE,0,1,0,(TEST_HANDLE_BASE+1u),0,1,0};blec_pair_rx(&a,&b,4,p,sizeof p,2);CHECK(a.credits==5); }
 /* Real notification routing, no unknown-handle delivery, both peer contexts. */
 for(unsigned slot=0;slot<2;slot++) {uint8_t p[]={TEST_HANDLE_BASE+slot,0,12,0,8,0,4,0,0x1b,6,0,0x80,0x81,0x90,0x18,127};blec_pair_rx(&a,&b,2,p,sizeof p,3);CHECK(received[slot]==1);}
 {uint8_t p[]={(TEST_HANDLE_BASE+2u),0,12,0,8,0,4,0,0x1b,6,0,0x80,0x81,0x90,0x18,127};blec_pair_rx(&a,&b,2,p,sizeof p,3);CHECK(received[0]==1 && received[1]==1);}
 /* Only accepted commands own completions; quiet updates never consume peer discovery. */
 setup(&a,&b);CHECK(blec_conn_update(&a,24,40,0,400));CHECK(cmds==1);
 b.state=BLEC_SCANNING;b.conn=0;b.step=S_SCAN_ON;b.deadline=10000;blec_progress(&b);
 CHECK(!b.waiting && b.command_retry && cmds==1);
 complete(&a,&b,COP_SCAN_ENABLE,0);CHECK(blec_command_owner==&a);
 {uint8_t p[]={0x0f,4,0,1,0x13,0x20};blec_pair_rx(&a,&b,4,p,sizeof p,4);}
 CHECK(!blec_command_owner && a.state==BLEC_CONNECTED);
 blec_pair_poll(&a,&b,5,1);CHECK(b.waiting && cmds==2 && blec_command_owner==&b);
 complete(&a,&b,COP_SCAN_ENABLE,0);CHECK(b.state==BLEC_SCANNING && !b.waiting);
 unsigned before=cmds;for(unsigned i=0;i<20;i++)blec_pair_poll(&a,&b,6+i,1);CHECK(cmds==before);
 /* Rejecting the second scan leaves the established peer and its feedback usable. */
 b.waiting=0;b.step=S_SCAN_ON;blec_progress(&b);complete(&a,&b,COP_SCAN_ENABLE,1);
 CHECK(b.state==BLEC_FAILED && a.state==BLEC_CONNECTED);
 CHECK(blec_midi_send(&a,note,3));blec_pair_poll(&a,&b,30,1);CHECK(packets[0]==1);
 /* Two simultaneous stop requests serialize Disconnect without resetting radio. */
 setup(&a,&b);CHECK(blec_acl_cid(&a,4,note,3));CHECK(blec_acl_cid(&b,4,note,3));
 blec_pair_poll(&a,&b,100,0);CHECK(cmds==1 && a.state==BLEC_STOPPING && b.state==BLEC_STOPPING);
 {uint8_t p[]={0x0f,4,0,1,6,4};blec_pair_rx(&a,&b,4,p,sizeof p,101);}
 blec_pair_poll(&a,&b,102,0);CHECK(cmds==2);
 down(&a,&b,TEST_HANDLE_BASE);CHECK(a.state==BLEC_OFF && a.credits==4);
 {uint8_t p[]={0x0f,4,0,1,6,4};blec_pair_rx(&a,&b,4,p,sizeof p,103);}
 down(&a,&b,(TEST_HANDLE_BASE+1u));CHECK(b.state==BLEC_OFF && a.credits==5);
 /* One-credit sustained load gives both peers forward progress. */
 setup(&a,&b);a.credits=a.acl_total=1;
 for(unsigned i=0;i<16;i++){CHECK(blec_midi_send(&a,note,3));CHECK(blec_midi_send(&b,note,3));}
 for(unsigned i=0;i<16;i++){blec_pair_poll(&a,&b,i,1);done(&a,&b,a.inflight?TEST_HANDLE_BASE:(TEST_HANDLE_BASE+1u),1);}
 CHECK(packets[0]==8 && packets[1]==8 && a.credits==1);
 /* A stopped second initiation can still report a late connection. Retain
  * the handle and the original cutoff; never silently reuse it or reset peer A. */
 for(unsigned enhanced=0;enhanced<2;enhanced++) for(unsigned expired=0;expired<2;expired++) {
  setup(&a,&b);b.conn=0;b.state=BLEC_CONNECTING;b.step=S_CREATE;b.deadline=7000;
  b.now_ms=100;blec_stop(&b);CHECK(b.state==BLEC_STOPPING && b.waiting);
  complete(&a,&b,COP_CANCEL,0);unsigned deadline=b.deadline;
  uint8_t p[33]={0x3e,enhanced?31:19,enhanced?0x0a:1,0,(TEST_HANDLE_BASE+1u),0,0};
  blec_pair_rx(&a,&b,4,p,enhanced?33:21,expired?deadline:deadline-1);
  CHECK(b.conn==(TEST_HANDLE_BASE+1u) && b.deadline==deadline && a.state==BLEC_CONNECTED);
  CHECK(b.state==(expired?BLEC_FAILED:BLEC_STOPPING));
  if(!expired)complete(&a,&b,COP_DISCONNECT,0);
  down(&a,&b,(TEST_HANDLE_BASE+1u));CHECK(b.state==BLEC_OFF && a.state==BLEC_CONNECTED);
 }
 /* Controller command credits can pause new commands after completion. */
 setup(&a,&b);CHECK(blec_conn_update(&a,24,40,0,400));
 {uint8_t p[]={0x0f,4,0,0,0x13,0x20};blec_pair_rx(&a,&b,4,p,sizeof p,1);}
 CHECK(!blec_command_owner && !blec_command_credit);
 CHECK(!blec_conn_update(&b,24,40,0,400) && cmds==1);
 {uint8_t p[]={0x0e,4,1,0,0,0};blec_pair_rx(&a,&b,4,p,sizeof p,2);}
 CHECK(blec_conn_update(&b,24,40,0,400) && cmds==2);
 setup(&a,&b);a.state=b.state=BLEC_OFF;blec_command_credit=0;
 CHECK(!blec_pair_stopped(&a,&b));blec_command_credit=1;CHECK(blec_pair_stopped(&a,&b));
 /* Full fake discovery of both peers through production event routing. */
 setup(&a,&b); a.state=b.state=BLEC_OFF;a.conn=b.conn=0;a.credits=a.acl_total=0;
 blec_start(&a,0);
 complete(&a,&b,COP_RESET,0);complete(&a,&b,COP_EV_MASK,0);complete(&a,&b,COP_LE_MASK,0);
 {uint8_t p[]={0x0e,7,1,2,0x20,0,27,0,5};blec_pair_rx(&a,&b,4,p,sizeof p,1);}
 complete(&a,&b,COP_RAND,0);complete(&a,&b,COP_SCAN_PARAM,0);complete(&a,&b,COP_SCAN_ENABLE,0);
 CHECK(a.state==BLEC_SCANNING && b.state==BLEC_OFF);
 for(unsigned slot=0;slot<2;slot++) {
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
 CHECK(a.state==BLEC_CONNECTED && b.state==BLEC_CONNECTED);
 CHECK(cmds==12); /* one reset sequence plus two scan-off/create pairs and second scan-on */
 blec_pair_bind(0,0);
 printf("dual peer transport: %u checks passed\n",checks);return 0;
}
