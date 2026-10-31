"""Check the bounded abort repair against public tests and prior independent probes.
All HCI packets and mailbox conditions are simulated. No device access occurs.
"""
from pathlib import Path
import hashlib, json, shutil, subprocess, tempfile
R=Path(__file__).resolve().parents[2]
E=Path(__file__).resolve().parent
records=[]
def run(name,argv,cwd=R,expected=0):
    p=subprocess.run([str(a) for a in argv],cwd=cwd,capture_output=True,text=True)
    (E/(name+'.log')).write_text(p.stdout+p.stderr)
    records.append(dict(name=name,argv=[str(a) for a in argv],cwd=str(cwd),returncode=p.returncode,expected=expected,log=name+'.log'))
    (E/'host-checks.json').write_text(json.dumps(records,indent=2)+'\n')
    assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
    print(name+': PASS',flush=True)
run('receive',['sh','tests/host/ble_midi_receive_test.sh'])
run('diag',['python3','-B','tests/ble_central_diag_test.py'])
flags=['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='central-abort-') as scratch:
    for revision in ['base','candidate']:
        w=Path(scratch)/revision
        for sub in ['firmware/src/ble','tests/host','evidence/session-stage3']:(w/sub).mkdir(parents=True,exist_ok=True)
        for f in ['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h']:
            data=subprocess.check_output(['git','show','28d0fc230f60bc823c1e5e73d3aed61e68a8e32f:'+f],cwd=R) if revision=='base' else (R/f).read_bytes()
            (w/f).write_bytes(data)
        for f in ['tests/host/ble_midi_signal_test.c','tests/host/ble_midi_abort_test.c','evidence/session-stage3/test-busy-disconnect.c','evidence/session-stage3/investigation-abort-probe.c']:
            shutil.copyfile(R/f,w/f)
        for name,f,modes in [('public','tests/host/ble_midi_abort_test.c',['overflow','timeout']),('investigation','evidence/session-stage3/investigation-abort-probe.c',['overflow','timeout']),('independent-test','evidence/session-stage3/test-busy-disconnect.c',[''])]:
            binary=w/name
            run(revision+'-'+name+'-compile',flags+[f,'firmware/src/ble/ble_midi_central.c','-o',binary],w)
            for mode in modes:
                expected=0 if revision=='candidate' else 1 if name=='independent-test' else -6
                run(revision+'-'+name+'-'+(mode or 'both'),[binary]+([mode] if mode else []),w,expected)
files=['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','firmware/src/ble.c','tests/host/ble_midi_abort_test.c','tests/host/ble_midi_receive_test.sh','tests/host/ble_midi_signal_test.c','tools/build.py','tools/locked_settings.json']
(E/'source-sha256.json').write_text(json.dumps({f:hashlib.sha256((R/f).read_bytes()).hexdigest() for f in files},indent=2)+'\n')
