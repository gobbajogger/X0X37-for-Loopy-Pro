#!/usr/bin/env python3
"""GPL-3.0-only. Native host RCH sweep from exact PR #93 C/H source; NEVER flashes hardware."""
import argparse
import hashlib
import json
import random
import re
import shutil
import statistics
import subprocess
import tempfile
from pathlib import Path

EXPECT_C = '0fcdbe65b674dea3becb26fc265cb2514908f1a5'
EXPECT_H = '6cb71d336c2c90fdc60a9594c2356f3a4379c264'
LEVELS = (512, 256, 128)
LENGTHS = (4096, 176400, 2116800)
COMPILERS = ('gcc', 'clang')

def blobhash(path):
    data = Path(path).read_bytes()
    return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()

def run(cmd, cwd=None, env=None):
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, env=env)
    if p.returncode:
        raise RuntimeError(f'exit {p.returncode}: {cmd}\n{p.stdout[-1500:]}\n{p.stderr[-1500:]}')
    return p.stdout, p.stderr

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path.cwd(), help='Git-exact PR93 source checkout/projection root')
    parser.add_argument('--output', type=Path, default=Path.cwd()/'rch_sweep_evidence.json', help='Existing file is never overwritten')
    parser.add_argument('--trials', type=int, default=3)
    parser.add_argument('--skip-adversarial', action='store_true', help='Use only when separately proven R6 adversarial PASS for the same C/H variants')
    args = parser.parse_args()
    root = args.source.resolve()
    assert args.trials >= 1 and args.trials <= 10
    c = root/'firmware/src/dsp/breaks.c'
    h = root/'firmware/src/dsp/breaks.h'
    if blobhash(c) != EXPECT_C or blobhash(h) != EXPECT_H:
        raise SystemExit(f'WRONG SOURCE: breaks.c {blobhash(c)} breaks.h {blobhash(h)}')
    if args.output.exists():
        raise SystemExit(f'REFUSE overwrite: {args.output}')
    benchmark = Path(__file__).parent/'reverse_steady_benchmark.c'
    checks = []
    cases = []
    with tempfile.TemporaryDirectory(prefix='pr93-rch-sweep-') as temp:
        tmp = Path(temp)
        for rch in LEVELS:
            part = tmp/str(rch)
            shutil.copytree(root/'firmware', part/'firmware')
            tests = part/'tests/host'
            tests.mkdir(parents=True)
            for name in ('run_breaks_renderer_adversarial.sh', 'breaks_renderer_adversarial_test.c'):
                shutil.copy2(root/'tests/host'/name, tests/name)
            if rch != 512:
                hdr = part/'firmware/src/dsp/breaks.h'
                src = hdr.read_text()
                old='#define BRK_RCH 512'
                if src.count(old) != 1:
                    raise RuntimeError(f'Missing unique source macro: {hdr}')
                hdr.write_text(src.replace(old, f'#define BRK_RCH {rch}', 1))
            if not args.skip_adversarial:
                out, err = run(['bash', 'tests/host/run_breaks_renderer_adversarial.sh'], cwd=part)
                assert out.count('ADVERSARIAL_RENDER_PASS') == 4, out
                checks.append({'type': 'adversarial', 'rch': rch, 'exit': 0, 'stdout': out.strip(), 'stderr': err.strip()})
            for cc in COMPILERS:
                exe = tmp/f'micro-{cc}-{rch}'
                cmd=[cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic','-ffp-contract=off',f'-I{part}',str(benchmark),str(part/'firmware/src/dsp/breaks.c'),'-o',str(exe)]
                out,err=run(cmd)
                checks.append({'type': 'compile', 'command': cmd, 'exit': 0, 'stdout': out.strip(), 'stderr': err.strip()})
        for direction,lens in [('reverse',LENGTHS),('forward',(176400,))]:
            for n in lens:
                pairs=[(cc,rch) for cc in COMPILERS for rch in LEVELS]
                for trial in range(args.trials):
                    random.Random(trial+1039).shuffle(pairs)
                    for cc,rch in pairs:
                        cmd=[str(tmp/f'micro-{cc}-{rch}'),direction,str(n)]
                        out,err=run(cmd)
                        vals=dict(re.findall(r'([a-z_]+)=([^\s]+)',out))
                        cases.append({'compiler':cc,'rch':rch,'direction':direction,'n':n,'trial':trial,'time_ns':int(vals['time_ns']),'sizeof_breaks':int(vals['sizeof_breaks']),'hash':vals['hash'],'exit':0,'command':cmd[1:]})
        for direction,lens in [('reverse',LENGTHS),('forward',(176400,))]:
            for n in lens:
                hashes={r['hash'] for r in cases if r['direction']==direction and r['n']==n}
                if len(hashes)!=1:
                    raise RuntimeError(f'PCM mismatch {direction} {n}: {hashes}')
        summary=[]
        for direction,lens in [('reverse',LENGTHS),('forward',(176400,))]:
            for n in lens:
                for cc in COMPILERS:
                    base=statistics.median(x['time_ns'] for x in cases if x['direction']==direction and x['n']==n and x['compiler']==cc and x['rch']==512)
                    for rch in LEVELS:
                        row=[x for x in cases if x['direction']==direction and x['n']==n and x['compiler']==cc and x['rch']==rch]
                        times=[x['time_ns'] for x in row]
                        med=statistics.median(times)
                        summary.append({'direction':direction,'n':n,'compiler':cc,'rch':rch,'trials':len(times),'median_ms':round(med/1e6,3),'delta_vs512_percent':round(100*(med/base-1),2),'host_struct_bytes':row[0]['sizeof_breaks'],'pcm_hash':row[0]['hash']})
    doc={'evidence_revision':'performance-pr93-rch-buffer-host-tradeoff-20261024-r7',
      'scope':'HOST-ONLY Git-blob exact breaks.c/h; experimental macro changes in ephemeral copies; not target RAM/MCU/ISR or independent QA',
      'source':{'repository':'gobbajogger/X0X37-for-Loopy-Pro','full_commit':'1074d667c106472bb3f05db571abb4b9fea13ceb','root_tree':'4f6c4734f20a33d79e248860622ef77689008fa7','firmware_tree':'a87a83e15116422eed84528d95f52ddef5029927','breaks_c_blob':EXPECT_C,'breaks_h_blob':EXPECT_H},
      'compiler_flags':'-std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off','adversarial_executed':not args.skip_adversarial,'checks':checks,'cases':cases,'summary':summary,
      'decision':'REJECT cache reductions at unchanged decoder strategy: materially slower worst long loops, despite host size reduction; do not edit firmware/source or compromise 8192-byte target reserve',
      'unrun':['new Elite and Pro builds of experimental configurations','actual target static RAM/pool/flash','MCU audio ISR/XRUN and owner device','independent QA for PR93']}
    args.output.write_text(json.dumps(doc,indent=2)+'\n')
    print('HOST_SWEEP_PASS',len(cases),'samples; adversarial_executed',not args.skip_adversarial,'output',args.output)
    for x in summary:
        if x['direction']=='reverse' and x['n']==2116800:
            print(x['compiler'],'long-loop RCH',x['rch'],'median_ms',x['median_ms'],'delta%',x['delta_vs512_percent'])

if __name__=='__main__':main()
