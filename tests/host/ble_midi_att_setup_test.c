/* SPDX-License-Identifier: GPL-3.0-only */
/* Public-API, simulated HCI normal setup; no state injection or physical device. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ble_midi_central.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s (state=%u step=%u pending=%u errors=%u)\n",__FILE__,__LINE__,#x,c.state,c.step,c.att_waiting,c.errors); exit(1); } } while(0)
static blec_t c;
static unsigned now, busy, tx_n, tx_type, tx_count;
static uint8_t tx[64];
static int send_h4(uint8_t type,const uint8_t *p,uint16_t n,void *ctx)
{
    (void)ctx;
    if (busy) return -1;
    CHECK(n<=sizeof tx); memcpy(tx,p,n); tx_n=n; tx_type=type; tx_count++;
    return 0;
}
static void cc(unsigned op,const uint8_t *extra,unsigned n)
{
    uint8_t b[16]={0x0e,0,1,(uint8_t)op,(uint8_t)(op>>8),0};
    b[1]=(uint8_t)(4+n); if(n)memcpy(b+6,extra,n);
    blec_rx(&c,4,b,(uint16_t)(6+n),++now);
}
static void completed(unsigned n)
{
    const uint8_t e[]={0x13,5,1,1,0,(uint8_t)n,0};
    blec_rx(&c,4,e,sizeof e,++now);
}
static void acl(unsigned pb,const uint8_t *p,unsigned n,unsigned total)
{
    uint8_t b[80]={1,(uint8_t)(pb<<4),0,0};
    unsigned head=pb==1?4:8;
    b[2]=(uint8_t)(n+head-4);
    if(head==8){b[4]=(uint8_t)total;b[6]=4;}
    memcpy(b+head,p,n);blec_rx(&c,2,b,(uint16_t)(head+n),++now);
}
static void att(const uint8_t *p,unsigned n,unsigned split)
{
    if(split<n){acl(2,p,split,n);acl(1,p+split,n-split,0);}
    else acl(2,p,n,n);
}
static void setup(unsigned credits)
{
    const uint8_t addr[6]={1,2,3,4,5,0xc0}, buf[]={27,0,(uint8_t)credits};
    const uint8_t adv[]={0x3e,0x17,2,1,0,1,1,2,3,4,5,6,11,10,9,'S','M','C','-','M','i','x','e','r',0xc0};
    const uint8_t status[]={0x0f,4,0,1,0x0d,0x20};
    const uint8_t conn[]={0x3e,0x13,1,0,1,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};
    now=busy=tx_count=0;blec_init(&c,send_h4,0,0,addr);blec_start(&c,now);
    cc(0x0c03,0,0);cc(0x0c01,0,0);cc(0x2001,0,0);cc(0x2002,buf,3);
    cc(0x2005,0,0);cc(0x200b,0,0);cc(0x200c,0,0);
    CHECK(c.state==BLEC_SCANNING);
    blec_rx(&c,4,adv,sizeof adv,++now);cc(0x200c,0,0);
    blec_rx(&c,4,status,sizeof status,++now);blec_rx(&c,4,conn,sizeof conn,++now);
    CHECK(c.state==BLEC_DISCOVERING && c.step==10 && c.att_waiting);
    CHECK(tx_type==2 && tx[8]==0x10);
}
static const uint8_t svc[]={0x11,20,4,0,8,0,0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
static const uint8_t chr[]={0x09,21,5,0,0x16,6,0,0xf3,0x6b,0x10,0x9d,0x66,0xf2,0xa9,0xa1,0x12,0x41,0x68,0x38,0xdb,0xe5,0x72,0x77};
static const uint8_t info[]={0x05,1,7,0,2,0x29}, wr[]={0x13};
static const uint8_t mtu[]={2,0xf7,0}, indication[]={0x1d,3,0,1,0,0xff,0xff};
static void discover(unsigned split)
{
    completed(5);att(svc,sizeof svc,split%sizeof svc);CHECK(c.step==11&&c.att_waiting);
    completed(5);att(chr,sizeof chr,split%sizeof chr);CHECK(c.step==12&&c.att_waiting);
    completed(5);att(info,sizeof info,split%sizeof info);CHECK(c.step==13&&c.att_waiting);
}
static void finish(void)
{ att(wr,sizeof wr,1);CHECK(blec_connected(&c)); }
static void response(unsigned op,unsigned n)
{ CHECK(tx_type==2&&tx_n==n+8&&tx[6]==4&&tx[8]==op); }
static int run_case(int argc,char **argv)
{
    CHECK(argc==2);
    if(!strcmp(argv[1],"fragmented")){
        for(unsigned i=0;i<24;i++){setup(5);discover(i);finish();}
    }else if(!strcmp(argv[1],"mtu")){
        setup(5);att(mtu,sizeof mtu,1);
        CHECK(c.state==BLEC_DISCOVERING&&c.step==10&&c.att_waiting);
        response(3,3);CHECK(tx[9]==23&&tx[10]==0);discover(3);finish();
    }else if(!strcmp(argv[1],"indication")){
        setup(5);discover(2);att(indication,sizeof indication,2);
        CHECK(c.state==BLEC_DISCOVERING&&c.step==13&&c.att_waiting);
        response(0x1e,1);finish();
        att(indication,sizeof indication,7);CHECK(blec_connected(&c));response(0x1e,1);
    }else if(!strcmp(argv[1],"unrelated_error")){
        const uint8_t error[]={1,0x0a,1,0,0x0a};setup(5);att(error,sizeof error,5);
        CHECK(c.state==BLEC_DISCOVERING&&c.att_waiting);discover(1);finish();
    }else if(!strcmp(argv[1],"expected_error")){
        const uint8_t error[]={1,0x10,1,0,0x0a};setup(5);att(error,sizeof error,5);
        CHECK(c.state==BLEC_DISCONNECTING&&tx_type==1&&tx[0]==6&&tx[1]==4);
    }else if(!strcmp(argv[1],"pressure")){
        setup(1);unsigned before=tx_count;CHECK(!c.credits);
        att(mtu,sizeof mtu,3);att(indication,sizeof indication,7);
        CHECK(c.state==BLEC_DISCOVERING&&c.att_waiting&&tx_count==before);
        completed(1);busy=1;blec_poll(&c,++now);CHECK(tx_count==before);
        busy=0;blec_poll(&c,++now);response(3,3);CHECK(tx_count==before+1&&c.att_waiting);
        completed(1);blec_poll(&c,++now);response(0x1e,1);CHECK(tx_count==before+2&&c.att_waiting);
        discover(1);finish();
    }else return 2;
    printf("PASS %s: simulated public-API ATT setup\n",argv[1]);return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1) return run_case(argc, argv);
    static const char *modes[] = {"fragmented", "mtu", "indication", "unrelated_error", "expected_error", "pressure"};
    for (unsigned i = 0; i < sizeof modes / sizeof modes[0]; i++) {
        char *args[] = {argv[0], (char *)modes[i]};
        if (run_case(2, args)) return 1;
    }
    return 0;
}
