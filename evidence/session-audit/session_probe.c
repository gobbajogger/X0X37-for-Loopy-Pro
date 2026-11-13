
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ble_midi_central.h"
static unsigned sends, messages;
static int tx(uint8_t h4,const uint8_t *p,uint16_t n,void *ctx)
{ (void)h4;(void)p;(void)n;(void)ctx;sends++;return 0; }
static void midi(const uint8_t *p,uint8_t n,void *ctx)
{ (void)ctx;assert(n==3 && p[0]==0xb0 && p[1]==16 && p[2]==1);messages++; }
int main(void) {
 blec_t c; const uint8_t addr[6]={0};
 blec_init(&c,tx,midi,0,addr);
 c.state=BLEC_CONNECTED;c.conn=0x50;c.midi_handle=114;c.cccd_handle=115;
 c.step=14;c.credits=c.acl_total=5;
 const uint8_t parameter_update[]={0x50,0x20,0x10,0,0x0c,0,5,0,0x12,1,8,0,6,0,12,0,0,0,0x90,1};
 blec_rx(&c,2,parameter_update,sizeof parameter_update,1000);
 assert(sends==0 && messages==0 && blec_connected(&c));
 blec_poll(&c,601000);
 assert(sends==0 && blec_connected(&c));
 const uint8_t notification[]={0x50,0x20,12,0,8,0,4,0,0x1b,114,0,0x80,0x81,0xb0,16,1};
 blec_rx(&c,2,notification,sizeof notification,601001);
 assert(messages==1 && blec_connected(&c));
 printf("{\"signaling_responses\":%u,\"still_connected_after_600_seconds\":true,\"MIDI_after_rejection\":%u}\n",sends,messages);
}
