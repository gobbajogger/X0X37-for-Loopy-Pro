"""Run independent event-order probes with simulated HCI, clock and transport."""
from pathlib import Path
import subprocess,json,tempfile
R=Path(__file__).resolve().parents[3]; E=Path(__file__).resolve().parent; records=[]
with tempfile.TemporaryDirectory(prefix='central-event-edge-') as scratch:
 binary=Path(scratch)/'probe'
 for name,argv in [('event-edge-compile',['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(E/'event-edge-probe.c'),'firmware/src/ble/ble_midi_central.c','-o',str(binary)]),('event-edge-run',[str(binary)])]:
  p=subprocess.run(argv,cwd=R,capture_output=True,text=True)
  (E/(name+'.log')).write_text(p.stdout+p.stderr)
  records.append(dict(name=name,argv=argv,cwd=str(R),returncode=p.returncode,expected=0,log=name+'.log'))
  (E/'event-edge-record.json').write_text(json.dumps(records,indent=2)+'\n')
  assert p.returncode==0,(name,p.stdout,p.stderr)
print('12 independent event-order and stop-intent cases: PASS')
