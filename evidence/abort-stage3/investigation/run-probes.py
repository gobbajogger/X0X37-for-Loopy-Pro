"""Run independent HCI investigation probes in scratch folders."""
from pathlib import Path
import hashlib,json,platform,shutil,subprocess,tempfile
R=Path(__file__).resolve().parents[3]
E=Path(__file__).resolve().parent
records=[]
def run(name,argv,cwd,expected):
 p=subprocess.run([str(x) for x in argv],cwd=cwd,capture_output=True,text=True)
 (E/(name+'.log')).write_text(p.stdout+p.stderr)
 record=dict(name=name,argv=[str(x) for x in argv],cwd=str(cwd),returncode=p.returncode,expected=expected,log=name+'.log')
 records.append(record)
 (E/'commands.json').write_text(json.dumps(records,indent=2)+'\n')
 assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
flags=['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='abort-investigation-') as scratch:
 for rev,commit in [('broken','28d0fc230f60bc823c1e5e73d3aed61e68a8e32f'),('candidate','05eff604cb3f136db71727a2aeeea785cab73b99')]:
  w=Path(scratch)/rev
  for sub in ['firmware/src/ble','tests/host','evidence/session-stage3']:(w/sub).mkdir(parents=True,exist_ok=True)
  for name in ['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h']:
   (w/name).write_bytes(subprocess.check_output(['git','show',commit+':'+name],cwd=R))
  for name in ['tests/host/ble_midi_signal_test.c','evidence/session-stage3/investigation-abort-probe.c','evidence/session-stage3/test-busy-disconnect.c']:shutil.copyfile(R/name,w/name)
  for name,file,modes,failed in [('saved-investigation','evidence/session-stage3/investigation-abort-probe.c',['overflow','timeout'],-6),('saved-test','evidence/session-stage3/test-busy-disconnect.c',[''],1)]:
   binary=w/name
   run(rev+'-'+name+'-compile',flags+[file,'firmware/src/ble/ble_midi_central.c','-o',binary],w,0)
   for mode in modes:run(rev+'-'+name+'-'+(mode or 'both'),[binary]+([mode] if mode else []),w,failed if rev=='broken' else 0)
 binary=Path(scratch)/'edge-probe'
 run('edge-compile',flags+[E/'edge-probe.c',R/'firmware/src/ble/ble_midi_central.c','-o',binary],R,0)
 run('edge-run',[binary],R,0)
inputs=['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','firmware/src/ble.c','firmware/hal/x0x_ble_abi.h','firmware/src/app/smc_mixer.c','tests/host/ble_midi_abort_test.c','tests/host/ble_midi_signal_test.c','evidence/session-stage3/investigation-abort-probe.c','evidence/session-stage3/test-busy-disconnect.c']
(E/'source-identity.json').write_text(json.dumps(dict(commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip(),tree=subprocess.check_output(['git','rev-parse','HEAD^{tree}'],cwd=R,text=True).strip(),host=platform.platform(),cc=subprocess.check_output(['cc','--version'],text=True),inputs={x:hashlib.sha256((R/x).read_bytes()).hexdigest() for x in inputs}),indent=2)+'\n')
print('All independent investigation probes have the expected results.')
