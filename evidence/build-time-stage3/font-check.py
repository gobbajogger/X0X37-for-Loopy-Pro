#!/usr/bin/env python3
"""Check only Linux font output through the checked worker class; no target build."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,tempfile,time
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import dev_validate as v
out=Path(__file__).resolve().parent
image='sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
with tempfile.TemporaryDirectory(prefix='font-stage3-',dir=root/'build') as tmp:
    scratch=Path(tmp)
    worker=v.LinuxWorker(image,[root,scratch],{str(scratch)},v.development_env(os.environ,'elite'),60)
    try:
        worker.start()
        argv=worker.argv(root,['python3','-B','tools/gen_font.py',str(scratch/'font.h')])
        start=time.monotonic()
        result=subprocess.run(argv,capture_output=True,text=True,timeout=60)
        elapsed=time.monotonic()-start
        (out/'font-check.log').write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError(result.stderr)
        actual=hashlib.sha256((scratch/'font.h').read_bytes()).hexdigest()
        audit=json.loads((root/'evidence/build-time-audit/generated-header-comparison.json').read_text())[0]
        runtime,provenance=worker.snapshot({'plan':{'checks':[]}})
        proof={'command':argv,'returncode':result.returncode,'command_seconds':elapsed,'image':image,
            'worker_environment':worker.env,'runtime':runtime,'provenance':provenance,
            'generated_header_sha256':actual,'audit_linux_sha256':audit['linux_sha256'],
            'equal':actual==audit['linux_sha256'],'log_sha256':hashlib.sha256((out/'font-check.log').read_bytes()).hexdigest(),
            'scope':'Only the font generator; no target artifacts, full validation or speed claim.'}
        (out/'font-check.json').write_text(json.dumps(proof,indent=2)+'\n')
        print(json.dumps({k:proof[k] for k in ('returncode','command_seconds','generated_header_sha256','equal')}))
        assert proof['equal']
    finally:
        worker.close()
