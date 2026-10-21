/* SPDX-License-Identifier: MPL-2.0
 * Native callback-contract test. No vendor code, HCI producer, replay, hardware,
 * controller transport, or radio is executed. The source adapter is included. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
static int ready=1, next_result, sends, disconnects, copied, links, failures;
static int callback_status=-1;
static int disconnect_error, disconnect_down, confirmations, link_blocked, scans;
static u16 last_handle;
static u32 now;
static u32 ctl_ms(void) { return now; }
int ctl_vendor_address(u8 a[6]) { memset(a,0xc1,6); return 0; }
int ctl_vendor_host_ready(void) { return ready; }
void ctl_vendor_events_reset(void) { ready=0; }
int ctl_os_bootstrap_begin(u32 ms) { (void)ms;return 0; }
void ctl_os_bootstrap_end(void) {}
#ifndef PROFILE_SOURCE
#define PROFILE_SOURCE "../../ble/ctl/src/ctl_vendor_profile.c"
#endif
#include PROFILE_SOURCE
int le_controller_set_mac(void *p) { (void)p;return 0; }
int btstack_init(void) { return 0; }
void os_time_dly(int n) { (void)n;ready=1; }
void le_device_db_init(void) {}
void ble_stack_gatt_role(u8 r) { (void)r; }
void sm_init(void) {}
void sm_set_io_capabilities(int v) { (void)v; }
void sm_set_authentication_requirements(u8 v) { (void)v; }
void sm_set_encryption_key_size_range(u8 a,u8 b) { (void)a;(void)b; }
void sm_set_master_request_pair(int v) { (void)v; }
void sm_event_callback_set(packet_cb cb) { (void)cb; }
void sm_just_works_confirm(u16 h) { (void)h;++confirmations; }
void gatt_client_init(void) {}
void gatt_client_register_packet_handler(packet_cb cb) { (void)cb; }
void hci_event_callback_set(packet_cb cb) { (void)cb; }
void le_l2cap_register_packet_handler(packet_cb cb) { (void)cb; }
u16 ble_vendor_set_default_att_mtu(u16 m) { return m; }
void user_client_init(u16 h,u8 *p,u16 len) { (void)h;(void)p;(void)len; }
int ble_user_cmd_prepare(int cmd,int argc,...)
{
    va_list ap;va_start(ap,argc);
    if(cmd==CMD_DISCONNECT) {
        ++disconnects;last_handle=(u16)va_arg(ap,int);
        if(disconnect_down) {
            u8 down[6]={5,4,0,(u8)last_handle,(u8)(last_handle>>8),0x13};
            packet_event(4,0,down,sizeof down);
        }
        va_end(ap);return disconnect_error;
    }
    if(cmd==CMD_SCAN) { int enable=va_arg(ap,int);if(enable)++scans; }
    if(cmd==CMD_ATT_SEND) {
        ++sends;last_handle=(u16)va_arg(ap,int);
        const u8 *p=va_arg(ap,const u8 *);int n=va_arg(ap,int);int op=va_arg(ap,int);
        if(last_handle==9 && n==2 && p[0]==1 && op==5 && callback_status>=0) {
            u8 status=(u8)callback_status;
            struct data_report done={0xa0,9,0,1,&status,connection};
            user_client_report_data_callback(&done);
        }
    }
    va_end(ap);return next_result;
}
int ctl_vendor_link_copy(u8 up,u8 why) { (void)why;if(link_blocked)return -1;links+=up?1:10;return 0; }
int ctl_vendor_notification_copy(const u8 *p,u32 n) { (void)p;(void)n;++copied;return 0; }
#define CHECK(c) do { if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);++failures;} } while(0)
static void reset(void)
{
    /* Retire the preceding synthetic owned link via the adapter's callback. */
    disconnect_error=disconnect_down=link_blocked=confirmations=0;
    wanted=0;connection=0;u8 down[6]={5,4,0,0,0,0x13};packet_event(4,0,down,6);
    ready=1;next_result=0;sends=disconnects=copied=links=scans=0;callback_status=-1;
    connection=0;connecting=retiring=receiving=off_pending=down_pending=scan_pending=0;
    wanted=discovering=1;midi_handle=midi_end=cccd=0;
}
static void discover(void)
{
    struct search_report r={0};
    const u8 service[16]={0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00};
    const u8 characteristic[16]={0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3};
    memcpy(r.service.uuid128,service,16);memcpy(r.characteristic.uuid128,characteristic,16);
    r.service.start=4;r.service.end=12;r.characteristic.start=5;r.characteristic.value=6;
    r.characteristic.end=10;r.characteristic.properties=0x14;
    user_client_report_search_result(&r);
    struct descriptor_report d={9,0x2902,{0}};user_client_report_descriptor_result(&d);
}
static void finish(void) { user_client_report_search_result((void *)UINTPTR_MAX); }
static void done(u16 handle,u16 peer,u16 offset,u16 len,u8 *status)
{
    struct data_report d={0xa0,handle,offset,len,status,peer};
    user_client_report_data_callback(&d);
}
static void notify(void)
{
    u8 midi[5]={0x80,0x81,0xb0,1,2};struct data_report d={0xa7,6,0,5,midi,0};
    user_client_report_data_callback(&d);
}
int main(void)
{
    const u8 errors[]={0x01,0x03,0x0e,0x7f};
    for(unsigned i=0;i<sizeof(errors);++i) {
        reset();discover();finish();CHECK(sends==1 && !discovering);
        u8 e=errors[i];done(9,0,0,1,&e);
        for(unsigned j=0;j<20;++j){now+=1000;ctl_vendor_poll();}
        CHECK(retiring && disconnects==1 && last_handle==0);
        notify();CHECK(copied==0 && links==0);
    }
    /* A security-related failure must not cancel vendor pairing. Completion
     * and CCCD retry after security remain outside this terminal-error fix. */
    const u8 security_errors[]={0x05,0x08,0x0c,0x0f};
    for(unsigned i=0;i<sizeof(security_errors);++i) {
        reset();discover();finish();u8 e=security_errors[i];done(9,0,0,1,&e);
        CHECK(!retiring && disconnects==0 && !receiving && !links);
        request.address_type=1;memset(request.address,0x42,sizeof request.address);
        u8 jw[16]={0xd0,14,0,0,1,0x42,0x42,0x42,0x42,0x42,0x42};
        security_event(4,0,jw,sizeof jw);CHECK(confirmations==1);
        notify();CHECK(copied==1 && links==1);
    }
    /* Successful acknowledgment alone is not a receiving/link-up claim. */
    reset();discover();finish();u8 ok=0;done(9,0,0,1,&ok);
    CHECK(!retiring && !receiving && !links && !copied);notify();CHECK(copied==1 && links==1);
    /* A valid early notification is not discarded solely because ACK is pending. */
    reset();discover();finish();notify();CHECK(copied==1 && links==1);done(9,0,0,1,&ok);CHECK(!retiring);
    /* Completion ownership: wrong attribute/peer, read-value event and malformed reports. */
    reset();discover();finish();u8 bad=3;
    done(6,0,0,1,&bad);done(9,1,0,1,&bad);done(9,0,1,1,&bad);
    done(9,0,0,0,&bad);done(9,0,0,2,&bad);done(9,0,0,1,NULL);
    struct data_report read={0xa5,9,0,1,&bad,0};user_client_report_data_callback(&read);
    CHECK(!retiring && disconnects==0);done(9,0,0,1,&ok);
    done(9,0,0,1,&bad);CHECK(!retiring && disconnects==0); /* stale duplicate */
    /* No response ownership before a CCCD write is submitted. */
    reset();discover();done(9,0,0,1,&bad);CHECK(!retiring && disconnects==0);
    notify();CHECK(!copied && !links);
    /* Late discovery completion cannot re-enable notifications after OFF/retirement. */
    reset();discover();wanted=0;finish();CHECK(sends==0);
    reset();discover();retiring=1;finish();CHECK(sends==0);
    reset();discover();off_pending=1;finish();CHECK(sends==0);
    reset();discover();down_pending=1;finish();CHECK(sends==0);
    /* Invalid/lost ownership cannot populate handles from late search results. */
    reset();wanted=0;discover();CHECK(midi_handle==0 && cccd==0);
    reset();retiring=1;discover();CHECK(midi_handle==0 && cccd==0);
    /* Handle zero remains a valid connection. Synchronous completion is safe. */
    reset();discover();callback_status=3;finish();CHECK(retiring && disconnects==1);
    reset();discover();callback_status=0;finish();CHECK(!retiring);notify();CHECK(copied==1);
    /* A rejected enqueue never grants subscription, and late errors after down are ignored. */
    reset();discover();next_result=-95;finish();CHECK(retiring);next_result=0;notify();CHECK(!copied);
    reset();discover();finish();u8 down[6]={5,4,0,0,0,0x13};packet_event(4,0,down,6);
    int before=disconnects;done(9,0,0,1,&bad);CHECK(disconnects==before && connection==ABSENT);
    /* Terminal completion uses the retirement retry rather than stranding
     * the owned link when cleanup is temporarily busy/buffer-full. */
    const int transient[]={-100,-97};
    for(unsigned i=0;i<sizeof transient/sizeof transient[0];++i) {
        reset();discover();finish();now=UINT32_MAX-10u;disconnect_error=transient[i];
        done(9,0,0,1,&bad);CHECK(retiring==RETIRING_RETRY && disconnects==1);
        for(unsigned j=0;j<20;++j) { ctl_vendor_poll();++now; }
        CHECK(disconnects==1);ctl_vendor_poll();CHECK(disconnects==2);
        disconnect_error=0;now+=20;ctl_vendor_poll();
        CHECK(retiring==RETIRING_WAIT && disconnects==3 && connection==0);
        for(unsigned j=0;j<10;++j) { now+=100;ctl_vendor_poll(); }
        CHECK(disconnects==3 && scans==0);notify();CHECK(!copied);
        packet_event(4,0,down,sizeof down);
        CHECK(connection==ABSENT && !retiring && subscription==SUB_NONE && scans==1);
    }
    /* Reentrant failure cleanup may synchronously deliver the real down.
     * Neither enqueue return nor retire return may revive old ownership. */
    reset();discover();callback_status=3;disconnect_down=1;finish();
    CHECK(connection==ABSENT && !retiring && subscription==SUB_NONE && disconnects==1);
    reset();discover();finish();disconnect_down=1;done(9,0,0,1,&bad);
    CHECK(connection==ABSENT && !retiring && subscription==SUB_NONE && disconnects==1);
    /* An undelivered old down must defer new discovery/security ownership,
     * not discard their one-shot callbacks after a fresh connection. */
    for(unsigned mode=0;mode<3;++mode) {
        reset();discover();finish();link_blocked=1;
        packet_event(4,0,down,sizeof down);CHECK(connection==ABSENT && down_pending && scans==0);
        const u8 address[6]={1,2,3,4,5,6};
        CHECK(ctl_vendor_scan(1)==-1 && ctl_vendor_connect(1,address)==-1 && !connecting);
        for(unsigned j=0;j<10;++j) { ++now;ctl_vendor_poll(); }
        CHECK(down_pending && scans==0 && scan_pending);
        if(mode) {
            u8 off=0,on=1;CHECK(ctl_vendor_command(0x74,&off,1)==0 && !off_pending);
            if(mode==2) CHECK(ctl_vendor_command(0x74,&on,1)==0);
        }
        CHECK(scans==0);link_blocked=0;ctl_vendor_poll();
        CHECK(!down_pending && links==10 && scans==(mode==1?0:1));
        if(mode==1) { CHECK(!wanted && !scan_pending);continue; }
        CHECK(ctl_vendor_connect(1,address)==0);
        u8 connected[21]={0x3e,19,1,0,0,0,0,1,1,2,3,4,5,6};
        packet_event(4,0,connected,sizeof connected);
        CHECK(connection==0 && discovering && !down_pending);
        u8 jw[16]={0xd0,14,0,0,1,1,2,3,4,5,6};
        security_event(4,0,jw,sizeof jw);CHECK(confirmations==1);
        discover();CHECK(midi_handle==6 && cccd==9);finish();
        CHECK(sends==2 && subscription==SUB_PENDING);
        done(9,0,0,1,&ok);CHECK(subscription==SUB_ENABLED && !retiring);
        notify();CHECK(copied==1 && receiving && links==11);
    }
    if(failures){fprintf(stderr,"subscription callback checks: %d failed\n",failures);return 1;}
    puts("subscription callback checks: terminal/security status, early input, ownership, retry, reentrancy, ordered reconnect PASS");
    return 0;
}
