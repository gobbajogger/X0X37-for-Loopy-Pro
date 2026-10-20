/* Independent public API probe. HCI events, time and mailbox refusal are simulated. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../../firmware/src/ble/ble_midi_central.h"
static unsigned busy, cancels, refused_cancels, disconnects;
static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx; assert(type == 1 || type == 2); assert(n >= 3);
    unsigned op = (unsigned)p[0] | (unsigned)p[1] << 8;
    if (type == 1 && op == 0x200e) {
        if (busy) refused_cancels++; else cancels++;
    }
    if (type == 1 && op == 0x0406 && !busy) disconnects++;
    return busy ? -1 : 0;
}
static void cc(blec_t *c, unsigned op, const uint8_t *extra, unsigned n)
{
    uint8_t b[16] = {0x0e, 0, 1, (uint8_t)op, (uint8_t)(op >> 8), 0};
    b[1] = (uint8_t)(4u + n);
    if (n) memcpy(b + 6, extra, n);
    blec_rx(c, 4, b, (uint16_t)(6u + n), 10);
}
int main(int argc, char **argv)
{
    blec_t c; const uint8_t addr[6] = {1,2,3,4,5,0xc0};
    const uint8_t buf[] = {27,0,5};
    const uint8_t adv[] = {0x3e,0x17,0x02,1,0,1,1,2,3,4,5,6,11,10,9,'S','M','C','-','M','i','x','e','r',0xc0};
    const uint8_t status[] = {0x0f,4,0,1,0x0d,0x20};
    uint8_t conn[33] = {0x3e,0x13,1,0,0x50,0,0,1,2,3,4,5,6,0,0,0,0,0,0,0,0};
    unsigned refused = argc > 1 && !strcmp(argv[1], "busy");
    unsigned conn_n = 21;
    if (argc > 2) {
        conn[1] = 31; conn[2] = 0x0a; conn_n = sizeof conn;
        conn[26] = 24; conn[30] = 0x90; conn[31] = 1;
    } else { conn[14] = 24; conn[18] = 0x90; conn[19] = 1; }
    blec_init(&c,send_h4,0,0,addr); blec_start(&c,0);
    cc(&c,0x0c03,0,0); cc(&c,0x0c01,0,0); cc(&c,0x2001,0,0);
    cc(&c,0x2002,buf,sizeof buf); cc(&c,0x2005,0,0); cc(&c,0x200b,0,0); cc(&c,0x200c,0,0);
    assert(c.state == BLEC_SCANNING);
    blec_rx(&c,4,adv,sizeof adv,20); cc(&c,0x200c,0,0);
    assert(c.state == BLEC_CONNECTING && c.waiting && c.cmd_op == 0x200d && !c.conn);
    blec_rx(&c,4,status,sizeof status,30);
    assert(c.state == BLEC_CONNECTING && !c.waiting && !c.conn);
    printf("Accepted Create status: state=%u step=%u waiting=%u conn=%u\n",c.state,c.step,c.waiting,c.conn);
    busy = refused; blec_stop(&c);
    printf("After Cancel: refused=%u accepted=%u state=%u waiting=%u conn=%u deadline=%u\n",refused_cancels,cancels,c.state,c.waiting,c.conn,c.deadline);
    busy = 0; blec_rx(&c,4,conn,(uint16_t)conn_n,31); blec_poll(&c,31);
    printf("After successful %s connection: state=%u waiting=%u conn=%u disconnects=%u\n",argc > 2 ? "enhanced" : "legacy",c.state,c.waiting,c.conn,disconnects);
    fflush(stdout);
    assert(c.conn == 0x50 && c.state == BLEC_STOPPING && disconnects == 1);
    return 0;
}
