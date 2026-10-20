/* SPDX-License-Identifier: GPL-3.0-only */
/* Simulated controller, independent OpenSSL AES oracle fixtures; no device PASS.
 * Core5.4 Vol3H2.2.3/2.2.4 order is pinned at every HCI encryption boundary. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"
static uint8_t type,last[40]; static unsigned sent,busy,nlast,now,messages,last_fail_reason;
static const uint8_t plain[5][16]={ {0x01,0x01,0x03,0x00,0x04,0x05,0x16,0x07,0x08,0x0b,0x09,0x0b,0x0d,0x1d,0x0e,0x0f},
 {0x7c,0xa0,0x60,0x42,0xa8,0x8e,0x68,0x6a,0x26,0x58,0x24,0x70,0x27,0x28,0x07,0xd6},
 {0x11,0x11,0x13,0x10,0x14,0x15,0x06,0x17,0x18,0x1b,0x19,0x1b,0x1d,0x0d,0x1e,0x1f},
 {0xba,0xe3,0xda,0x03,0x11,0xa1,0xb4,0xfa,0xd4,0xf1,0x03,0x30,0xe4,0x50,0x9b,0x85},
 {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17}};
static const uint8_t cipher[5][16]={ {0x6c,0x80,0x50,0x02,0xf8,0xee,0x69,0x68,0x25,0x5c,0x21,0xb0,0x27,0x28,0x07,0xd6},
 {0x55,0x65,0xbb,0x65,0xe8,0xdd,0xcf,0x6e,0x97,0xea,0xa7,0x60,0x78,0x64,0x6a,0x11},
 {0xaa,0xc3,0xea,0x43,0x41,0xc1,0xb5,0xf8,0xd7,0xf5,0x06,0xf0,0xe4,0x50,0x9b,0x85},
 {0x7e,0xa2,0x19,0x3e,0xe0,0x9b,0xf7,0x1a,0xa8,0x8d,0x6e,0x38,0x9f,0x1c,0x7c,0x30},
 {0x5a,0xcb,0x9e,0xd7,0xed,0xa4,0x2b,0x41,0x52,0x7c,0xdd,0x92,0x2a,0x81,0xf4,0xdd}};
static const uint8_t local[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static const uint8_t peer[16]={16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
static const uint8_t security[]={0x50,0x20,6,0,2,0,6,0,0x0b,1};
static const uint8_t response[]={2,3,0,1,16,0,0};
static int send_h4(uint8_t t,const uint8_t*p,uint16_t n,void*ctx)
{(void)ctx;if(busy)return -1;assert(n<=sizeof last);type=t;nlast=n;memcpy(last,p,n);if(t==2&&n==10&&p[6]==6&&p[8]==5)last_fail_reason=p[9];sent++;return 0;}
static void midi(const uint8_t*p,uint8_t n,void*ctx)
{(void)ctx;assert(n==3&&p[0]==0xb0&&p[1]==16&&p[2]==1);messages++;}
static void ready(blec_t*c)
{const uint8_t addr[]={1,2,3,4,5,0xc0},target[]={0x10,0x20,0x30,0x40,0x50,0x60};blec_init(c,send_h4,midi,0,addr);c->state=BLEC_CONNECTED;c->step=14;c->conn=0x50;c->midi_handle=114;c->cccd_handle=115;c->credits=c->acl_total=5;memcpy(c->target,target,6);c->target_type=0;sent=busy=messages=last_fail_reason=0;now=100;}
static void feed(blec_t*c,const uint8_t*p,unsigned n){blec_rx(c,2,p,(uint16_t)n,now);}
static void pdu(blec_t*c,unsigned cid,const uint8_t*p,unsigned n)
{uint8_t b[80]={0x50,0x20,0,0,0,0,0,0};assert(n+8<=sizeof b);b[2]=(uint8_t)(n+4);b[4]=(uint8_t)n;b[6]=(uint8_t)cid;memcpy(b+8,p,n);feed(c,b,n+8);}
static void smp(blec_t*c,const uint8_t*p,unsigned n){pdu(c,6,p,n);}
static void complete(blec_t*c,unsigned op,const uint8_t*p,unsigned n)
{uint8_t e[30]={0x0e,0,1,(uint8_t)op,(uint8_t)(op>>8),0};assert(n+6<=sizeof e);e[1]=(uint8_t)(n+4);memcpy(e+6,p,n);blec_rx(c,4,e,(uint16_t)(n+6),now);}
static void command(unsigned op,unsigned n)
{assert(type==1&&nlast==n+3&&last[0]==(uint8_t)op&&last[1]==(uint8_t)(op>>8)&&last[2]==n);}
static void outgoing(unsigned op,unsigned n)
{assert(type==2&&nlast==n+8&&last[0]==0x50&&last[2]==n+4&&last[4]==n&&last[6]==6&&last[8]==op);}
static void begin(blec_t*c)
{feed(c,security,sizeof security);blec_poll(c,now);outgoing(1,7);const uint8_t req[]={1,3,0,0,16,0,0};assert(!memcmp(last+8,req,7));smp(c,response,7);}
static void crypt_complete(blec_t*c,unsigned i)
{blec_poll(c,++now);command(0x2017,32);for(unsigned j=0;j<16;j++)assert(!last[3+j]);assert(!memcmp(last+19,plain[i],16));complete(c,0x2017,cipher[i],16);}
static void randoms(blec_t*c)
{for(unsigned i=0;i<2;i++){blec_poll(c,++now);command(0x2018,0);complete(c,0x2018,local+8*i,8);}}
static void through_peer_random(blec_t*c,int corrupt)
{begin(c);randoms(c);crypt_complete(c,0);crypt_complete(c,1);blec_poll(c,++now);outgoing(3,17);assert(!memcmp(last+9,cipher[1],16));uint8_t p[17]={3};memcpy(p+1,cipher[3],16);if(corrupt)p[1]^=1;smp(c,p,sizeof p);blec_poll(c,++now);outgoing(4,17);assert(!memcmp(last+9,local,16));p[0]=4;memcpy(p+1,peer,16);smp(c,p,sizeof p);}
static void encrypted(blec_t*c)
{const uint8_t status[]={0x0f,4,0,1,0x19,0x20},wrong[]={8,4,0,0x51,0,1},ok[]={8,4,0,0x50,0,1};blec_rx(c,4,status,sizeof status,now);unsigned before=sent;blec_poll(c,++now);assert(sent==before);blec_rx(c,4,wrong,sizeof wrong,now);blec_poll(c,++now);assert(sent==before);blec_rx(c,4,ok,sizeof ok,now);}
static void success_and_att(void)
{blec_t c;ready(&c);c.state=BLEC_DISCOVERING;c.step=15;c.att_waiting=1;c.deadline=200;begin(&c);const uint8_t rd[]={0x0b};pdu(&c,4,rd,1);assert(!c.att_waiting&&c.step==13);randoms(&c);crypt_complete(&c,0);crypt_complete(&c,1);blec_poll(&c,++now);outgoing(3,17);assert(!memcmp(last+9,cipher[1],16));uint8_t p[17]={3};memcpy(p+1,cipher[3],16);smp(&c,p,17);blec_poll(&c,++now);outgoing(4,17);p[0]=4;memcpy(p+1,peer,16);smp(&c,p,17);crypt_complete(&c,2);crypt_complete(&c,3);crypt_complete(&c,4);blec_poll(&c,++now);command(0x2019,28);assert(last[3]==0x50);for(unsigned i=5;i<15;i++)assert(!last[i]);assert(!memcmp(last+15,cipher[4],16));encrypted(&c);blec_poll(&c,++now);assert(type==2&&last[6]==4&&last[8]==0x12&&last[9]==115);const uint8_t wr[]={0x13};pdu(&c,4,wr,1);assert(blec_connected(&c));const uint8_t note[]={0x1b,114,0,0x80,0x81,0xb0,16,1};pdu(&c,4,note,8);assert(messages==1);}
static void retries(void)
{blec_t c;ready(&c);c.credits=0;feed(&c,security,sizeof security);blec_poll(&c,now);assert(!sent);c.credits=1;busy=1;blec_poll(&c,++now);assert(!sent&&c.credits==1);busy=0;blec_poll(&c,++now);outgoing(1,7);assert(!c.credits);smp(&c,response,7);busy=1;unsigned before=sent;blec_poll(&c,++now);assert(sent==before&&!c.waiting);busy=0;blec_poll(&c,++now);command(0x2018,0);before=sent;blec_poll(&c,++now);assert(sent==before&&c.waiting);complete(&c,0x2018,local,8);}
static void failures(void)
{blec_t c;ready(&c);through_peer_random(&c,1);crypt_complete(&c,2);crypt_complete(&c,3);assert(blec_connected(&c));c.credits=0;unsigned before_fail=sent;complete(&c,0x2017,cipher[3],16);blec_poll(&c,++now);assert(sent==before_fail);c.credits=1;busy=1;blec_poll(&c,++now);assert(sent==before_fail);busy=0;blec_poll(&c,++now);assert(c.state==BLEC_DISCONNECTING&&last_fail_reason==4);command(0x0406,3);
 for(unsigned how=0;how<2;how++){ready(&c);begin(&c);blec_poll(&c,++now);command(0x2018,0);if(how==0)blec_stop(&c);else{const uint8_t down[]={5,4,0,0x50,0,0x13};blec_rx(&c,4,down,sizeof down,now);}unsigned before=sent;complete(&c,0x2018,local,8);assert(!c.sec&&sent==before);for(unsigned i=0;i<16;i++)assert(!c.random[i]&&!c.peer[i]&&!c.confirm[i]&&!c.crypt[i]);}
 ready(&c);now=0xfffffff0u;feed(&c,security,sizeof security);c.credits=0;blec_poll(&c,now+29999u);assert(blec_connected(&c));blec_poll(&c,now+30000u);assert(c.state==BLEC_DISCONNECTING);
 ready(&c);const uint8_t failed[]={5,5};smp(&c,failed,2);assert(!sent&&!c.sec);uint8_t wrong[sizeof security];memcpy(wrong,security,sizeof wrong);wrong[0]++;feed(&c,wrong,sizeof wrong);assert(!c.sec&&c.rx_reason==BLEC_RX_LINK);
 ready(&c);begin(&c);unsigned oldsec=c.sec,before=sent;const uint8_t reserved[]={0xff};smp(&c,reserved,1);assert(c.sec==oldsec&&sent==before);const uint8_t zero[]={0};smp(&c,zero,1);assert(c.sec==oldsec&&sent==before);
 ready(&c);const uint8_t short_request[]={0x0b};smp(&c,short_request,1);assert(!c.sec&&c.rx_reason==BLEC_RX_LENGTH);
}
static void att_security_retry(void)
{blec_t c;ready(&c);c.state=BLEC_DISCOVERING;c.step=15;c.att_waiting=1;const uint8_t error[]={1,0x0a,114,0,5};pdu(&c,4,error,5);assert(c.sec&&!c.att_waiting&&c.step==15);through_peer_random(&c,0);crypt_complete(&c,2);crypt_complete(&c,3);crypt_complete(&c,4);blec_poll(&c,++now);command(0x2019,28);encrypted(&c);blec_poll(&c,++now);assert(type==2&&last[6]==4&&last[8]==0x0a&&last[9]==114&&c.att_waiting);}
int main(void){success_and_att();att_security_retry();retries();failures();puts("PASS: independent legacy pairing AES oracle, 35-byte HCI, ATT overlap, retries, peer-confirm failure, lifetime and wrap");return 0;}
