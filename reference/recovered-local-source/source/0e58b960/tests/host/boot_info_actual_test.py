#!/usr/bin/env python3
from pathlib import Path
import subprocess,os
root=Path(__file__).resolve().parents[2];out=root/'build/boot-info-test';out.mkdir(parents=True,exist_ok=True)
s=(root/'firmware/src/app/editor.c').read_text()
def func(name):
 at=s.rfind('\nstatic ',0,s.index(name+'('))+1;a=s.index('{',at);i=a+1;n=1
 while n:
  n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[at:i]
enum=s[s.index('enum { ED_INFO'):s.index('};',s.index('enum { ED_INFO'))+2]
pre='''#include <stdio.h>\n#include <string.h>\n#include "app/boot_screen.h"\n'''+enum+r'''
#define ED_READ_MAX 224
static uint8_t sector[4096],reply[1024],ed_smp_buf[256],scratch[3840];static unsigned nr,mutations;static uint32_t fm1_ms;static int flash_ok=1;static boot_stage_t bst;static boot_io_t bio;
static const uint8_t *boot_shown(void){return sector;}
static void ed_b(uint32_t x){reply[nr++]=x&127;}
static void ed_str(const char*s,uint32_t n){for(unsigned i=0;i<n&&s[i];i++)ed_b(s[i]);ed_b(0);}
static void ed_put7(const uint8_t*p,uint32_t n){(void)p;(void)n;}
static int ui_boot_flash_busy(void){return 0;}static int store_stage_held(void){return 0;}static uint8_t*store_stage_lock(uint32_t*c){*c=3840;mutations++;return scratch;}static void store_stage_unlock(void){}
static int boot_dev_erase(void*c,uint32_t o){(void)c;(void)o;mutations++;return 1;}static int boot_dev_prog(void*c,uint32_t o,const uint8_t*p,uint32_t n){(void)c;(void)o;(void)p;(void)n;mutations++;return 1;}static int boot_dev_read(void*c,uint32_t o,uint8_t*p,uint32_t n){(void)c;(void)o;(void)p;(void)n;return 1;}
static uint32_t ed_unpack_exact(const uint8_t*p,uint32_t n,uint8_t*d,uint32_t c){(void)p;(void)n;(void)d;(void)c;return 0;}
static void boot_finish(int rc){ed_b(rc);}
'''
test=r'''
#define C(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){uint8_t legacy[128];unsigned len;sector[32]=2;nr=0;ed_boot(ED_BOOT_INFO,0,0);len=nr;memcpy(legacy,reply,nr);C(reply[0]==1&&reply[1]==0);sector[32]=3;sector[33]=1;nr=0;ed_boot(ED_BOOT_INFO,0,0);C(nr==len&&!memcmp(legacy,reply,nr));sector[32]=1;sector[33]=1;sector[34]=1;sector[35]=1;sector[36]='A';sector[48]='B';nr=0;ed_boot(ED_BOOT_INFO,0,0);C(reply[0]==0&&reply[1]==1&&reply[2]=='A'&&reply[4]=='B');nr=0;ed_boot(ED_BOOT_CAPS,0,0);C(nr==29&&reply[0]==1);
#if BOARD_SMK37
C(reply[1]==1&&reply[2]==1&&reply[3]==1&&reply[4]==1);C((reply[14]|reply[15]<<7|reply[16]<<14)==3840);
#else
for(unsigned i=1;i<nr;i++)C(reply[i]==0);
#endif
C(!mutations);C(ED_INSTALL==48&&ED_BOOT_CAPS==49);printf("Actual boot INFO/capability kind compatibility PASS model=%d, zero mutations\n",BOARD_SMK37);return 0;}
'''
(out/'info.c').write_text(pre+func('ed_boot')+test)
for model in (0,1):
 cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g',f'-DBOARD_SMK37={model}','-fsanitize=address,undefined','-I'+str(root/'firmware/src'),str(out/'info.c'),'-o',str(out/f'info-{model}')];subprocess.run(cmd,check=True);subprocess.run([str(out/f'info-{model}')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
