/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: local timeout does not retire accepted asynchronous initiation.
 * cc -std=c99 -O1 -Wall -Wextra -Werror -Ifirmware/src/ble tests/host/ble_async_create_test.c -o /tmp/ble-async && /tmp/ble-async
 */
#include <stdio.h>
#include "ble_midi_central.c"
#include "ble_midi_bus.c"
static int deny;
static int sink(uint8_t h,const uint8_t *p,uint16_t n,void *ctx)
{ (void)h;(void)p;(void)n;(void)ctx;return deny; }
int main(void)
{
    blec_bus_t b; uint8_t addr[6]={0};
    blec_bus_init(&b,sink,0,0,addr); b.started=b.ready=1;b.total=b.available=5;
    b.peer[0].state=BLEC_CONNECTED;b.peer[0].conn=0x40;b.peer[0].has_conn=1;
    b.allowed[1]=1;b.scan_owner=1;b.peer[1].state=BLEC_CONNECTING;
    b.peer[1].step=S_CREATE;b.peer[1].deadline=10;
    blec_progress(&b.peer[1]);
    uint8_t accepted[]={0x0f,4,0,1,0x0d,0x20};
    blec_bus_rx(&b,4,accepted,sizeof accepted,0);
    deny=1;blec_bus_poll(&b,11);blec_bus_poll(&b,3012);blec_bus_poll(&b,3013);
    if(b.scan_owner!=1 || b.peer[0].state!=BLEC_CONNECTED) {
        fprintf(stderr,"FAIL accepted Create lost owner: owner=%u first=%u\n",b.scan_owner,b.peer[0].state);return 1;
    }
    puts("PASS accepted Create retains owner after rejected Cancel and local deadlines");return 0;
}
