#!/usr/bin/env python3
"""Measure unchanged generator entrypoints; no target or simulator build."""
from pathlib import Path
import hashlib,json,os,platform,subprocess,sys,time
root=Path.cwd(); out=root/'build/cache-consumer-baseline'; out.mkdir(parents=True,exist_ok=True)
rows=[]
for kind,argv in [
 ('font',[sys.executable,'tools/gen_font.py',str(out/'felucca_font.h')]),
 ('drums',[sys.executable,'tools/gen_drum_samples.py',str(out/'x0x_drum_samples.h')]),
 ('breaks',['sh','tools/gen_builtin_break.sh',str(out)])]:
 start=time.perf_counter()
 result=subprocess.run(argv,capture_output=True,text=True)
 elapsed=time.perf_counter()-start
 (out/(kind+'.log')).write_text(result.stdout+result.stderr)
 if result.returncode: raise SystemExit(kind+' failed: '+str(result.returncode))
 rows.append({'kind':kind,'argv':argv,'seconds':elapsed,'returncode':result.returncode,'log':str(out/(kind+'.log'))})
files={}
for folder in ('tools','assets','firmware/src'):
 for file in sorted((root/folder).rglob('*')):
  if file.is_file() and '__pycache__' not in file.parts:
   files[str(file.relative_to(root))]=hashlib.sha256(file.read_bytes()).hexdigest()
result={'scope':'Unchanged generator operations only; not a full host/test/firmware timing.',
 'source_revision':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
 'image':os.environ.get('MEASURE_IMAGE_ID'),'platform':platform.platform(),
 'environment':{k:v for k,v in os.environ.items() if k.startswith(('X0X','PYTHON','LD_','LC_')) or k in ('PATH','CC','LANG','TZ')},
 'rows':rows,'source_files':files,
 'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.h')},
 'limitations':['One observation per generator; concurrent host activity may affect elapsed time.','Pinned immutable Linux image supplies the tool and dependency bytes; no independent whole runtime manifest is claimed.']}
Path('evidence/cache-consumer-audit/original-linux.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'rows':rows,'outputs':result['outputs']},indent=2))
