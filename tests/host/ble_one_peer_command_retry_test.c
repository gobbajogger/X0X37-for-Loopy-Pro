/* SPDX-License-Identifier: GPL-3.0-only */
/* One-peer production central: bounded mailbox retry, HCI credit, stop. No radio. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.c"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %u line %d: %s\n",checks,__LINE__,#x); return 1; } } while (0)
typedef struct {
    unsigned attempts, accepted, last_op, blocked_op, blocked_remaining, acl;
} trace_t;
static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    trace_t *t=ctx;
    if(type == H4_ACL) { t->acl++; return 0; }
    assert(type == H4_CMD && n >= 3u);
    t->last_op=blec_rd16(p);
    t->attempts++;
    if(t->last_op==t->blocked_op && t->blocked_remaining) { t->blocked_remaining--; return -1; }
    t->accepted++;return 0;
}
static void start(blec_t *c,trace_t *t,uint32_t at)
{
    const uint8_t addr[6]={1,2,3,4,5,0xc0};
    memset(t,0,sizeof *t);blec_init(c,send_h4,0,t,addr);blec_start(c,at);
}
static void cc(blec_t *c,unsigned op,unsigned status,unsigned credit,uint32_t now)
{
    const uint8_t p[]={0x0e,4u,(uint8_t)credit,(uint8_t)op,(uint8_t)(op>>8),(uint8_t)status};
    blec_rx(c,H4_EVT,p,sizeof p,now);
}
static void status(blec_t *c,unsigned op,unsigned result,unsigned credit,uint32_t now)
{
    const uint8_t p[]={0x0f,4u,(uint8_t)result,(uint8_t)credit,(uint8_t)op,(uint8_t)(op>>8)};
    blec_rx(c,H4_EVT,p,sizeof p,now);
}
static void credit(blec_t *c,unsigned n,uint32_t now)
{
    const uint8_t p[]={0x0e,3u,(uint8_t)n,0,0};
    blec_rx(c,H4_EVT,p,sizeof p,now);
}
static int rejected_start_and_zero_credit(void)
{
    blec_t c;trace_t t;
    const uint8_t addr[6]={1,2,3,4,5,0xc0};
    memset(&t,0,sizeof t);t.blocked_op=COP_RESET;t.blocked_remaining=1;
    blec_init(&c,send_h4,0,&t,addr);blec_start(&c,100);
    CHECK(t.attempts==1 && t.accepted==0 && c.command_retry && !c.waiting);
    blec_poll(&c,101); CHECK(t.attempts==2 && t.accepted==1 && !c.command_retry && c.waiting);
    blec_poll(&c,102); CHECK(t.attempts==2);
    cc(&c,COP_RESET,0,0,103); /* Reset completed, but no HCI command allowance. */
    CHECK(t.attempts==2 && c.step==S_EVENT && !c.waiting && c.command_retry && !c.command_credit);
    blec_poll(&c,104);CHECK(t.attempts==2);
    { const uint8_t bad[]={0x0e,2,1,0};blec_rx(&c,H4_EVT,bad,sizeof bad,105); }
    { const uint8_t missing_status[]={0x0e,3,1,3,0x0c};blec_rx(&c,H4_EVT,missing_status,sizeof missing_status,105); }
    CHECK(!c.command_credit && t.attempts==2);
    credit(&c,1,106);CHECK(c.command_retry && c.command_credit && t.attempts==2);
    blec_poll(&c,107);CHECK(t.attempts==3 && t.last_op==COP_EV_MASK && c.waiting && !c.command_retry);
    blec_poll(&c,108);CHECK(t.attempts==3);
    return 0;
}
static int later_startup_and_scanner_retry(void)
{
    blec_t c;trace_t t;
    const uint8_t addr[6]={1,2,3,4,5,0xc0};
    memset(&t,0,sizeof t);blec_init(&c,send_h4,0,&t,addr);
    blec_start(&c,0); CHECK(t.last_op==COP_RESET && t.accepted==1);
    cc(&c,COP_RESET,0,1,1);CHECK(t.last_op==COP_EV_MASK && t.accepted==2);
    t.blocked_op=COP_LE_MASK;t.blocked_remaining=1;
    cc(&c,COP_EV_MASK,0,1,2);CHECK(t.last_op==COP_LE_MASK && t.accepted==2 && c.command_retry);
    blec_poll(&c,3); CHECK(t.accepted==3 && c.waiting && !c.command_retry);
    blec_poll(&c,4);CHECK(t.accepted==3);
    cc(&c,COP_LE_MASK,0,1,5);CHECK(t.accepted==4 && t.last_op==COP_LE_BUF);
    const uint8_t buf[]={0x0e,7,1,2,0x20,0,27,0,5};
    blec_rx(&c,H4_EVT,buf,sizeof buf,6); CHECK(t.accepted==5 && t.last_op==COP_RAND);
    cc(&c,COP_RAND,0,1,7);CHECK(t.last_op==COP_SCAN_PARAM);
    cc(&c,COP_SCAN_PARAM,0,1,8);CHECK(t.last_op==COP_SCAN_ENABLE);
    cc(&c,COP_SCAN_ENABLE,0,1,9);CHECK(c.state==BLEC_SCANNING && !c.waiting);
    /* A scan-off command rejected once must resume, but an accepted scan must not repeat. */
    t.blocked_op=COP_SCAN_ENABLE;t.blocked_remaining=1;
    c.step=S_SCAN_OFF;
    blec_progress(&c);CHECK(c.command_retry && !c.waiting);
    unsigned previous=t.accepted;
    blec_poll(&c,10);CHECK(t.accepted==previous+1 && c.waiting && !c.command_retry);
    blec_poll(&c,11);CHECK(t.accepted==previous+1);
    cc(&c,COP_SCAN_ENABLE,0,1,12);CHECK(c.state==BLEC_CONNECTING && c.step==S_CREATE && t.last_op==COP_CREATE);
    status(&c,COP_CREATE,0,1,13);CHECK(!c.waiting);
    previous=t.accepted;
    blec_poll(&c,14);CHECK(t.accepted==previous); /* Accepted Create awaits link event. */
    return 0;
}
static int repeated_rejection_deadline_and_mode(void)
{
    blec_t c;trace_t t;
    const uint8_t addr[6]={1,2,3,4,5,0xc0};
    memset(&t,0,sizeof t);t.blocked_op=COP_RESET;t.blocked_remaining=100;
    blec_init(&c,send_h4,0,&t,addr);blec_start(&c,0);
    blec_poll(&c,1);blec_poll(&c,2999);
    CHECK(t.attempts==3 && c.state==BLEC_STARTING && c.command_retry);
    blec_poll(&c,3000);CHECK(c.state==BLEC_FAILED && t.attempts==3);
    blec_poll(&c,4000);CHECK(t.attempts==3);
    /* Stop clears a stale unaccepted Reset without sending it late. */
    t.blocked_remaining=1;
    blec_start(&c,5000);CHECK(t.attempts==4 && c.command_retry);
    blec_stop(&c);CHECK(c.state==BLEC_OFF && !c.command_retry);
    blec_pair_poll(&c,0,5001,0);CHECK(t.attempts==4);
    t.blocked_remaining=0;
    blec_start(&c,6000);CHECK(c.state==BLEC_STARTING && c.waiting && t.last_op==COP_RESET);
    CHECK(t.attempts==5 && t.accepted==1);
    blec_poll(&c,6001);CHECK(t.attempts==5);
    return 0;
}
static int stop_cancel_retries_and_cutoff(void)
{
    blec_t c;trace_t t;
    start(&c,&t,0);CHECK(t.accepted==1);
    /* No create has been accepted; stop must replace the old operation with Cancel. */
    c.state=BLEC_CONNECTING;c.step=S_CREATE;c.waiting=0;c.command_credit=1;c.now_ms=100;
    t.blocked_op=COP_CANCEL;t.blocked_remaining=2;
    blec_stop(&c);CHECK(c.state==BLEC_STOPPING && c.step==S_CANCEL && c.command_retry && !c.waiting);
    unsigned was=t.accepted;
    blec_poll(&c,101);CHECK(t.accepted==was && c.command_retry);
    blec_poll(&c,102);CHECK(t.accepted==was+1 && !c.command_retry && c.waiting);
    blec_poll(&c,103);CHECK(t.accepted==was+1);
    cc(&c,COP_CANCEL,0,1,104);
    blec_poll(&c,105);CHECK(t.accepted==was+1);
    /* A genuinely exhausted mailbox may not trigger a late unsafe Cancel. */
    start(&c,&t,0);
    c.state=BLEC_CONNECTING;c.step=S_CREATE;c.waiting=0;c.command_credit=1;c.now_ms=10;
    t.blocked_op=COP_CANCEL;t.blocked_remaining=100;
    blec_stop(&c);
    blec_poll(&c,c.deadline-1);was=t.attempts;
    blec_poll(&c,c.deadline);blec_poll(&c,c.deadline+1);
    CHECK(c.state==BLEC_FAILED && t.attempts==was);
    return 0;
}
static int connect_wins_stop_handle_zero(void)
{
    blec_t c; trace_t t;
    start(&c,&t,0);
    c.state=BLEC_CONNECTING;c.step=S_CREATE;c.waiting=0;c.command_credit=1;c.now_ms=100;
    blec_stop(&c);
    CHECK(t.last_op==COP_CANCEL && c.state==BLEC_STOPPING && c.waiting);
    /* A valid HCI handle zero can win the cancellation race. No Disconnect
     * may steal the accepted Cancel command's HCI ownership. */
    const uint8_t link[]={0x3e,19,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned before=t.accepted;
    blec_rx(&c,H4_EVT,link,sizeof link,101);
    CHECK(c.state==BLEC_STOPPING && c.conn==0 && t.accepted==before);
    cc(&c,COP_CANCEL,0,1,102);
    blec_poll(&c,103);
    CHECK(t.accepted==before+1 && t.last_op==COP_DISCONNECT && c.waiting);
    blec_poll(&c,104);CHECK(t.accepted==before+1);
    { const uint8_t down[]={5,4,0,0,0,0x13};blec_rx(&c,H4_EVT,down,sizeof down,105); }
    CHECK(c.state==BLEC_OFF && c.conn==0);
    return 0;
}
static int quiet_command_owner_and_pair_disabled(void)
{
    blec_t a,b;trace_t ta,tb;
    const uint8_t addr[6]={1,2,3,4,5,0xc0};
    memset(&ta,0,sizeof ta);memset(&tb,0,sizeof tb);
    blec_init(&a,send_h4,0,&ta,addr);blec_init(&b,send_h4,0,&tb,addr);
    blec_pair_bind(&a,&b);
    a.state=BLEC_CONNECTED;a.conn=0x40;a.linked=1;a.step=S_READY;a.credits=a.acl_total=5;
    uint8_t bdata[14]={0};
    CHECK(blec_command_quiet(&a,COP_UPDATE,bdata,sizeof bdata));
    CHECK(ta.accepted==1 && a.quiet_pending && !a.command_credit && !a.waiting);
    CHECK(!blec_command_quiet(&a,COP_UPDATE,bdata,sizeof bdata));
    CHECK(ta.accepted==1);
    /* Quiet HCI update cannot let STOP steal its pending command owner. */
    blec_stop(&a);CHECK(a.state==BLEC_STOPPING && a.command_retry && ta.accepted==1);
    status(&a,COP_UPDATE,0,1,1);
    CHECK(!a.quiet_pending && a.command_credit && !a.waiting);
    blec_pair_poll(&a,&b,2,0);CHECK(ta.accepted==2 && ta.last_op==COP_DISCONNECT);
    CHECK(b.state==BLEC_OFF && tb.accepted==0);
    return 0;
}
int main(void)
{
    if(rejected_start_and_zero_credit())return 1;
    if(later_startup_and_scanner_retry())return 1;
    if(repeated_rejection_deadline_and_mode())return 1;
    if(stop_cancel_retries_and_cutoff())return 1;
    if(connect_wins_stop_handle_zero())return 1;
    if(quiet_command_owner_and_pair_disabled())return 1;
    printf("single-peer HCI retry/credit/stop: %u checks PASS\n",checks);
    return 0;
}
