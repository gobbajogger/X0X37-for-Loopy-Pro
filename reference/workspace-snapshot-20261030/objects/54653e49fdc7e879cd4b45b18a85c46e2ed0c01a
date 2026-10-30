/* SPDX-License-Identifier: GPL-3.0-only
 * ATT MTU and CCCD responses survive HCI mailbox backpressure.
 * Build: cc -std=c99 -Wall -Wextra -Werror -I. this.c ble_midi_host.c -o test
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"
static unsigned checks, failed;
#define CHECK(x) do { checks++; if (!(x)) { failed++; fprintf(stderr,"FAIL:%d %s\n",__LINE__,#x); } } while (0)
typedef struct { int reject; unsigned acl; uint8_t type, data[80]; unsigned len; } bus_t;
static int send_packet(uint8_t t,const uint8_t *p,uint16_t n,void *ctx)
{
    bus_t *b=ctx;
    if(b->reject)return -1;
    b->type=t;b->len=n;
    if(n>sizeof b->data)return -1;
    memcpy(b->data,p,n);
    if(t==BLEH_H4_ACL)b->acl++;
    return 0;
}
static void setup(bleh_t*h,bus_t*b)
{
    memset(b,0,sizeof *b);
    bleh_init(h,send_packet,b,"X0X37",0);
    h->state=BLEH_CONNECTED;h->conn=0x40;
    h->acl_total=h->credits=4;
}
static void request(bleh_t*h,const uint8_t*p,unsigned n)
{
    uint8_t b[40]={0x40,0x20,0,0,0,0,4,0};
    CHECK(n<=23);
    if(n>23)return;
    b[2]=b[4]=(uint8_t)n;b[2]+=4;
    memcpy(b+8,p,n);
    bleh_rx(h,BLEH_H4_ACL,b,(uint16_t)(8+n));
}
static void test_mtu(void)
{
    bleh_t h;bus_t b;
    const uint8_t mtu[]={2,23,0},cccd[]={18,7,0,1,0},off[]={0x52,6,0,0x80,0x80,0x80,60,0};
    bleh_msg_t m={0};
    setup(&h,&b);b.reject=1;
    request(&h,mtu,sizeof mtu);
    CHECK(h.mtu_req==23 && b.acl==0);
    request(&h,cccd,sizeof cccd);
    CHECK(h.cccd[0]==0); /* competing request cannot overwrite pending response */
    request(&h,off,sizeof off);
    CHECK(bleh_midi_next(&h,&m)==1 && m.b[0]==0x80 && m.b[1]==60);
    for(unsigned i=0;i<100;i++)bleh_poll(&h);
    CHECK(b.acl==0);
    b.reject=0;bleh_poll(&h);
    CHECK(b.acl==1 && b.type==BLEH_H4_ACL && b.len==11);
    if(b.len==11)CHECK(b.data[8]==3 && b.data[9]==23 && b.data[10]==0);
    for(unsigned i=0;i<10;i++)bleh_poll(&h);
    CHECK(b.acl==1); /* no duplicate, works before CCCD enabled */
}
static void test_cccd(void)
{
    bleh_t h;bus_t b;
    const uint8_t cccd[]={18,7,0,1,0};
    setup(&h,&b);b.reject=1;
    request(&h,cccd,sizeof cccd);
    CHECK(h.cccd[0]==1 && b.acl==0);
    b.reject=0;bleh_poll(&h);
    CHECK(b.acl==1 && b.len==9 && b.data[8]==19);
    CHECK(h.credits==3);
}
static void test_disconnect(void)
{
    bleh_t h;bus_t b;
    const uint8_t mtu[]={2,23,0},bye[]={5,4,0,0x40,0,0x13};
    setup(&h,&b);b.reject=1;
    request(&h,mtu,sizeof mtu);
    bleh_rx(&h,BLEH_H4_EVT,bye,sizeof bye);
    b.reject=0;
    for(unsigned i=0;i<10;i++)bleh_poll(&h);
    CHECK(b.acl==0); /* response from prior connection may never replay */
}
int main(void)
{
    test_mtu();test_cccd();test_disconnect();
    printf("ble ATT retry: %u checks, %u failures\n",checks,failed);
    return failed?1:0;
}
