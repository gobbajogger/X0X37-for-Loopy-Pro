"""Verify retained tests and the new combined replay in a Linux worker.
The replay ran on the supported macOS compiler. Do not repeat it in Linux.
"""
from pathlib import Path
import hashlib,json,platform,subprocess,sys
ROOT=Path.cwd()
SAVED=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text())
# The integration checker checks this private worktree's matching contribution inputs.
subprocess.run([sys.executable,'-B',str(ROOT/'evidence/complete-stage5/verify-integration.py')],cwd=ROOT,check=True)
env=read(OUT/'environment.json')
inputs=read(OUT/'replay-inputs.json')
for name,expected in inputs.items():
    p=Path(name)
    if name=='/usr/bin/cc' and platform.system()=='Linux':
        assert expected==env['host_cc']['sha256']=='5f593b5dfc0e9e583d326d25a27e1a268419d4ae9440866da0bd94096a58b76d'
        continue
    if not p.is_absolute():
        # Generated data and payloads remain outside Git. Source is checked in the private tree.
        p=(SAVED if name.startswith('build/') else ROOT)/p
    assert sha(p)==expected,name
replay=read(OUT/'replay.json')
r=replay['result']
assert r=={'packets':1268,'decoded':2616,'feedback':513,'link':1,'queue_empty':True,'perf_test':-1,'perf_phase':0,'help':0,'overlay':0}
assert json.loads((OUT/'replay.log').read_text())==r
assert replay['capture_sha256']=='a92f72f54c6aba7004e2c3a29759fa38818bb72a976486fb272b38c61d9d245a'
assert str(SAVED/'host/x0x_host.c') in (OUT/'replay.c').read_text()
for name,expected in read(OUT/'saved-inputs.json').items():
    assert sha(SAVED/name)==expected,name
assert sha(Path('/Users/mac/.jieli/toolchain/pi32v2/bin/clang'))==env['target_cc']['sha256']
print(json.dumps({'result':'PASS','replay_inputs':len(inputs),'combined_replay':r,'host_replay_repeated':False,'physical_result':'NOT TESTED'},indent=2))
