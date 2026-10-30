/* SPDX-License-Identifier: GPL-3.0-only */
/* Real central parser, pair router and stop lifecycle; synchronous HCI replay. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.c"
static unsigned checks,commands,messages[2],offs[2];
static uint16_t last_op,last_handle;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int send_h4(uint8_t type,const uint8_t*p,uint16_t n,void*ctx){(void)n;(void)ctx;if(type==H4_CMD){commands++;last_op=blec_rd16(p);last_handle=n>=5?blec_rd16(p+3):0xffff;}return 0;}
static void midi(const uint8_t*p,uint8_t n,void*ctx){assert(n==3);unsigned slot=(unsigned)(uintptr_t)ctx;messages[slot]++;if(p[0]==0x80||(p[0]==0x90&&!p[2]))offs[slot]++;}
static void event(blec_t*a,blec_t*b,unsigned handle){uint8_t p[]={0x3e,0x13,1,0,handle,handle>>8,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};blec_pair_rx(a,b,H4_EVT,p,sizeof p,1);}
static void status(blec_t*a,blec_t*b,unsigned op,unsigned code){uint8_t p[]={0x0f,4,code,1,op,op>>8};blec_pair_rx(a,b,H4_EVT,p,sizeof p,2);}
static void down(blec_t*a,blec_t*b,unsigned handle){uint8_t p[]={5,4,0,handle,handle>>8,0x13};blec_pair_rx(a,b,H4_EVT,p,sizeof p,3);}
static void notify(blec_t*a,blec_t*b,unsigned handle,unsigned release){uint8_t p[]={handle,handle>>8,12,0,8,0,4,0,0x1b,6,0,0x80,0x81,0x90,0x18,release?0:127};blec_pair_rx(a,b,H4_ACL,p,sizeof p,4);}
int main(void){
 for(unsigned zero_slot=0;zero_slot<2;zero_slot++){
  blec_t a,b;blec_t*peers[]={&a,&b};const uint8_t addr[]={1,2,3,4,5,0xc0};
  memset(messages,0,sizeof messages);memset(offs,0,sizeof offs);commands=0;
  blec_init(&a,send_h4,midi,0,addr);blec_init(&b,send_h4,midi,(void*)1,addr);blec_pair_bind(&a,&b);
  for(unsigned i=0;i<2;i++){
   peers[i]->state=BLEC_CONNECTING;peers[i]->step=S_CREATE;
   event(&a,&b,i==zero_slot?0:7);
   CHECK(peers[i]->state==BLEC_DISCOVERING);
   peers[i]->state=BLEC_CONNECTED;peers[i]->step=S_READY;peers[i]->att_waiting=0;peers[i]->midi_handle=6;
  }
  a.credits=a.acl_total=5;
  notify(&a,&b,0,0);notify(&a,&b,7,0);notify(&a,&b,0,1);notify(&a,&b,7,1);
  CHECK(messages[0]==2&&messages[1]==2&&offs[0]==1&&offs[1]==1);
  notify(&a,&b,8,0);CHECK(messages[0]==2&&messages[1]==2);
  blec_t*z=peers[zero_slot],*other=peers[1-zero_slot];
  CHECK(blec_conn_update(z,24,40,0,400));status(&a,&b,COP_UPDATE,0);
  z->now_ms=100;blec_stop(z);CHECK(z->state==BLEC_STOPPING&&last_op==COP_DISCONNECT&&last_handle==0);
  unsigned deadline=z->deadline;status(&a,&b,COP_DISCONNECT,0);
  down(&a,&b,8);CHECK(z->state==BLEC_STOPPING&&z->deadline==deadline);
  notify(&a,&b,7,0);CHECK(messages[1-zero_slot]==3&&other->state==BLEC_CONNECTED);
  down(&a,&b,0);CHECK(z->state==BLEC_OFF&&!z->disconnect_flags&&other->state==BLEC_CONNECTED);
  unsigned count=messages[zero_slot];notify(&a,&b,0,0);CHECK(messages[zero_slot]==count);
  /* A fresh zero handle can be admitted to either peer after the matching end. */
  z->state=BLEC_CONNECTING;z->step=S_CREATE;event(&a,&b,0);CHECK(z->state==BLEC_DISCOVERING);
  z->state=BLEC_CONNECTED;z->step=S_READY;z->att_waiting=0;z->midi_handle=6;
  notify(&a,&b,0,0);CHECK(messages[zero_slot]==count+1);
  /* Preserve zero-handle ownership after an expired disconnect until genuine end. */
  z->now_ms=500;blec_stop(z);status(&a,&b,COP_DISCONNECT,0);blec_poll(z,z->deadline);
  CHECK(z->state==BLEC_FAILED);unsigned cmd_before=commands;
  blec_pair_poll(&a,&b,z->deadline+1,1);CHECK(commands==cmd_before&&z->state==BLEC_FAILED);
  down(&a,&b,0);CHECK(z->state==BLEC_OFF&&other->state==BLEC_CONNECTED);
 }
 /* Both basic and enhanced late connection-complete after Cancel retain handle zero. */
 for(unsigned enhanced=0;enhanced<2;enhanced++)for(unsigned expired=0;expired<2;expired++){
  blec_t a,b;const uint8_t addr[]={1,2,3,4,5,0xc0};blec_init(&a,send_h4,midi,0,addr);blec_init(&b,send_h4,midi,(void*)1,addr);blec_pair_bind(&a,&b);
  a.state=BLEC_CONNECTING;a.step=S_CREATE;a.deadline=7000;a.now_ms=100;blec_stop(&a);
  uint8_t complete[]={0x0e,4,1,0x0e,0x20,0};blec_pair_rx(&a,&b,H4_EVT,complete,sizeof complete,101);
  unsigned deadline=a.deadline;uint8_t p[33]={0x3e,enhanced?31:19,enhanced?0x0a:1,0,0,0,0};
  blec_pair_rx(&a,&b,H4_EVT,p,enhanced?33:21,expired?deadline:deadline-1);
  CHECK(a.state==(expired?BLEC_FAILED:BLEC_STOPPING)&&a.deadline==deadline);
  if(!expired)status(&a,&b,COP_DISCONNECT,0);
  down(&a,&b,0);CHECK(a.state==BLEC_OFF&&b.state==BLEC_OFF);
 }
 printf("PASS %u zero-handle lifecycle checks: either peer, note/release isolation, stale/foreign input, STOP/reconnect, expired and late basic/enhanced links\n",checks);
}
