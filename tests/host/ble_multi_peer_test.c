/* SPDX-License-Identifier: GPL-3.0-only
 * Production HCI manager traces, no radio/device claim.
 * cc -std=c99 -O1 -g -Wall -Wextra -Werror tests/host/ble_multi_peer_test.c -o /tmp/ble-multi && /tmp/ble-multi
 */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.c"
#include "../../firmware/src/ble/ble_midi_bus.c"
static unsigned checks, failures, commands, resets, acls, reject_send, midi_peer = 255;
#define CHECK(x) do { checks++; if (!(x)) { failures++; fprintf(stderr,"line %d: %s\n",__LINE__,#x); } } while(0)
static int sink(uint8_t h, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)n; (void)ctx;
    if (reject_send) return -1;
    if (h == 1) { commands++; if (blecb_rd16(p) == COP_RESET) resets++; }
    if (h == 2) acls++;
    return 0;
}
static void midi(uint8_t peer, const uint8_t *p, uint8_t n, void *ctx)
{ (void)p; (void)n; (void)ctx; midi_peer = peer; }
static void init(blec_bus_t *b)
{
    uint8_t addr[6] = {1,2,3,4,5,0xc6};
    blec_bus_init(b,sink,midi,0,addr);
    commands=resets=acls=reject_send=0; midi_peer=255;
}
static void complete(blec_bus_t *b, unsigned op, uint8_t status)
{
    uint8_t p[] = {0x0e,4,1,(uint8_t)op,(uint8_t)(op>>8),status};
    blec_bus_rx(b,4,p,sizeof p,100);
}
static void status(blec_bus_t *b, unsigned op, uint8_t status)
{
    uint8_t p[] = {0x0f,4,status,1,(uint8_t)op,(uint8_t)(op>>8)};
    blec_bus_rx(b,4,p,sizeof p,100);
}
static void live(blec_bus_t *b)
{
    init(b); b->ready=b->started=1; b->total=b->available=5;
    for (unsigned i=0;i<2;i++) {
        b->peer[i].state=BLEC_CONNECTED; b->peer[i].step=S_READY;
        b->peer[i].conn=(uint16_t)(0x40+i); b->peer[i].midi_handle=7;
    }
    blecb_credit_view(b);
}
static void done(blec_bus_t *b,unsigned h,unsigned count)
{
    uint8_t p[]={0x13,5,1,(uint8_t)h,(uint8_t)(h>>8),(uint8_t)count,(uint8_t)(count>>8)};
    blec_bus_rx(b,4,p,sizeof p,200);
}
static void down(blec_bus_t *b,unsigned h)
{
    uint8_t p[]={5,4,0,(uint8_t)h,(uint8_t)(h>>8),0x13};
    blec_bus_rx(b,4,p,sizeof p,200);
}
static void test_bootstrap(void)
{
    blec_bus_t b; init(&b); blec_bus_start(&b,0);
    CHECK(resets==1 && b.command==COP_RESET);
    for (unsigned op=COP_RESET;;) {
        if (op==COP_LE_BUF) {
            uint8_t p[]={0x0e,7,1,2,0x20,0,27,0,5};
            blec_bus_rx(&b,4,p,sizeof p,100);
        } else complete(&b,op,0);
        op=b.command;
        if (op==COP_SCAN_ENABLE) break;
        CHECK(op!=0); if (!op) break;
    }
    complete(&b,COP_SCAN_ENABLE,0);
    CHECK(b.ready && b.peer[0].state==BLEC_SCANNING && b.scan_owner==0);
    b.allowed[1]=1;
    blec_join(&b.peer[1],100);
    CHECK(resets==1 && b.peer[1].retry_command && !b.peer[1].waiting);
    b.peer[0].state=BLEC_CONNECTED; b.peer[0].step=S_READY; b.peer[0].conn=0x40;
    b.scan_owner=BLEC_NO_OWNER;
    blec_bus_poll(&b,101);
    CHECK(b.command==COP_SCAN_ENABLE && b.command_owner==1 && resets==1);
    complete(&b,COP_SCAN_ENABLE,0);
    CHECK(b.peer[0].conn==0x40 && b.peer[0].state==BLEC_CONNECTED);
}
static void test_credits(void)
{
    blec_bus_t b; uint8_t p[]={0x90,1,1}; live(&b);
    CHECK(blec_bus_midi_send(&b,0,p,3) && blec_bus_midi_send(&b,1,p,3));
    blec_bus_poll(&b,1);
    CHECK(acls==2 && b.available==3 && b.outstanding[0]==1 && b.outstanding[1]==1);
    done(&b,0x41,1); CHECK(b.available==4 && b.outstanding[0]==1 && !b.outstanding[1]);
    done(&b,0x41,600); CHECK(b.available==4 && b.outstanding[0]==1);
    done(&b,0x888,4); CHECK(b.available==4);
    reject_send=1; CHECK(blec_bus_midi_send(&b,0,p,3)); blec_bus_poll(&b,2);
    CHECK(b.available==4 && b.outstanding[0]==1 && b.peer[0].qr!=b.peer[0].qw);
    reject_send=0; blec_bus_poll(&b,3); CHECK(b.available==3 && b.outstanding[0]==2);
    down(&b,0x40); CHECK(b.available==5 && !b.outstanding[0] && b.peer[1].conn==0x41);
    done(&b,0x40,2); CHECK(b.available==5);
    /* Handle reused only after ordered Disconnection Complete. No old debt survives. */
    b.peer[0].conn=0x40; b.peer[0].state=BLEC_CONNECTED; b.peer[0].step=S_READY;
    CHECK(blec_bus_midi_send(&b,0,p,3)); blec_bus_poll(&b,4);
    CHECK(b.available==4 && b.outstanding[0]==1);
    done(&b,0x40,1); CHECK(b.available==5);
    uint8_t truncated[]={0x13,9,2,0x40,0,1,0,0x41};
    blec_bus_rx(&b,4,truncated,sizeof truncated,5); CHECK(b.available==5);
    CHECK(!blec_bus_midi_send(&b,2,p,3));
}
static void test_command_owner(void)
{
    blec_bus_t b; live(&b);
    blec_bus_stop(&b,10);
    CHECK(b.command==COP_DISCONNECT && b.command_owner==0);
    CHECK(b.peer[0].waiting && !b.peer[1].waiting);
    status(&b,COP_UPDATE,0); CHECK(b.command==COP_DISCONNECT);
    status(&b,COP_DISCONNECT,0); CHECK(!b.command && !b.peer[0].waiting);
    blec_bus_poll(&b,11); CHECK(b.command==COP_DISCONNECT && b.command_owner==1);
    status(&b,COP_DISCONNECT,0); down(&b,0x40); down(&b,0x41);
    CHECK(b.peer[0].state==BLEC_OFF && b.peer[1].state==BLEC_OFF && b.available==5);
    blec_bus_start(&b,12); blec_bus_poll(&b,12);
    CHECK(!b.stopping && !resets && b.command==COP_SCAN_ENABLE);
}
static void test_notification_and_targets(void)
{
    blec_bus_t b; live(&b);
    uint8_t p[]={0x41,0x20,10,0,6,0,4,0,0x1b,7,0,0x80,0x80,0x90,1,2};
    /* Exact L2CAP/ACL lengths are 8 and 12. */
    p[2]=12; p[4]=8;
    blec_bus_rx(&b,2,p,sizeof p,1); CHECK(midi_peer==1);
    p[0]=0x42; midi_peer=255; blec_bus_rx(&b,2,p,sizeof p,2); CHECK(midi_peer==255);
    uint8_t addr[]={1,2,3,4,5,6}; memcpy(b.peer[0].target,addr,6); b.peer[0].target_type=1;
    CHECK(!blecb_target(addr,1,&b.peer_ctx[1]));
    b.allowed[1]=1; memcpy(b.allowed_addr[1],addr,6); b.allowed_type[1]=1;
    CHECK(!blecb_target(addr,1,&b.peer_ctx[1]));
    CHECK(!blecb_target(addr,0,&b.peer_ctx[1]));
    b.peer[0].conn=0; CHECK(blecb_target(addr,1,&b.peer_ctx[1]));
}
static void test_command_fault(void)
{
    blec_bus_t b; live(&b);
    uint8_t q[]={1,2,3};
    CHECK(blec_conn_update(&b.peer[0],24,40,0,400));
    blec_bus_poll(&b,4000); CHECK(b.command_fault && b.command==COP_UPDATE);
    CHECK(blec_bus_midi_send(&b,1,q,3)); blec_bus_poll(&b,4001);
    CHECK(acls==1 && b.peer[1].state==BLEC_CONNECTED);
    CHECK(!blec_conn_update(&b.peer[1],24,40,0,400));
    status(&b,COP_UPDATE,0); CHECK(!b.command_fault && !b.command);
    CHECK(blec_conn_update(&b.peer[1],24,40,0,400));
}
static void test_zero_handle_and_lengths(void)
{
    blec_bus_t b; init(&b); b.started=b.ready=1; b.total=b.available=5;
    b.peer[0].state=BLEC_CONNECTING; b.peer[0].step=S_CREATE; b.scan_owner=0; b.create_pending=1;
    uint8_t bad[]={0x3e,19,0x0a,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    blec_bus_rx(&b,4,bad,sizeof bad,1);
    CHECK(!blec_has_conn(&b.peer[0]) && !b.generation[0]);
    bad[2]=1; blec_bus_rx(&b,4,bad,sizeof bad,2);
    CHECK(blec_has_conn(&b.peer[0]) && b.peer[0].conn==0 && b.generation[0]==1);
    CHECK(b.peer[0].state==BLEC_DISCOVERING && b.outstanding[0]==1);
    b.peer[0].state=BLEC_CONNECTED; b.peer[0].step=S_READY; b.peer[0].att_waiting=0;
    uint8_t msg[]={0x90,4,5}; CHECK(blec_bus_midi_send(&b,0,msg,3)); blec_bus_poll(&b,3);
    CHECK(b.outstanding[0]==2 && b.available==3);
    done(&b,0,2); CHECK(!b.outstanding[0] && b.available==5);
    down(&b,0); CHECK(!blec_has_conn(&b.peer[0]) && b.available==5);
    done(&b,0,2); CHECK(b.available==5);
    CHECK(!blec_bus_allow_peer(&b,1,msg,1)); /* Config closes at start. */
    init(&b); uint8_t addr[6]={1,2,3,4,5,6};
    CHECK(!blecb_target(addr,1,&b.peer_ctx[1]));
    CHECK(blec_bus_allow_peer(&b,1,addr,1)); CHECK(blecb_target(addr,1,&b.peer_ctx[1]));
    addr[0]++; CHECK(!blecb_target(addr,1,&b.peer_ctx[1]));
}
static void test_credit_only_command_event(void)
{
    blec_bus_t b; live(&b);
    b.command_credit=0;
    CHECK(!blec_conn_update(&b.peer[0],24,40,0,400));
    uint8_t p[]={0x0e,3,1,0,0}; blec_bus_rx(&b,4,p,sizeof p,1);
    CHECK(b.command_credit==1 && !b.command);
    CHECK(blec_conn_update(&b.peer[0],24,40,0,400));
    p[2]=2; blec_bus_rx(&b,4,p,sizeof p,2);
    CHECK(b.command==COP_UPDATE && b.command_owner==0 && b.command_credit==2);
    status(&b,COP_UPDATE,0); CHECK(!b.command && b.command_credit==1);
}
static void pending_create(blec_bus_t *b)
{
    live(b); b->allowed[1]=1; b->peer[1].conn=0; b->peer[1].has_conn=0;
    b->peer[1].state=BLEC_CONNECTING; b->peer[1].step=S_CREATE;
    b->peer[1].now_ms=0; b->peer[1].deadline=10; b->scan_owner=1;
    blec_progress(&b->peer[1]); CHECK(b->create_pending && b->command==COP_CREATE);
    status(b,COP_CREATE,0); CHECK(b->create_pending && !b->command);
}
static void terminal_create(blec_bus_t *b,unsigned status_code)
{
    uint8_t p[]={0x3e,19,1,(uint8_t)status_code,0x41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    blec_bus_rx(b,4,p,sizeof p,3014);
}
static void test_async_create_quarantine(void)
{
    blec_bus_t b; pending_create(&b);
    reject_send=1; blec_bus_poll(&b,11); blec_bus_poll(&b,3012); blec_bus_poll(&b,3013);
    CHECK(b.scan_owner==1 && b.create_pending && b.peer[1].state==BLEC_FAILED);
    CHECK(b.peer[0].state==BLEC_CONNECTED);
    reject_send=0; terminal_create(&b,0);
    CHECK(!b.create_pending && blec_has_conn(&b.peer[1]) && b.peer[1].conn==0x41);
    CHECK(b.peer[1].state==BLEC_STOPPING && b.command==COP_DISCONNECT && b.command_owner==1);
    status(&b,COP_DISCONNECT,0); down(&b,0x41);
    CHECK(!blec_has_conn(&b.peer[1]) && b.peer[0].state==BLEC_CONNECTED);
    pending_create(&b); blec_bus_poll(&b,11);
    CHECK(b.command==COP_CANCEL); complete(&b,COP_CANCEL,0);
    CHECK(b.create_pending && b.scan_owner==1 && !b.command);
    unsigned sent=commands; blec_bus_poll(&b,12);
    CHECK(commands==sent && b.create_pending); /* Same-owner scan is deferred too. */
    terminal_create(&b,2); CHECK(!b.create_pending && b.peer[1].state==BLEC_OFF);
    blec_bus_poll(&b,3015); CHECK(b.command==COP_SCAN_ENABLE && b.command_owner==1);
    CHECK(b.peer[0].state==BLEC_CONNECTED && !resets);
    /* A confirmed scanner cannot transfer ownership on local timeout either. */
    live(&b); b.allowed[1]=1; b.peer[1].conn=0; b.peer[1].state=BLEC_FAILED;
    b.scan_owner=1; b.scan_active=1; blec_bus_poll(&b,5000);
    CHECK(b.scan_owner==1 && !b.command && b.peer[0].state==BLEC_CONNECTED);
}
static void advertise(blec_bus_t *b,const uint8_t addr[6])
{
    uint8_t p[]={0x3e,17,2,1,0,1,0,0,0,0,0,0,5,4,9,'S','M','C',0};
    memcpy(p+6,addr,6); blec_bus_rx(b,4,p,sizeof p,5);
}
static void scan_fixture(blec_bus_t *b,unsigned peer)
{
    b->peer[peer].state=BLEC_SCANNING; b->peer[peer].step=S_SCAN_ON;
    b->peer[peer].waiting=0; b->peer[peer].deadline=10;
    b->scan_owner=(uint8_t)peer; b->scan_active=1; b->command=0;
    b->command_owner=BLEC_NO_OWNER; b->command_credit=1;
}
static void test_exact_slot_admission_and_fair_scan(void)
{
    blec_bus_t b; uint8_t a[6]={1,2,3,4,5,6},z[6]={6,5,4,3,2,1},other[6]={7,7,7,7,7,7};
    for(unsigned first=0;first<2;first++) {
        init(&b); CHECK(blec_bus_allow_peer(&b,1,z,1));
        CHECK(!blec_bus_allow_peer(&b,0,z,1)); CHECK(blec_bus_allow_peer(&b,0,a,1));
        b.started=b.ready=1;b.total=b.available=5;scan_fixture(&b,0);
        advertise(&b,first? a:z);
        if(!first) CHECK(!b.peer[0].found);
        advertise(&b,first? z:a);
        CHECK(b.peer[0].found && !memcmp(b.peer[0].target,a,6));
        b.peer[0].state=BLEC_CONNECTED;b.peer[0].conn=0x40;b.peer[0].has_conn=1;
        scan_fixture(&b,1);advertise(&b,a);CHECK(!b.peer[1].found);
        advertise(&b,z);CHECK(b.peer[1].found && !memcmp(b.peer[1].target,z,6));
        /* Reconnection cannot silently inherit the logical slot's locks. */
        b.peer[0].conn=0;b.peer[0].has_conn=0;b.peer[0].found=0;
        scan_fixture(&b,0);advertise(&b,other);CHECK(!b.peer[0].found);
        advertise(&b,z);CHECK(!b.peer[0].found);advertise(&b,a);CHECK(b.peer[0].found);
    }
    init(&b);CHECK(blec_bus_allow_peer(&b,1,z,1));blec_bus_start(&b,0);
    CHECK(!b.started && !resets && b.peer[0].state==BLEC_FAILED);
    init(&b); CHECK(blec_bus_allow_peer(&b,0,a,1));CHECK(blec_bus_allow_peer(&b,1,z,1));
    b.started=b.ready=1;b.total=b.available=5;scan_fixture(&b,0);
    blec_bus_poll(&b,11);
    CHECK(b.scan_owner==0 && b.scan_active && b.command==COP_SCAN_ENABLE && b.scan_value==0);
    complete(&b,COP_SCAN_ENABLE,0);CHECK(!b.scan_active && b.peer[0].state==BLEC_OFF);
    blec_bus_poll(&b,12);CHECK(b.scan_owner==1 && b.command==COP_SCAN_ENABLE && b.scan_value==1);
    complete(&b,COP_SCAN_ENABLE,0);advertise(&b,z);
    CHECK(b.peer[1].found && !resets);
    init(&b);b.started=b.ready=1;b.total=b.available=5;scan_fixture(&b,0);
    blec_bus_poll(&b,11);CHECK(b.scan_owner==0 && b.peer[0].state==BLEC_SCANNING && !b.command);
}
int main(void)
{
    test_exact_slot_admission_and_fair_scan(); test_async_create_quarantine(); test_credit_only_command_event(); test_zero_handle_and_lengths(); test_command_fault(); test_bootstrap(); test_credits(); test_command_owner(); test_notification_and_targets();
    printf("multi-peer HCI: %u checks, %u failures; sizeof central=%zu bus=%zu\n",checks,failures,sizeof(blec_t),sizeof(blec_bus_t));
    return failures?1:0;
}
