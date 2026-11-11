"""Record public stop regressions and the independent production-service probes.
All events, mailbox pressure and clock values are simulated. No device access occurs.
"""
from pathlib import Path
import hashlib,json,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
E=Path(__file__).resolve().parent
records=[]
def run(name,argv,expected=0):
    p=subprocess.run([str(a) for a in argv],cwd=R,capture_output=True,text=True)
    output=p.stdout+p.stderr
    (E/(name+'.log')).write_text(output)
    ok=p.returncode==expected and not any(s in output for s in ['ERROR: AddressSanitizer','runtime error:'])
    if expected: ok=ok and 'Assertion failed' in output
    records.append(dict(name=name,argv=[str(a) for a in argv],cwd=str(R),returncode=p.returncode,expected=expected,result='PASS' if ok else 'FAIL',log=name+'.log'))
    (E/'stop-checks.json').write_text(json.dumps(records,indent=2)+'\n')
    assert ok,(name,output)
with tempfile.TemporaryDirectory(prefix='stop-controls-') as temp:
    w=Path(temp); (w/'firmware/src/ble').mkdir(parents=True)
    for f in ['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','firmware/src/ble.c']:
        (w/f).write_bytes(subprocess.check_output(['git','show','f080d1a5f31073dde600238f780acf37c12b2738:'+f],cwd=R))
    for case in ['cancel','scan','failed-status','edges']:
        run('broken-stop-'+case,['python3','-B','tests/host/ble_midi_stop_test.py','--source-root',w,case],1)
        run('repaired-stop-'+case,['python3','-B','tests/host/ble_midi_stop_test.py',case])
    source=(R/'firmware/src/ble.c').read_text(); start=source.index('static void ble_central_service(void)\n{'); end=source.index('\n}\n',start)+3
    (w/'ble_central_service.inc').write_text(source[start:end])
    probe=R/'evidence/deadline-stage3/investigation/service-stop-probe.c'
    flags=['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    binary=w/'service-probe'
    run('saved-service-compile',flags+['-I'+str(w),'-I'+str(R/'firmware/src/ble'),probe,R/'firmware/src/ble/ble_midi_central.c','-o',binary])
    for path,mode in [('cancel','refused'),('cancel','accepted'),('scan','refused'),('scan','accepted'),('scan','failed-status')]:
        run('saved-service-'+path+'-'+mode,[binary,path,mode])
(E/'stop-inputs.json').write_text(json.dumps({f:hashlib.sha256((R/f).read_bytes()).hexdigest() for f in ['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','firmware/src/ble.c','tests/host/ble_midi_stop_test.c','tests/host/ble_midi_stop_test.py','tests/host/ble_midi_receive_test.sh']},indent=2)+'\n')
print('Stop fail-before/pass-after and saved production service checks: PASS')
