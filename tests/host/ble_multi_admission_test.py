#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the real boot target setter: two exact, distinct, immutable slots."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'firmware/src/ble.c').read_text();start=s.index('int plat_ble_smc_peer_target(');end=s.index('\n}',start)+2
fixture='''#include <stdint.h>
#define BLEC_PEERS 2u
static uint8_t ble_known_target[BLEC_PEERS][6],ble_known_type[BLEC_PEERS],ble_known_allowed[BLEC_PEERS],ble_config_closed;
'''+s[start:end]+'''
int main(void) {
 uint8_t a[6]={1,2,3,4,5,6},b[6]={6,5,4,3,2,1};
 if(!plat_ble_smc_peer_target(1,b,1))return 1;
 if(plat_ble_smc_peer_target(0,b,1))return 2;
 if(!plat_ble_smc_peer_target(0,a,1))return 3;
 if(plat_ble_smc_peer_target(2,a,1))return 4;
 if(plat_ble_smc_peer_target(0,a,2))return 5;
 ble_config_closed=1;if(plat_ble_smc_peer_target(0,b,0))return 6;
 for(unsigned j=0;j<6;j++)if(ble_known_target[0][j]!=a[j]||ble_known_target[1][j]!=b[j])return 7;
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ble-multi-admission-') as tmp:
 p=Path(tmp);(p/'admission.c').write_text(fixture)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'admission.c'),'-o',str(p/'admission')],check=True)
 subprocess.run([str(p/'admission')],check=True)
print('multi-peer production boot target setter: 7 scenarios PASS')
