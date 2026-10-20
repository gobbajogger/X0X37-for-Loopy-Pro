#!/usr/bin/env python3
"""Compare recorded MIDI delivery and screen-target selection without a device.

Read-only Git inputs; generated sources and executables stay in scratch space.
Recorded notification bytes are exact, but HCI framing and link state are simulated.
The mapping probe extracts production functions and substitutes page storage only.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / 'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json'
REVISIONS = ['53d5654', '91356e7', '39db004']


def source(revision, path):
    return subprocess.check_output(['git', '-C', str(ROOT), 'show', f'{revision}:{path}'], text=True)


def function(text, name):
    import re
    match = re.search(r'^static [^\n]*\b' + name + r'\([^\n]*\)\n\{', text, re.M)
    assert match, name
    start = text.index('{', match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


MIDI = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.c"
static unsigned count;
static int send_h4(uint8_t t,const uint8_t *p,uint16_t n,void *x)
{ (void)t;(void)p;(void)n;(void)x;return 0; }
static void got(const uint8_t *p,uint8_t n,void *x)
{ (void)x;count++;for(unsigned i=0;i<n;i++)printf("%02x",p[i]);puts(""); }
int main(int argc,char **argv) {
    (void)argv;blec_t c;uint8_t addr[6]={0},h[2],p[512],b[520];
    blec_init(&c,send_h4,got,0,addr);c.state=BLEC_CONNECTED;c.conn=1;c.midi_handle=113;
    unsigned index=0;
    while(fread(h,1,2,stdin)==2) {
        unsigned n=h[0]|(unsigned)h[1]<<8;if(n>sizeof p||fread(p,1,n,stdin)!=n)return 2;
        unsigned len=n+7,tail=argc>1?++index%9:0;
        memset(b,0,sizeof b);b[0]=1;b[2]=len-tail;b[4]=n+3;b[6]=4;
        b[8]=0x1b;b[9]=113;memcpy(b+11,p,n);
        blec_rx(&c,2,b,len+4-tail,1);
        if(tail) {
            uint8_t end[16]={1,0x10,0,0};end[2]=tail;memcpy(end+4,b+len+4-tail,tail);
            blec_rx(&c,2,end,tail+4,1);
        }
    }
    fprintf(stderr,"%u\n",count);return 0;
}
'''

MAPPING = r'''
#include <stdint.h>
#include <stdio.h>
typedef struct {uint8_t kind,a,b,c;} pref_t;
enum {R_NONE,R_ENG,V_HOME,V_GLO,V_PART,O_NONE,O_LIST,O_ASK,T_MIX,T_FX,T_MST,T_909,T_808,MX_LEVEL,FX_RV_LEVEL,FX_DL_LEVEL,FX_VOLUME,MST_DJF,NDRUM=11,NPARTS=5};
#define PR(k,a,b,c) ((pref_t){k,a,b,c})
#define NONE PR(0,0,0,0)
static struct {int view,overlay,help,list_top,list_n,part,sel[2];} ui;
static struct {int n;pref_t r[2][4];} pg;
static pref_t list_rows[6];
static int cur_page(void){return 0;}
static int smk_clip_mode_on(void){return 0;}
static int drum_mix_page(void){return -1;}
static pref_t find_ref(int a,int b,const char *s){(void)s;return PR(R_ENG,a,b,MX_LEVEL);}
'''

out = {'capture_sha256': hashlib.sha256(CAPTURE.read_bytes()).hexdigest(), 'revisions': {}}
packets = [bytes.fromhex(x['hex']) for x in json.loads(CAPTURE.read_bytes())]
supported = [p for p in packets if len(p)+3 <= 64]
wire = b''.join(len(p).to_bytes(2,'little')+p for p in supported)
expected = [bytes((p[2],*p[i:i+2])).hex() for p in supported for i in range(3,len(p),2)]
assert len(packets)==1271 and len(expected)==2616
with tempfile.TemporaryDirectory() as tmp:
    for revision in REVISIONS:
        path = Path(tmp)/revision
        path.mkdir()
        for name in ['ble_midi_central.c','ble_midi_central.h']:
            (path/name).write_text(source(revision,'firmware/src/ble/'+name))
        (path/'probe.c').write_text(MIDI)
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(path/'probe.c'),'-o',str(path/'probe')],check=True)
        results = {}
        for label,args in [('whole',[]),('fragment_tails_1_to_8',['fragment'])]:
            r = subprocess.run([str(path/'probe'),*args],input=wire,capture_output=True,check=True)
            actual = r.stdout.decode().splitlines()
            results[label] = {'midi_count':len(actual),'expected':len(expected),'exact_match':actual==expected,'output_sha256':hashlib.sha256(r.stdout).hexdigest()}
        text = source(revision,'firmware/src/app/smc_mixer.c')
        names = ['smc_home_target','smc_page_target','smc_target']
        target = MAPPING+'\n'+'\n'.join(function(text,n) for n in names)
        target += r'''
int main(void) {
    ui.view=V_PART;pg.n=2;
    for(int p=0;p<2;p++)for(int n=0;n<4;n++)pg.r[p][n]=PR(R_ENG,T_909,0,40+p*4+n);
    pref_t a=smc_target(0),b=pg.r[0][0],c=smc_target(4),d=pg.r[1][0];
    printf("%d %d\n",a.kind==b.kind&&a.a==b.a&&a.b==b.b&&a.c==b.c,c.kind==d.kind&&c.a==d.a&&c.b==d.b&&c.c==d.c);
    return 0;
}
'''
        (path/'mapping.c').write_text(target)
        subprocess.run(['cc','-std=c99','-Wno-unused-function',str(path/'mapping.c'),'-o',str(path/'mapping')],check=True)
        mapping = subprocess.check_output([str(path/'mapping')],text=True).strip().split()
        results['sound_page_fader1_follows_visible_field'] = mapping[0]=='1'
        results['sound_page_fader5_follows_next_page_field'] = mapping[1]=='1'
        out['revisions'][revision] = results
assert out['revisions']['53d5654']['whole']['exact_match']
assert out['revisions']['39db004']['whole']['exact_match']
assert out['revisions']['39db004']['fragment_tails_1_to_8']['exact_match']
assert out['revisions']['53d5654']['sound_page_fader1_follows_visible_field']
assert not out['revisions']['91356e7']['sound_page_fader1_follows_visible_field']
assert not out['revisions']['39db004']['sound_page_fader1_follows_visible_field']
print(json.dumps(out,indent=2))
