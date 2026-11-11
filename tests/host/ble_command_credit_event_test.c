/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic HCI events through production routing. No radio proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef BLE_CENTRAL_SOURCE
#define BLE_CENTRAL_SOURCE "../../firmware/src/ble/ble_midi_central.c"
#endif
#include BLE_CENTRAL_SOURCE
static unsigned sends, last_op;
static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx; assert(type == H4_CMD && n >= 3);
    sends++; last_op = blec_rd16(p); return 0;
}
static void event(blec_t *a, blec_t *b, const uint8_t *p, unsigned n)
{ blec_pair_rx(a, b, H4_EVT, p, (uint16_t)n, 10); }
static void setup(blec_t *a, blec_t *b)
{
    const uint8_t addr[6] = {1,2,3,4,5,0xc0};
    blec_init(a, send_h4, 0, 0, addr); blec_init(b, send_h4, 0, 0, addr);
    blec_pair_bind(a,b); sends=last_op=0; blec_start(a,0);
    assert(sends == 1 && last_op == COP_RESET);
}
int main(void)
{
    blec_t a,b;
    const unsigned ops[] = {COP_RESET,COP_EV_MASK,COP_LE_MASK,COP_LE_BUF,
                            COP_RAND,COP_SCAN_PARAM,COP_SCAN_ENABLE};
    const uint8_t credit[] = {0x0e,3,1,0,0}; /* no command => no Status return */
    /* Delay the returned credit at every startup boundary. Each next command
     * must wait, then resume on the five-byte event without a fabricated status. */
    for (unsigned pause=0; pause<sizeof ops/sizeof *ops; pause++) {
        setup(&a,&b);
        for(unsigned i=0;i<sizeof ops/sizeof *ops;i++) {
            assert(last_op==ops[i]);
            uint8_t cc[]={0x0e,4,i==pause?0:1,ops[i],ops[i]>>8,0,27,0,5};
            unsigned n=ops[i]==COP_LE_BUF?9:6; cc[1]=(uint8_t)(n-2);
            unsigned before=sends;
            event(&a,&b,cc,n);
            if(i==pause) {
                assert(sends==before && !blec_command_credit && !blec_command_owner);
                blec_pair_poll(&a,&b,11,1); assert(sends==before);
                event(&a,&b,credit,sizeof credit);
                assert(blec_command_credit); /* original B loses this event */
                blec_pair_poll(&a,&b,12,1);
            }
            if(i+1<sizeof ops/sizeof *ops) assert(sends==before+1);
        }
        assert(a.state==BLEC_SCANNING && b.state==BLEC_OFF && sends==7);
    }
    /* A credit announcement must not complete or steal an outstanding Reset. */
    setup(&a,&b); event(&a,&b,credit,sizeof credit);
    assert(a.waiting && blec_command_owner==&a && sends==1);
    blec_pair_poll(&a,&b,11,1); assert(sends==1);
    /* Truncated/malformed packets cannot grant credit or complete the owner. */
    blec_command_credit=0;
    for(unsigned n=0;n<sizeof credit;n++) event(&a,&b,credit,n);
    assert(!blec_command_credit && blec_command_owner==&a && a.waiting);
    { const uint8_t short_cc[]={0x0e,2,1,0}; event(&a,&b,short_cc,sizeof short_cc); }
    { const uint8_t short_status[]={0x0f,3,0,1,0}; event(&a,&b,short_status,sizeof short_status); }
    assert(!blec_command_credit && blec_command_owner==&a && a.waiting);
    /* Matching opcode without mandatory return Status must not release owner. */
    { const uint8_t missing_status[]={0x0e,3,1,3,0x0c}; event(&a,&b,missing_status,sizeof missing_status); }
    assert(blec_command_owner==&a && a.waiting && sends==1);
    /* No-command Status remains supported; zero credit revokes readiness. */
    { const uint8_t zero[]={0x0e,3,0,0,0}; event(&a,&b,zero,sizeof zero); }
    assert(!blec_command_credit);
    { const uint8_t status[]={0x0f,4,0,1,0,0}; event(&a,&b,status,sizeof status); }
    assert(blec_command_credit && blec_command_owner==&a && a.waiting);
    /* Real command rejection still reaches the production failure path. */
    { const uint8_t error[]={0x0e,4,1,3,0x0c,0x0c}; event(&a,&b,error,sizeof error); }
    assert(a.state==BLEC_FAILED && !blec_command_owner);
    blec_pair_bind(0,0);
    puts("command credit event: 7 startup boundaries and ownership/malformed/error negatives PASS");
    return 0;
}
