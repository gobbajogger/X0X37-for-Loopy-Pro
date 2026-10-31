/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual peer-name copying/joining, with hardware interrupt stubs only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/plat.h"
#include "../../firmware/src/ble/ble_midi_central.h"

static struct { char nm[21]; struct { blec_t c, c2; } host; } ble_r;
static int ble_central_role, locked_irq;
static uint32_t irq_save(void) { assert(!locked_irq); locked_irq=1; return 42; }
static void irq_restore(uint32_t state) { assert(state==42 && locked_irq); locked_irq=0; }
#include "ble_peer_names.inc"

int main(void)
{
    struct {
        uint8_t before;
        char names[PLAT_BLE_PEER_CAP][PLAT_BLE_PEER_NAME_MAX+1u];
        uint8_t after;
    } captured;
    const char *joined;
    ble_central_role=1;
    ble_r.host.c.state=ble_r.host.c2.state=BLEC_CONNECTED;
    memcpy(ble_r.host.c.link_name,"ABCDEFGHIJKLMNOPQRST",21u);
    memcpy(ble_r.host.c2.link_name,"abcdefghijklmnopqrst",21u);
    memset(&captured,0xA5,sizeof captured);
    assert(plat_ble_peer_names(captured.names)==2u);
    assert(!memcmp(captured.names[0],"ABCDEFGHIJKLMNOPQRST",21u));
    assert(!memcmp(captured.names[1],"abcdefghijklmnopqrst",21u));
    assert(captured.before==0xA5u && captured.after==0xA5u);
    joined=plat_ble_name();
    assert(!memcmp(joined,"ABCDEFGHIJKLMNOPQRST, abcdefghijklmnopqrst",43u));
    assert(strlen(joined)==42u && joined[42]=='\0');

    /* The stored link name need not be terminated within the copy limit. */
    memset(ble_r.host.c.link_name,'X',sizeof ble_r.host.c.link_name);
    assert(plat_ble_peer_names(captured.names)==2u);
    assert(!memcmp(captured.names[0],"XXXXXXXXXXXXXXXXXXXX",21u));
    assert(captured.before==0xA5u && captured.after==0xA5u);
    ble_r.host.c.state=BLEC_OFF;
    assert(!strcmp(plat_ble_name(),"abcdefghijklmnopqrst"));
    ble_r.host.c2.link_name[0]=0;
    assert(!strcmp(plat_ble_name(),"SMC-Mixer"));
    ble_r.host.c2.state=BLEC_OFF;
    assert(!plat_ble_name()[0]);
    ble_central_role=0;
    memcpy(ble_r.nm,"X0X37",6u);
    assert(!strcmp(plat_ble_name(),"X0X37"));
    assert(!locked_irq);
    puts("PASS BLE peer-name bounds and maximum-length join");
    return 0;
}
