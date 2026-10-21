/* SPDX-License-Identifier: MPL-2.0
 * Public callback-boundary contract tests; vendor code/radio are not run. */
#define main subscription_contract_main
#include "ctl_vendor_subscription_test.c"
#undef main
static void encryption(u16 peer,u8 status,u8 enabled)
{
    u8 e[6]={8,4,status,(u8)peer,(u8)(peer>>8),enabled};packet_event(4,0,e,6);
}
static void security_failure(u8 tag)
{
    u8 e[16]={0xdf,14,0,0,1,1,2,3,4,5,6,tag,0,0,0,0};
    security_event(4,0,e,16);
}
static void waiting(void)
{
    reset();request.address_type=1;
    for(unsigned i=0;i<6;++i)request.address[i]=(u8)(i+1);
    discover();finish();u8 auth=5;done(9,0,0,1,&auth);
    CHECK(!retiring && disconnects==0 && sends==1);
}
static void reconnected(u16 handle,u8 final_address)
{
    u8 down[6]={5,4,0,(u8)connection,(u8)(connection>>8),0x13};
    packet_event(4,0,down,6);
    request.address[5]=final_address;connecting=1;
    u8 up[21]={0x3e,19,1,0,(u8)handle,(u8)(handle>>8),0,1,1,2,3,4,5,final_address};
    packet_event(4,0,up,21);
    CHECK(connection==handle && discovering && !retiring);
    discover();finish();u8 auth=5;done(9,handle,0,1,&auth);
    CHECK(!retiring);
}
int main(void)
{
    waiting();encryption(0,0,1);CHECK(sends==1); /* Deferred, not reentrant in HCI dispatch. */
    ctl_vendor_poll();CHECK(sends==2 && last_handle==9 && !retiring && !receiving);
    encryption(0,0,1);ctl_vendor_poll();CHECK(sends==2); /* one accepted retry */
    u8 ok=0;done(9,0,0,1,&ok);CHECK(!links && !receiving);notify();CHECK(copied==1);
    waiting();next_result=-100;encryption(0,0,1);ctl_vendor_poll();CHECK(sends==2 && !retiring);
#ifdef CHECK_INTERNAL_BUDGET
    CHECK(!security_retry_used);
#endif
    next_result=-97;ctl_vendor_poll();CHECK(sends==3 && !retiring);
#ifdef CHECK_INTERNAL_BUDGET
    CHECK(!security_retry_used);
#endif
    next_result=0;ctl_vendor_poll();CHECK(sends==4 && !retiring);
#ifdef CHECK_INTERNAL_BUDGET
    CHECK(security_retry_used);
#endif
    ctl_vendor_poll();CHECK(sends==4);u8 auth=5;done(9,0,0,1,&auth);CHECK(retiring && disconnects==1);
    encryption(0,0,1);ctl_vendor_poll();CHECK(sends==4 && retiring); /* no endless auth retry */
    waiting();encryption(0,5,0);CHECK(retiring && disconnects==1);
    waiting();encryption(0,0,0);CHECK(retiring && disconnects==1);
    waiting();encryption(0,0,1);encryption(0,0,0);ctl_vendor_poll();CHECK(retiring && sends==1);
    waiting();encryption(0,0,1);ctl_vendor_poll();encryption(0,5,0);CHECK(retiring && sends==2);
    waiting();encryption(0,0,1);security_failure(3);ctl_vendor_poll();CHECK(retiring && sends==1);
    waiting();encryption(0,0,1);ctl_vendor_poll();security_failure(4);CHECK(retiring && sends==2);
    waiting();encryption(1,0,1);ctl_vendor_poll();CHECK(sends==1 && !retiring);
    encryption(0,0,2);ctl_vendor_poll();CHECK(sends==1 && !retiring);
    for(u16 n=0;n<6;++n){u8 e[6]={8,4,0,0,0,1};packet_event(4,0,e,n);}
    u8 extra[7]={8,4,0,0,0,1,0};packet_event(4,0,extra,7);
    extra[1]=3;packet_event(4,0,extra,6);extra[1]=5;packet_event(4,0,extra,6);
    extra[1]=4;packet_event(2,0,extra,6);packet_event(4,0,NULL,6);
    CHECK(sends==1 && !retiring);
    /* No recorded CCCD security error means encryption events cause no retry. */
    reset();discover();finish();encryption(0,0,1);ctl_vendor_poll();CHECK(sends==1);
    /* Exact peer verification on DF failure; malformed and unrelated tags ignored. */
    waiting();u8 df[16]={0xdf,14,0,0,1,1,2,3,4,5,6,3,9,1,0,0};
    for(u16 n=0;n<16;++n)security_event(4,0,df,n);
    df[2]=1;security_event(4,0,df,16);df[2]=0;
    df[4]=0;security_event(4,0,df,16);df[4]=1;
    df[5]=2;security_event(4,0,df,16);df[5]=1;
    df[13]=2;security_event(4,0,df,16);df[13]=1;
    df[14]=1;security_event(4,0,df,16);df[14]=0;
    df[11]=4;security_event(4,0,df,16);df[11]=3; /* timeout has no reason/origin */
    security_failure(1);security_failure(16);CHECK(!retiring && disconnects==0);
    security_failure(3);CHECK(retiring && disconnects==1);
    waiting();security_failure(4);CHECK(retiring && disconnects==1);
    /* No silent permanent wait: SDK30s timeout plus5s scheduling grace. */
    now=0xfffffff0u;waiting();now+=34999u;ctl_vendor_poll();CHECK(!retiring);
    ++now;ctl_vendor_poll();CHECK(retiring && disconnects==1);
    waiting();next_result=-100;encryption(0,0,1);ctl_vendor_poll();now+=4999u;
    encryption(0,0,1);ctl_vendor_poll();CHECK(!retiring);++now;ctl_vendor_poll();CHECK(retiring);
    /* OFF/down/retirement cannot trigger a new subscription write. */
    waiting();wanted=0;encryption(0,0,1);ctl_vendor_poll();CHECK(sends==1);
    waiting();retiring=1;encryption(0,0,1);ctl_vendor_poll();CHECK(sends==1);
    u8 off=0;
    waiting();CHECK(ctl_vendor_command(0x74,&off,1)==0);encryption(0,0,1);ctl_vendor_poll();
    CHECK(sends==1 && !wanted && retiring);
    waiting();encryption(0,0,1);CHECK(ctl_vendor_command(0x74,&off,1)==0);
    CHECK(sends==1 && !wanted && retiring); /* OFF precedes poll of queued retry. */
    waiting();encryption(0,0,1);next_result=-100;ctl_vendor_poll();next_result=0;
    CHECK(ctl_vendor_command(0x74,&off,1)==0);CHECK(sends==2 && !wanted && retiring);
    waiting();u8 down[6]={5,4,0,0,0,0x13};packet_event(4,0,down,6);
    encryption(0,0,1);security_failure(3);ctl_vendor_poll();CHECK(sends==1);
    /* Deferred retry can itself synchronously report failure without reviving. */
    waiting();callback_status=3;encryption(0,0,1);ctl_vendor_poll();CHECK(retiring && disconnects==1);
    waiting();callback_status=5;encryption(0,0,1);ctl_vendor_poll();CHECK(retiring && disconnects==1);
    /* A new owned connection gets its own budget, including reused handle zero.
     * Old-peer SM events still carry its old address and must be ignored. */
    waiting();encryption(0,0,1);ctl_vendor_poll();reconnected(0,7);
    security_failure(3);CHECK(!retiring);int before=sends;
    encryption(0,0,1);ctl_vendor_poll();CHECK(sends==before+1 && !retiring);
    done(9,0,0,1,&auth);CHECK(retiring);
    waiting();encryption(0,0,1);ctl_vendor_poll();reconnected(2,8);
    encryption(0,0,1);ctl_vendor_poll();before=sends;
    encryption(2,0,1);ctl_vendor_poll();CHECK(sends==before+1 && !retiring);
    /* Current encryption/failure completion is not lost to old link backlog. */
    waiting();down_pending=1;encryption(0,0,1);CHECK(sends==1);
    ctl_vendor_poll();CHECK(sends==2 && !down_pending && !retiring);
    waiting();down_pending=1;security_failure(3);CHECK(retiring && disconnects==1);
    /* A valid early notification during security recovery remains deliverable. */
    waiting();notify();CHECK(copied==1 && links==1);encryption(0,0,1);ctl_vendor_poll();
    done(9,0,0,1,&ok);CHECK(!retiring && copied==1 && links==1);
    if(failures){fprintf(stderr,"security recovery callback checks: %d failed\n",failures);return 1;}
    puts("security recovery callback checks: deferred single retry, BUSY bound, peer ownership, failures, wrap-time, early input PASS");
    return 0;
}
