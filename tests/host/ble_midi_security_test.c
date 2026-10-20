/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact observed SMP bytes, simulated HCI transport and clock. No physical SMC PASS. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"
static uint8_t type,last[40];
static unsigned sent,busy,nlast,now,messages;
static int send_h4(uint8_t t,const uint8_t *p,uint16_t n,void *ctx)
{ (void)ctx;if(busy)return -1;assert(n<=sizeof last);type=t;nlast=n;memcpy(last,p,n);sent++;return 0; }
static void midi(const uint8_t *p,uint8_t n,void *ctx)
{ (void)ctx;assert(n==3 && p[0]==0xb0 && p[1]==16 && p[2]==1);messages++; }
static void ready(blec_t *c)
{ uint8_t addr[6]={0};blec_init(c,send_h4,midi,0,addr);c->state=BLEC_CONNECTED;c->step=14;c->conn=0x50;c->credits=c->acl_total=5;sent=busy=messages=0;now=100; }
/* Header and payload were observed on installed 4668e48 after initial read. */
static const uint8_t security[]={0x50,0x20,6,0,2,0,6,0,0x0b,1};
static const uint8_t notification[]={0x50,0x20,12,0,8,0,4,0,0x1b,0x72,0,0x80,0x81,0xb0,16,1};
static void feed(blec_t *c,const uint8_t *p,unsigned n)
{ blec_rx(c,2,p,(uint16_t)n,now); }
static void failed_reply(void)
{ const uint8_t expected[]={0x50,0,6,0,2,0,6,0,5,5};assert(type==2 && nlast==sizeof expected && !memcmp(last,expected,sizeof expected)); }
static void notify(blec_t *c)
{ c->midi_handle=114;unsigned before=messages;feed(c,notification,sizeof notification);assert(messages==before+1); }
int main(void)
{
 blec_t c;ready(&c);feed(&c,security,sizeof security);
 assert(sent==1);failed_reply();assert(c.credits==4 && !c.sig_count && blec_connected(&c));notify(&c);
 /* Preserve the same policy during discovery and across a short HCI tail. */
 ready(&c);c.state=BLEC_DISCOVERING;c.step=15;c.att_waiting=1;c.deadline=5000;
 uint8_t first[sizeof security];memcpy(first,security,sizeof first);first[2]=5;
 feed(&c,first,sizeof first-1);assert(!sent && c.rx_need);
 const uint8_t tail[]={0x50,0x10,1,0,1};feed(&c,tail,sizeof tail);
 assert(sent==1 && !c.rx_need && c.att_waiting && c.state==BLEC_DISCOVERING);failed_reply();
 /* A busy transport and zero credits cannot lose the mandatory response. */
 ready(&c);c.credits=0;feed(&c,security,sizeof security);assert(!sent && c.sig_count==1);
 const uint8_t feedback[]={0x90,0x5e,127};assert(blec_midi_send(&c,feedback,3));
 c.credits=1;busy=1;blec_poll(&c,now);assert(!sent && c.sig_count==1 && c.credits==1);
 busy=0;blec_poll(&c,now);assert(sent==1 && !c.sig_count && c.qr!=c.qw);failed_reply();notify(&c);
 /* Pairing Failed is terminal: never bounce failures back at the peer. */
 ready(&c);uint8_t p[sizeof security];memcpy(p,security,sizeof p);p[8]=5;p[9]=5;feed(&c,p,sizeof p);assert(!sent);
 /* Reserved commands and malformed requests do not initiate pairing. */
 p[8]=0xff;feed(&c,p,sizeof p);assert(!sent);
 p[8]=0x0b;p[2]=5;p[4]=1;feed(&c,p,sizeof p-1);assert(!sent && c.rx_reason==BLEC_RX_LENGTH);
 /* Wrong handles, unrelated CIDs and disconnection cannot carry replies. */
 ready(&c);memcpy(p,security,sizeof p);p[0]=0x51;feed(&c,p,sizeof p);assert(!sent && c.rx_reason==BLEC_RX_LINK);
 memcpy(p,security,sizeof p);p[6]=7;feed(&c,p,sizeof p);assert(!sent && c.rx_reason==BLEC_RX_CID);
 ready(&c);c.credits=0;feed(&c,security,sizeof security);const uint8_t down[]={5,4,0,0x50,0,0x13};
 blec_rx(&c,4,down,sizeof down,now);assert(!c.sig_count && c.state==BLEC_SCANNING);
 /* Existing bounded failure path also governs an unsent security rejection. */
 ready(&c);c.credits=0;now=0xfffffff0u;feed(&c,security,sizeof security);
 blec_poll(&c,now+2999u);assert(blec_connected(&c));blec_poll(&c,now+3000u);assert(c.state==BLEC_DISCONNECTING);
 puts("PASS: exact observed Security Request, explicit unsupported response, credits, fragments, lifetime and unchanged MIDI");
 return 0;
}
