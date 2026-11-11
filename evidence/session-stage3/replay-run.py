"""Replay unchanged capture payloads through production startup, mailbox, UI and feedback.
The synthetic peer also requests parameters during discovery. No device access.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
PRIOR = ROOT.parent / 'smc_working_source_regression_evidence_20261021'
BUILD = ROOT / 'build/session-stage3'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('prior', PRIOR / 'compare_startup.py')
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)
source = prior.PRE.replace('HOST_SOURCE', str(ROOT / 'host/x0x_host.c'))
ble = (ROOT / 'firmware/src/ble.c').read_text()
for name in ('ble_central_midi', 'ble_central_link_notify', 'ble_central_service', 'plat_ble_smc_send'):
    source += prior.extract(ble, name) + '\n'
peer = prior.PEER.replace(
    'incoming(4,conn,sizeof conn);',
    '''incoming(4,conn,sizeof conn);
 expect_tx(2,0x10);
 const uint8_t update[]={0x50,0x20,16,0,12,0,5,0,0x12,0xab,8,0,6,0,12,0,0,0,0x90,1};
 incoming(2,update,sizeof update);
 uint8_t t,reply[40];int rn=xb_frame_get(&ble_r.mb.h2c,&t,reply,sizeof reply);
 const uint8_t want[]={0x50,0,10,0,6,0,5,0,0x13,0xab,2,0,1,0};
 assert(t==2 && rn==sizeof want && !memcmp(reply,want,sizeof want));credit();''')
peer = peer.replace('response(gap,sizeof gap,0x10);', 'credit();peer_att(gap,sizeof gap);')
source += peer
(OUT / 'replay.c').write_text(source)
capture = ROOT / 'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json'
assert hashlib.sha256(capture.read_bytes()).hexdigest() == 'a92f72f54c6aba7004e2c3a29759fa38818bb72a976486fb272b38c61d9d245a'
payloads = [bytes.fromhex(row['hex']) for row in json.loads(capture.read_text())]
binary = b''.join(len(p).to_bytes(2, 'little') + p for p in payloads if len(p) <= 61)
(BUILD / 'payloads.bin').write_bytes(binary)
shutil.copyfile(PRIOR / 'working-source-store.bin', BUILD / 'store.bin')
units = ['dsp/drum909.c', 'dsp/drum808.c', 'dsp/bass303.c', 'dsp/breaks.c',
         'dsp/fxbus.c', 'dsp/master.c', 'seq/sequencer.c', 'seq/tb3po.c',
         'seq/pattern.c', 'seq/motion.c', 'app/engine.c']
cmd = ['cc', '-O1', '-g', '-ffp-contract=off', '-std=c99', '-Wall', '-Wextra',
       '-Wno-unused-function', '-Wno-unused-parameter', '-DX0X_HOST', '-D_DARWIN_C_SOURCE',
       '-Ifirmware/src', '-Ifirmware/src/dsp', '-Ifirmware/hal', '-Ibuild/gen', str(OUT / 'replay.c')]
cmd += ['firmware/src/' + unit for unit in units] + ['-lm', '-o', str(BUILD / 'replay')]
with (OUT / 'replay-compile.log').open('w') as log:
    subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
run = subprocess.run([str(BUILD / 'replay'), str(BUILD / 'payloads.bin'), str(BUILD / 'store.bin')],
                     cwd=ROOT, capture_output=True, text=True)
(OUT / 'replay.log').write_text(run.stdout + run.stderr)
assert run.returncode == 0, run.stderr
record = dict(compile=cmd, capture_sha256=hashlib.sha256(capture.read_bytes()).hexdigest(),
              harness_sha256=hashlib.sha256((PRIOR / 'compare_startup.py').read_bytes()).hexdigest(),
              store_sha256=hashlib.sha256((BUILD / 'store.bin').read_bytes()).hexdigest(),
              result=json.loads(run.stdout),
              limits=['Simulated peer, HCI framing and scheduling.',
                      '1268 exact payloads. Three ATT packets above 64 bytes are excluded.',
                      'The saved project is synthetic. No target interrupt timing or physical radio test.'])
(OUT / 'replay.json').write_text(json.dumps(record, indent=2) + '\n')
print(run.stdout, end='')
