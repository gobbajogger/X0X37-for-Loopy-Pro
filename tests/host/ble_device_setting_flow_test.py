#!/usr/bin/env python3
"""Actual project setting/UI save bodies over simulated storage, audio and time.

Checks first-safe-idle automatic saving, not durability during active audio.
--source-root permits the exact f550 negative control. No hardware access.
"""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[2])
a=p.parse_args();root=a.source_root
project=(root/'firmware/src/app/project.c').read_text();ui=(root/'firmware/src/app/ui.c').read_text();header=(root/'firmware/src/app/x0x.h').read_text()
def fn(text,name):
 m=re.search(r'^[^\n]*\b'+name+r'\([^\n{}]*\)[^;\n{}]*(?:\n\s*)?\{',text,re.M)
 if not m: return None
 i=m.end();depth=1
 while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[m.start():i]
settings=re.search(r'typedef struct \{\n    uint32_t magic, format;.*?\} settings_t;',header,re.S).group()
defines=header[header.index('#define PROJ_MAGIC'):header.index('typedef struct {',header.index('#define PROJ_MAGIC'))]
# Baseline has no automatic device writer; model its absent hook as no action.
names=['hash_of','get32','put32','save_buf','load_settings','save_obj']
functions='\n'.join(fn(project,n) for n in names)
defaults=fn(project,'project_settings_defaults')
if not defaults:
 # Extract the exact prior defaults assignments, without the rest of the project.
 body=fn(project,'project_defaults_for');start=body.index('    pr->set.magic');end=body.index('    engine_sound_defaults')
 defaults='static void project_settings_defaults(settings_t *s,uint8_t device_bt){\nmemset(s,0,sizeof *s);\n'+body[start:end].replace('pr->set.','s->').replace('    project_ck_check(pr, 1);','    memcpy(&s->ck_ch,CK_DEF,sizeof CK_DEF);')+'\n}'
functions=defaults+'\n'+functions
for name in ['project_device_bt_set','project_device_bt_pending','project_device_bt_save']:
 body=fn(project,name)
 if body:functions+='\n'+body
 else:functions+='\n'+{'project_device_bt_set':'void project_device_bt_set(int v){(void)v;}','project_device_bt_pending':'int project_device_bt_pending(void){return 0;}','project_device_bt_save':'int project_device_bt_save(void){return 0;}'}[name]
functions+='\n'+fn(ui,'ui_boot_flash_busy')+'\n'+(fn(ui,'device_bt_save') or 'static void device_bt_save(void){}')
case=re.search(r'    case R_BT:\n(.*?)\n        break;',ui,re.S).group(1)
functions+='\nstatic void toggle(int v){\n'+case+'\n}\n'
program=r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#define X0X_COLD
#define BOARD_SMK37 1
#define PLAT_STORE_MAX 128
#define OBJ_SET 0
#define OBJ_NOBJ 7
#define PLAT_BLE_RESTART 4
'''+defines+'\n'+settings+r'''
static const uint8_t CK_DEF[12]={2,0,0,0,0,0,9,0,4,0,0,0};
static struct {settings_t set;} proj;
static uint8_t io_buf[PLAT_STORE_MAX],disk[PLAT_STORE_MAX],old_disk[PLAT_STORE_MAX];
static uint64_t stored_hash[OBJ_NOBJ];static uint8_t stored_known[OBJ_NOBJ];
static int device_bt_pending,project_incompatible,disk_n=-1,held,stage,notes,perf,ck,project_busy,race,fail;
static unsigned saves,holds,checks;static uint32_t now=100;
static struct {unsigned playing,req_start,req_cont,req_stop;}seq;
static struct {unsigned rec;}ui;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int plat_store_load(unsigned obj,void *out,unsigned max){(void)obj;if(disk_n<0||disk_n>(int)max)return -1;memcpy(out,disk,disk_n);return disk_n;}
static int plat_store_save(unsigned obj,const void *in,unsigned n){(void)obj;if(!held)return -88;saves++;if(fail)return -1;memcpy(disk,in,n);disk_n=n;return 0;}
static int plat_ble_state(void){return 0;}
static void plat_ble_set(int on){(void)on;}
static void say(const char*a,const char*b){(void)a;(void)b;}
static int store_stage_held(void){return stage;}
static int engine_project_notes(void){return notes;}
static int perf_save_busy(void){return perf;}
static int perf_testing(void){return perf;}
uint8_t smk_ck_flash_busy(void){return ck;}
static uint32_t plat_ms(void){return now;}
static void engine_project_hold(int on){held=on;if(on){holds++;if(race)seq.req_start=1;}}
'''+functions+r'''
int main(void){
 (void)device_bt_pending;(void)project_incompatible; /* baseline lacks these production states */
 settings_t seed,loaded;project_settings_defaults(&seed,PROJ_BT_SMC_CENTRAL);
 memcpy(disk+4,&seed,sizeof seed);disk_n=sizeof seed+4;disk[0]=PROJ_FORMAT;
 memcpy(old_disk,disk,disk_n);proj.set=seed;proj.set.autosave_off=1;proj.set.bpm_x10=1700;
 toggle(1);CHECK(proj.set.bt&1);CHECK(project_device_bt_pending()); /* red: f550 has no writer */
 unsigned *transport[]={&seq.playing,&seq.req_start,&seq.req_cont,&seq.req_stop,&ui.rec};
 int *busy[]={&project_busy,&stage,&notes,&perf,&ck};
 for(unsigned i=0;i<5;i++){*transport[i]=1;device_bt_save();CHECK(!saves&&!held&&!holds&&project_device_bt_pending());CHECK(!memcmp(disk,old_disk,disk_n));*transport[i]=0;}
 for(unsigned i=0;i<5;i++){*busy[i]=1;device_bt_save();CHECK(!saves&&!held&&!holds&&project_device_bt_pending());*busy[i]=0;}
 race=1;device_bt_save();CHECK(!saves&&!held&&holds==1&&project_device_bt_pending());race=seq.req_start=0;
 device_bt_save();CHECK(saves==1&&!held&&!project_device_bt_pending());
 old_disk[4+offsetof(settings_t,bt)]|=1;CHECK(!memcmp(disk,old_disk,disk_n));
 memset(&loaded,0,sizeof loaded);CHECK(!load_settings(&loaded));CHECK((loaded.bt&1)&&loaded.bpm_x10==1250&&!loaded.autosave_off);CHECK(proj.set.bpm_x10==1700&&proj.set.autosave_off);
 toggle(1);device_bt_save();CHECK(saves==1);toggle(0);toggle(1);toggle(0);device_bt_save();CHECK(saves==2&&!(disk[4+offsetof(settings_t,bt)]&1));
 /* Restart/load reads the independently saved bit. */
 memset(&proj.set,0,sizeof proj.set);CHECK(!load_settings(&proj.set)&&!(proj.set.bt&1));
 toggle(1);fail=1;device_bt_save();CHECK(saves==3&&project_device_bt_pending()&&!(disk[4+offsetof(settings_t,bt)]&1));
 device_bt_save();CHECK(saves==3);now+=1000;fail=0;device_bt_save();CHECK(saves==4&&!project_device_bt_pending()&&(disk[4+offsetof(settings_t,bt)]&1));
 /* A short payload does not load here (load_settings wants the exact size): refuse it untouched; refuse unsupported bytes untouched. */
 disk_n=5+offsetof(settings_t,bt);memcpy(old_disk,disk,disk_n);toggle(0);device_bt_save();CHECK(project_device_bt_pending()&&disk_n==(int)(5+offsetof(settings_t,bt))&&!memcmp(disk,old_disk,disk_n));
 disk[0]++;memcpy(old_disk,disk,disk_n);toggle(1);device_bt_save();CHECK(project_device_bt_pending()&&!memcmp(disk,old_disk,disk_n));
 printf("PASS %u actual UI/project flow checks: autosave-off, byte-isolated save, busy/race no-hold, first-safe-idle, retry, reload and short/unknown payloads refused\n",checks);
}
'''
with tempfile.TemporaryDirectory(prefix='bt-device-flow-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(program)
 for tagged in (0,1):
  subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-DX0X_BASS_TAGGED_SOUND='+str(tagged),'-fsanitize=address,undefined','-fno-sanitize-recover=all',str(c),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
