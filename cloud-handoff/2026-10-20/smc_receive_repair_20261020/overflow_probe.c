#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../firmware_ux_repair_merge_20261020/firmware/src/ble/ble_midi_central.c"
int main(void) {
 blec_t *c=calloc(1,sizeof *c); uint8_t b[108]={1,0,104,0,8,0,4,0};
 memset(c->q,0xa5,sizeof c->q);
 b[72]=100; c->conn=1;c->state=BLEC_CONNECTED;
 blec_rx(c,2,b,sizeof b,0);assert(c->q[0][0]==0xa5);free(c);return 0;
}
