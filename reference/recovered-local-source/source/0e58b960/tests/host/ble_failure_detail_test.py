#!/usr/bin/env python3
"""Read-only failure classification from actual app code with simulated state.
No RF/device access. A missing-query baseline returns no additional detail.
"""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[2])
p.add_argument('--support-root',type=Path,required=True)
a=p.parse_args();s=(a.source_root/'firmware/src/ble.c').read_text()
def fn(name):
 m=re.search(r'^[^\n]*\b'+name+r'\([^\n{}]*\)[^;\n{}]*(?:\n\s*)?\{',s,re.M)
 if not m:return None
 i=m.end();d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[m.start():i]
query=fn('plat_ble_failure_detail') or 'uint32_t plat_ble_failure_detail(void){return 0;} /* original status has no failure detail */'
program=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"
#include "ble_midi_central.c"
#include "ble_glue.h"
#include "ble_failure.h"
enum {PLAT_BLE_NOIMG,PLAT_BLE_OFF,PLAT_BLE_RESTART,PLAT_BLE_STARTING,PLAT_BLE_READY,PLAT_BLE_CONNECTED,PLAT_BLE_FAILED,PLAT_BLE_BADIMG};
static struct {xb_mbox_t mb;union {bleh_t h;struct{blec_t c,c2;};}host;}ble_r;
static uint8_t ble_img,ble_want,ble_boot_on,ble_up,ble_host_on,ble_dead,ble_blocked,ble_c0_ready,ble_central_role,ble_role_changing;
static unsigned desired,fm1_c1_on,irqdepth,checks;
static ble_guard_t ble_guard;
static unsigned smc_ble_role_get(void){return desired;}
static uint32_t irq_save(void){assert(!irqdepth);irqdepth++;return 0;}
static void irq_restore(uint32_t f){(void)f;assert(irqdepth==1);irqdepth--;}
'''+fn('plat_ble_state')+'\n'+query+r'''
static uint32_t snapshot(void){
 unsigned char before[sizeof ble_r];memcpy(before,&ble_r,sizeof before);
 ble_guard_t g=ble_guard;uint32_t d=plat_ble_failure_detail();
 assert(!memcmp(before,&ble_r,sizeof before)&&!memcmp(&g,&ble_guard,sizeof g)&&!irqdepth);return d;
}

#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void){memset(&ble_r,0,sizeof ble_r);memset(&ble_guard,0,sizeof ble_guard);ble_img=BLE_IMG_OK;ble_want=ble_boot_on=ble_up=ble_host_on=ble_c0_ready=ble_central_role=desired=fm1_c1_on=1;ble_dead=ble_blocked=ble_role_changing=0;ble_r.mb.state=XB_ST_READY;}
static unsigned phase(uint32_t d){return d>>24;}
int main(void){
 reset();ble_blocked=ble_dead=1;ble_guard.why=BLE_WHY_RESET;
 CHECK(plat_ble_state()==PLAT_BLE_FAILED);uint32_t d=snapshot();CHECK(phase(d)==BLE_FAIL_GUARD&&((d>>16)&255)==255&&((d>>8)&255)==BLE_FAIL_CODE_GUARD&&(d&255)==BLE_WHY_RESET&&!irqdepth);
 reset();ble_dead=1;ble_c0_ready=0;ble_r.mb.err=XB_E_BOOT;d=snapshot();CHECK(phase(d)==BLE_FAIL_INIT&&(d&255)==XB_E_BOOT);
 reset();ble_dead=1;ble_c0_ready=0;ble_guard.why=BLE_WHY_NOT_READY;d=snapshot();CHECK(phase(d)==BLE_FAIL_INIT&&((d>>8)&255)==BLE_FAIL_CODE_GUARD&&(d&255)==BLE_WHY_NOT_READY);
 reset();ble_dead=1;fm1_c1_on=0;CHECK(phase(snapshot())==BLE_FAIL_CORE1);
 reset();ble_dead=ble_role_changing=1;CHECK(phase(snapshot())==BLE_FAIL_ROLE);
 reset();ble_dead=1;ble_r.mb.err=XB_E_BOOT;d=snapshot();CHECK(phase(d)==BLE_FAIL_CONTROLLER&&((d>>8)&255)==BLE_FAIL_CODE_MAILBOX&&(d&255)==XB_E_BOOT);
 const unsigned steps[]={S_RESET,S_EVENT,S_LE_EVENT,S_BUFFER,S_RANDOM,S_SCAN_PARAM,S_SCAN_ON,S_SCAN_OFF,S_CREATE,S_CANCEL,S_SERVICE,S_CHAR,S_CCCD,S_SUBSCRIBE,S_READY};
 const unsigned want[]={BLE_FAIL_HOST_START,BLE_FAIL_HOST_START,BLE_FAIL_HOST_START,BLE_FAIL_HOST_START,BLE_FAIL_HOST_START,BLE_FAIL_SCAN,BLE_FAIL_SCAN,BLE_FAIL_SCAN,BLE_FAIL_CONNECT,BLE_FAIL_CONNECT,BLE_FAIL_SERVICE,BLE_FAIL_CHAR,BLE_FAIL_CCCD,BLE_FAIL_SUBSCRIBE,BLE_FAIL_LINK};
 for(unsigned i=0;i<sizeof steps/sizeof *steps;i++){
  reset();ble_r.host.c.state=BLEC_FAILED;ble_r.host.c.step=steps[i];
  CHECK(plat_ble_state()==PLAT_BLE_FAILED);d=snapshot();CHECK(phase(d)==want[i]&&((d>>16)&255)==0&&!(d&65535)); /* no discarded HCI/ATT code invented */
  ble_r.host.c2.state=BLEC_CONNECTED;CHECK(plat_ble_state()==PLAT_BLE_CONNECTED&&!snapshot());
 }
 reset();ble_r.host.c.state=BLEC_FAILED;ble_r.host.c.step=S_CHAR;ble_r.host.c2.state=BLEC_FAILED;ble_r.host.c2.step=S_SUBSCRIBE;d=snapshot();CHECK(phase(d)==BLE_FAIL_CHAR); /* defined first-failed-peer priority, no blend */
 reset();ble_r.host.c.state=BLEC_FAILED;ble_r.host.c.step=S_SUBSCRIBE;ble_r.host.c.disconnect_flags=D_SENT;d=snapshot();CHECK(phase(d)==BLE_FAIL_STOP&&((d>>8)&255)==BLE_FAIL_CODE_STEP&&(d&255)==S_SUBSCRIBE);
 reset();ble_central_role=desired=0;ble_r.host.h.state=BLEH_ERROR;CHECK(phase(snapshot())==BLE_FAIL_HOST_START);
 reset();CHECK(plat_ble_state()==PLAT_BLE_READY&&!snapshot());
 ble_want=0;ble_dead=ble_blocked=1;CHECK(plat_ble_state()==PLAT_BLE_OFF&&!snapshot());
 ble_img=BLE_IMG_NONE;CHECK(plat_ble_state()==PLAT_BLE_NOIMG&&!snapshot());
 ble_img=BLE_IMG_BAD;CHECK(plat_ble_state()==PLAT_BLE_BADIMG&&!snapshot());
 reset();ble_host_on=0;CHECK(plat_ble_state()==PLAT_BLE_STARTING&&!snapshot());
 CHECK(!irqdepth);puts("PASS read-only failure phases: guard/init/core1/role/controller and all host steps, retained code types, successful/OFF/image negatives");printf("%u checks\n",checks);
}
'''
with tempfile.TemporaryDirectory(prefix='ble-failure-detail-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(program)
 subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(a.source_root/'firmware/src/ble'),'-I'+str(Path(__file__).resolve().parents[2]/'firmware/src/ble'),'-I'+str(a.support_root/'firmware/src/ble'),'-I'+str(a.support_root/'firmware/hal'),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
