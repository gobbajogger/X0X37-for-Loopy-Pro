/* SPDX-License-Identifier: GPL-3.0-only */
/* Public HCI ingress regression: fragments, invalid lengths, session boundaries.
 * SMC queue/UI consumers are covered by smc_reachability_test.c. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"
static unsigned count;
static uint8_t last[8];
static int send_h4(uint8_t t,const uint8_t *p,uint16_t n,void *x)
{ (void)t;(void)p;(void)n;(void)x;return 0; }
static void got(const uint8_t *p,uint8_t n,void *x)
{ (void)x;assert(n<=sizeof last);memcpy(last,p,n);count++; }
static void ready(blec_t *c)
{ c->state=BLEC_CONNECTED;c->conn=1;c->linked=1;c->midi_handle=6; }
static const uint8_t note[]={1,0,12,0,8,0,4,0,0x1b,6,0,0x80,0x81,0x90,24,127};
static void feed(blec_t *c,const uint8_t *p,unsigned n)
{ blec_rx(c,2,p,(uint16_t)n,1); }
static void fragment(blec_t *c,unsigned tail)
{
 uint8_t first[sizeof note],end[16]={1,0x10,0,0};
 memcpy(first,note,sizeof note);first[2]-=tail;end[2]=tail;
 memcpy(end+4,note+sizeof note-tail,tail);
 unsigned before=count;
 feed(c,first,sizeof note-tail);assert(count==before);
 feed(c,end,tail+4);assert(count==before+1 && !c->rx_need && !c->rx_have);
}
int main(void)
{
 blec_t c;const uint8_t addr[6]={0};blec_init(&c,send_h4,got,0,addr);ready(&c);
 for(unsigned tail=1;tail<=8;tail++) fragment(&c,tail);
 /* First payload overflows the old buffer into assembly state and transmit queue.
  * ASan does not detect intra-struct overwrites, so retain explicit sentinels. */
 uint8_t big[108]={1,0,104,0,8,0,4,0};big[72]=100;
 memset(c.q,0xa5,sizeof c.q);unsigned before=count;
 feed(&c,big,sizeof big);assert(count==before && c.q[0][0]==0xa5 && !c.rx_need);
 assert(c.rx_reason==BLEC_RX_SIZE);
 uint8_t bad[sizeof note+1];memcpy(bad,note,sizeof note);bad[sizeof note]=0;
 feed(&c,bad,sizeof bad);assert(count==before && c.rx_reason==BLEC_RX_LENGTH);
 bad[2]++;feed(&c,bad,sizeof bad);assert(count==before && c.rx_reason==BLEC_RX_SIZE);
 /* Flag 3 is a complete PDU. This one is still too long for its L2CAP length. */
 bad[1]=0x30;feed(&c,bad,sizeof bad);assert(c.rx_reason==BLEC_RX_SIZE);
 /* Oversized ATT cannot be silently truncated. Existing 64-byte tolerance stays bounded. */
 uint8_t large[73]={1,0,69,0,65,0,4,0};feed(&c,large,sizeof large);
 assert(c.rx_reason==BLEC_RX_SIZE && !c.rx_need);
 /* Short first, stray/overshooting continuation, wrong CID and wrong link. */
 uint8_t first[sizeof note];memcpy(first,note,sizeof note);first[2]=11;
 uint8_t end[]={1,0x10,2,0,127,0};
 feed(&c,first,sizeof note-1);feed(&c,end,sizeof end);
 assert(c.rx_reason==BLEC_RX_FRAGMENT && !c.rx_need && count==before);
 end[2]=1;feed(&c,end,5);assert(c.rx_reason==BLEC_RX_FRAGMENT);
 /* CID 6 is SMP. An unused channel is still rejected. */
 memcpy(first,note,sizeof note);first[6]=7;feed(&c,first,sizeof first);
 assert(c.rx_reason==BLEC_RX_CID);
 assert(c.reject_reason==BLEC_RX_CID && c.reject_n==sizeof first);
 assert(!memcmp(c.reject_head,first,8));
 /* Even a truncated header records exactly the bounded bytes, zero-padded. */
 feed(&c,first,1); assert(c.reject_n==1 && c.reject_reason==BLEC_RX_HEADER);
 assert(c.reject_head[0]==1);
 for(unsigned i=1;i<8;i++) assert(!c.reject_head[i]);
 first[6]=4;first[0]=2;feed(&c,first,sizeof first);assert(c.rx_reason==BLEC_RX_LINK);
 first[0]=1;first[9]=7;feed(&c,first,sizeof first);assert(c.rx_reason==BLEC_RX_HANDLE && c.state==BLEC_CONNECTED);
 assert(c.att_op==0x1b && c.att_handle==7);
 feed(&c,note,sizeof note);assert(count==before+1);
 assert(c.att_handle==6 && c.att_op==0x1b);
 /* Complete PDUs: captured DAW pitch bend, then a channel-1 note. */
 { const uint8_t bend[]={1,0x30,12,0,8,0,4,0,0x1b,6,0,0xae,0xce,0xe0,0x00,0x7e};
   const uint8_t key[]={1,0x30,12,0,8,0,4,0,0x1b,6,0,0xae,0xce,0x90,24,127};
   unsigned at=count;
   feed(&c,bend,sizeof bend);
   assert(count==at+1 && last[0]==0xe0 && last[1]==0x00 && last[2]==0x7e);
   feed(&c,key,sizeof key);
   assert(count==at+2 && last[0]==0x90 && last[1]==24 && last[2]==127);
   assert(c.state==BLEC_CONNECTED); }
 /* Partial SysEx AND partial ACL must not survive any link lifetime boundary. */
 const uint8_t sx[]={1,0,12,0,8,0,4,0,0x1b,6,0,0x80,0x81,0xf0,0x7f,0x7f};
 const uint8_t down[]={5,4,0,1,0,0x13};
 for(unsigned how=0;how<3;how++) {
  ready(&c);feed(&c,sx,sizeof sx);assert(c.in_sysex);
  memcpy(first,note,sizeof note);first[2]=11;feed(&c,first,sizeof note-1);
  assert(c.rx_need);
  if(how==0) blec_start(&c,2);
  else if(how==1) blec_stop(&c);
  else blec_rx(&c,4,down,sizeof down,2);
  assert(!c.in_sysex && !c.sx_len && !c.running && !c.rx_need && !c.rx_have);
  before=count;feed(&c,note,sizeof note);assert(count==before); /* no input while disconnected */
  ready(&c);feed(&c,note,sizeof note);assert(count==before+1 && last[0]==0x90);
 }
 assert(c.rx_acl && c.rx_notify && c.rx_midi && c.rx_reject);
 puts("PASS: HCI fragments 1-8, malformed assembly, bounds, rejection and lifecycle");
 return 0;
}
