import subprocess,os,json,time
from pathlib import Path
root=Path('/workspace/nightly-browser-repair')
evidence=Path('/workspace/nightly-repair-evidence/r5')
qa=Path('/workspace/nightly-repair-evidence/docs/independent-qa/pr8-r3')
env=dict(os.environ,PR8_QA_SOURCE=str(root))
commands=[
 ('npm-build',['npm','run','build']),
 ('npm-test',['npm','test']),
 ('default-browser',['npm','run','test:browser']),
 ('final-matrix-green',['python3',str(evidence/'final_matrix_extension.py')]),
]

results=[]
for name,cmd in commands:
 start=time.monotonic()
 with (evidence/(name+'.log')).open('w') as out:r=subprocess.run(cmd,cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=240)
 row={'name':name,'command':cmd,'exit':r.returncode,'seconds':round(time.monotonic()-start,2)}
 results.append(row);print(json.dumps(row),flush=True)
 (evidence/'consolidated-commands.json').write_text(json.dumps(results,indent=2)+'\n')
 if r.returncode:raise SystemExit(r.returncode)
