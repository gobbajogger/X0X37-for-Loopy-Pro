"""Confirm the deadline invariant through stop and connection-race paths."""
from pathlib import Path
import json,subprocess,tempfile
R=Path(__file__).resolve().parents[3]
E=Path(__file__).resolve().parent
records=[]
def run(name,argv,expected):
 p=subprocess.run([str(x) for x in argv],cwd=R,capture_output=True,text=True)
 (E/(name+'.log')).write_text(p.stdout+p.stderr)
 records.append(dict(name=name,argv=[str(x) for x in argv],cwd=str(R),returncode=p.returncode,expected=expected,log=name+'.log'))
 (E/'deadline-stop-record.json').write_text(json.dumps(records,indent=2)+'\n')
 assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
with tempfile.TemporaryDirectory(prefix='abort-deadline-stop-') as s:
 binary=Path(s)/'deadline-stop'
 run('deadline-stop-compile',['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all',E/'deadline-stop-probe.c','firmware/src/ble/ble_midi_central.c','-o',binary],0)
 for path in ['stop','service','cancel']:
  for when in ['exact','after']:
   for wrap in [False,True]:
    name='deadline-'+path+'-'+when+('-wrap' if wrap else '')
    run(name,[binary,path,when]+(['wrap'] if wrap else []),-6)
print('All 12 deadline probes reproduce the functional failure.')
