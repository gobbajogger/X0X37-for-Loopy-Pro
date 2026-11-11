"""Run the saved independent probes against the repaired central source."""
from pathlib import Path
import subprocess,tempfile,json
R=Path(__file__).resolve().parents[2]; E=Path(__file__).resolve().parent; records=[]
def run(name,argv):
 p=subprocess.run([str(a) for a in argv],cwd=R,capture_output=True,text=True)
 output=p.stdout+p.stderr; (E/(name+'.log')).write_text(output)
 records.append(dict(name=name,argv=[str(a) for a in argv],cwd=str(R),returncode=p.returncode,expected=0,log=name+'.log'))
 (E/'saved-probes.json').write_text(json.dumps(records,indent=2)+'\n')
 assert p.returncode==0,(name,output)
flags=['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='saved-stop-probes-') as temp:
 w=Path(temp)
 for name,source,cases in [
  ('realistic-cancel','evidence/deadline-stage3/test/cancel-realistic-probe.c',[['clear'],['clear','enhanced'],['busy'],['busy','enhanced']]),
  ('event-edge','evidence/deadline-stage3/investigation/event-edge-probe.c',[[]])]:
  binary=w/name
  run(name+'-compile',flags+[source,'firmware/src/ble/ble_midi_central.c','-o',binary])
  for i,args in enumerate(cases): run(name+'-'+str(i),[binary]+args)
print('Saved realistic Cancel and 12 event-order cases: PASS')
