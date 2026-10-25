#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the real state reporter; a surviving peer must remain visible."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
source=(root/'firmware/src/ble.c').read_text()
start=source.index('int plat_ble_state(void)')
end=source.index('\n}',start)+2
body=source[start:end]
fixture='''
#include "firmware/src/app/plat.h"
#include "firmware/src/ble/ble_midi_bus.h"
#include "firmware/src/ble/ble_midi_host.h"
enum {BLE_IMG_NONE=1,BLE_IMG_BAD=2};
static int ble_img,ble_dead,ble_boot_on=1,ble_want=1,ble_central_role=1,ble_up=1;
static struct {union {blec_bus_t bus;bleh_t h;} host;} ble_r;
'''+body+'''
int main(void) {
 ble_r.host.bus.peer[0].state=BLEC_FAILED;ble_r.host.bus.peer[1].state=BLEC_CONNECTED;
 if(plat_ble_state()!=PLAT_BLE_CONNECTED)return 1;
 ble_r.host.bus.peer[0].state=BLEC_STARTING;
 if(plat_ble_state()!=PLAT_BLE_CONNECTED)return 2;
 ble_dead=1;if(plat_ble_state()!=PLAT_BLE_FAILED)return 3;
 ble_dead=0;ble_up=0;if(plat_ble_state()!=PLAT_BLE_STARTING)return 4;
 ble_up=1;ble_r.host.bus.peer[0].state=BLEC_FAILED;ble_r.host.bus.peer[1].state=BLEC_OFF;
 if(plat_ble_state()!=PLAT_BLE_FAILED)return 5;
 ble_r.host.bus.allowed[1]=1;ble_r.host.bus.peer[1].state=BLEC_SCANNING;
 if(plat_ble_state()!=PLAT_BLE_READY)return 6;
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ble-multi-state-') as tmp:
 p=Path(tmp);(p/'state.c').write_text(fixture)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(root),str(p/'state.c'),'-o',str(p/'state')],check=True)
 subprocess.run([str(p/'state')],check=True)
print('multi-peer production state reporter: 6 scenarios PASS')
