/* SPDX-License-Identifier: MPL-2.0
 * Independent application callback edge checks; no vendor/producer execution. */
#define main existing_subscription_contract_main
#include "ctl_vendor_subscription_test.c"
#undef main
static void enc(u8 status, u8 enabled) {
    u8 e[6] = {8,4,status,(u8)connection,(u8)(connection>>8),enabled};
    packet_event(4,0,e,sizeof(e));
}
static void wait_security(u8 status) {
    reset(); request.address_type=1;
    for(unsigned i=0;i<6;++i) request.address[i]=(u8)(i+1);
    discover(); finish(); done(9,connection,0,1,&status);
    CHECK(subscription==SUB_SECURITY_PENDING && !retiring && sends==1);
}
int main(void) {
    const u8 errors[] = {5,8,12,15};
    for(unsigned a=0; a<sizeof(errors); ++a) {
        /* All four security statuses get the same single accepted retry. */
        wait_security(errors[a]); enc(0,1); callback_status=0; ctl_vendor_poll();
        CHECK(subscription==SUB_ENABLED && security_retry_used && sends==2 && !retiring);
        enc(0,1); ctl_vendor_poll(); CHECK(sends==2 && !receiving);
        notify(); CHECK(receiving && copied==1);
        for(unsigned b=0; b<sizeof(errors); ++b) {
            wait_security(errors[a]); enc(0,1); callback_status=errors[b]; ctl_vendor_poll();
            CHECK(retiring && subscription==SUB_NONE && sends==2 && disconnects==1);
        }
        /* Valid failure tags for each producer origin and each byte reason. */
        for(unsigned origin=0;origin<2;++origin) for(unsigned reason=0;reason<256;++reason) {
            wait_security(errors[a]);
            u8 e[16]={0xdf,14,0,0,1,1,2,3,4,5,6,3,(u8)reason,(u8)origin,0,0xa5};
            security_event(4,0,e,sizeof(e)); CHECK(retiring && disconnects==1);
        }
        /* Unrecognized encryption values never schedule a retry. */
        for(unsigned flag=2;flag<256;++flag) {
            wait_security(errors[a]); enc(0,(u8)flag); ctl_vendor_poll();
            CHECK(subscription==SUB_SECURITY_PENDING && sends==1 && !retiring);
        }
    }
    /* BUSY accompanied by a synchronous terminal callback cannot restore retry.
     * This is deliberately adversarial beyond the queue's usual contract. */
    wait_security(5); enc(0,1); callback_status=5; next_result=-100; ctl_vendor_poll();
    CHECK(retiring && subscription==SUB_NONE && sends==2); now+=5000; ctl_vendor_poll(); CHECK(sends==2);
    /* Exact retry-enqueue deadline wraps independently of the security wait. */
    now=0xffffff00u; wait_security(5); enc(0,1); next_result=-97;
    now+=4999; ctl_vendor_poll(); CHECK(!retiring && sends==2);
    ++now; ctl_vendor_poll(); CHECK(retiring && sends==2);
    /* Known boundary: an encryption event before the security-error report is
     * not remembered; the new wait still retires at its bound. */
    reset(); discover(); finish(); enc(0,1); u8 auth=5; done(9,0,0,1,&auth);
    now+=35000; ctl_vendor_poll(); CHECK(retiring && sends==1);
    if(failures) return 1;
    puts("independent callback edge matrix PASS: four security statuses, synchronous retry results, 2048 SM failure tags, 1016 unknown encryption values, BUSY retirement, wrapped enqueue deadline, pre-error race bounded");
    return 0;
}
