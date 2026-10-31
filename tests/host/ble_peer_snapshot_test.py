#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[2]
s=(r/'firmware/src/ble.c').read_text();f=s[s.index('void plat_ble_peers('):s.index('int plat_ble_smc_send_slot(')]
h='''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "plat.h"
typedef struct { int connected; char link_name[21]; } blec_t;
static struct { struct { blec_t c,c2; } host; } ble_r;
static int ble_central_role=1,role=1,ble_role_changing,ble_want=1,ble_boot_on=1,ble_up=1,ble_host_on=1,ble_dead,state=PLAT_BLE_CONNECTED,locked;
static unsigned irq_save(void){assert(!locked);locked=1;return 42;}
static void irq_restore(unsigned f){assert(locked&&f==42);locked=0;}
static int smc_ble_role_get(void){assert(locked);return role;}
static int blec_connected(const blec_t*c){assert(locked);return c->connected;}
int plat_ble_state(void){assert(locked);return state;}
'''
t='''int main(void){plat_ble_peers_t p;ble_r.host.c.connected=ble_r.host.c2.connected=1;memset(ble_r.host.c.link_name,'A',21);strcpy(ble_r.host.c2.link_name,"SMC-Mixer");plat_ble_peers(&p);assert(!locked&&p.count==2&&strlen(p.name[0])==20&&!strcmp(p.name[1],"SMC-Mixer"));ble_r.host.c.connected=0;plat_ble_peers(&p);assert(p.count==1&&!strcmp(p.name[0],"SMC-Mixer"));ble_r.host.c2.link_name[0]=0;plat_ble_peers(&p);assert(!strcmp(p.name[0],"SMC-Mixer"));int* flags[]={&ble_central_role,&role,&ble_want,&ble_boot_on,&ble_up,&ble_host_on};for(unsigned i=0;i<sizeof flags/sizeof flags[0];i++){*flags[i]=0;plat_ble_peers(&p);assert(p.count==0&&!p.name[0][0]);*flags[i]=1;}ble_role_changing=1;plat_ble_peers(&p);assert(!p.count);ble_role_changing=0;ble_dead=1;plat_ble_peers(&p);assert(!p.count);ble_dead=0;for(int st=PLAT_BLE_NOIMG;st<=PLAT_BLE_BADIMG;st++){state=st;plat_ble_peers(&p);assert(p.count==(st==PLAT_BLE_CONNECTED));}puts("actual snapshot bounded state/lock cases PASS");}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'t.c').write_text(h+f+t);subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(r/'firmware/src/app'),str(p/'t.c'),'-o',str(p/'t')],check=True);subprocess.run([str(p/'t')],check=True)
