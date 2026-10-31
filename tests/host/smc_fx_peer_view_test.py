#!/usr/bin/env python3
"""Native isolated regression of *current production* SMC peer UI functions.

Reads the real firmware/src/app/smc_mixer.c, extracts five complete production
functions, and compiles them unchanged with narrow fake UI/clock/peer state.
No reproduction from a manually copied function can count as this run.
"""
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(os.environ.get('SMC_PEER_UI_SOURCE', ROOT/'firmware/src/app/smc_mixer.c'))
OUT = Path(os.environ.get('SMC_PEER_UI_OUT', ROOT/'build/host/smc-fx-peer-ui'))
OUT.mkdir(parents=True,exist_ok=True)
if not SRC.is_file():
    sys.exit(f'BLOCKED: missing exact source {SRC}')
src = SRC.read_text()

def function(name):
    rx = re.compile(r'(?m)^(?:static\s+)?(?:int|void|uint32_t)\s+' + re.escape(name) + r'\([^\n]*\)\s*\{')
    match = rx.search(src)
    if not match:
        raise RuntimeError(f'Production function not found: {name}')
    i, depth = match.end(),1
    while depth:
        if i == len(src):
            raise RuntimeError('Unterminated production function: '+name)
        depth += (src[i]=='{')-(src[i]=='}')
        i+=1
    return src[match.start():i]

out = ["""/* Generated in temporary build directory from actual production smc_mixer.c. */
#define SMC_UI_MS 1200u
struct smc_ui_state {
    uint32_t until, context;
    uint8_t on, kind, strip, action, peer;
    char name[24], value[24];
};
static uint8_t smc_ui_raw[sizeof(struct smc_ui_state)] __attribute__((aligned(4)));
#define smc_ui (*((struct smc_ui_state *)(void *)smc_ui_raw))
/* No page peek (#737) in this harness: smc_ui_cancel() cancels nothing more. */
static void smc_peek_cancel(void) {}
"""]
for name in ['smc_held','smc_ui_context']+(['smc_ui_off'] if re.search(r'(?m)^static void smc_ui_off\(void\)$', src) else [])+['smc_ui_cancel','smc_ui_active','smc_ui_begin']:
    out.append(function(name)+'\n')
(OUT/'smc_fx_ui_extract.inc').write_text('\n'.join(out))
cmd = [os.environ.get('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra',
       '-Werror','-pedantic','-Ifirmware/src','-I'+str(OUT)]
if os.environ.get('SAN_FLAGS'):
    cmd += os.environ['SAN_FLAGS'].split()
cmd += ['tests/host/smc_fx_peer_view_test.c','-o',str(OUT/'test')]
print('COMPILING',*cmd,flush=True)
subprocess.run(cmd,cwd=ROOT,check=True)
subprocess.run([str(OUT/'test')],cwd=ROOT,check=True)
