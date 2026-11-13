"""Check refused Cancel and Scan Disable through each revision's production service.
The HCI peer, mailbox, clock and hardware dependencies are simulated.
No device access occurs. Source files are read from exact local commits.
"""
from pathlib import Path
import hashlib,json,subprocess,tempfile
R=Path(__file__).resolve().parents[3]
E=Path(__file__).resolve().parent
records=[]; identities={}
def run(name,argv,cwd,expected):
 p=subprocess.run([str(x) for x in argv],cwd=cwd,capture_output=True,text=True)
 (E/(name+'.log')).write_text(p.stdout+p.stderr)
 records.append(dict(name=name,argv=[str(x) for x in argv],cwd=str(cwd),returncode=p.returncode,expected=expected,result='PASS' if p.returncode==expected else 'UNEXPECTED',log=name+'.log'))
 (E/'service-stop-record.json').write_text(json.dumps(records,indent=2)+'\n')
 assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
flags=['cc','-std=c99','-g','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
with tempfile.TemporaryDirectory(prefix='central-service-stop-') as scratch:
 for label,rev in [('candidate','f080d1a5f31073dde600238f780acf37c12b2738'),('before-deadline','05eff604cb3f136db71727a2aeeea785cab73b99'),('before-session','1313e0fe384ef5df1f2f5f5ef699902c4543bc0d')]:
  w=Path(scratch)/label; w.mkdir(); hashes={}
  for name in ['firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','firmware/src/ble.c']:
   data=subprocess.check_output(['git','show',rev+':'+name],cwd=R)
   hashes[name]=hashlib.sha256(data).hexdigest()
   (w/Path(name).name).write_bytes(data)
  source=(w/'ble.c').read_text(); start=source.index('static void ble_central_service(void)\n{'); end=source.index('\n}\n',start)+3
  service=source[start:end]; (w/'ble_central_service.inc').write_text(service)
  identities[label]=dict(revision=rev,source_sha256=hashes,service_sha256=hashlib.sha256(service.encode()).hexdigest())
  binary=w/'probe'
  run(label+'-service-stop-compile',flags+['-I'+str(w),E/'service-stop-probe.c',w/'ble_midi_central.c','-o',binary],R,0)
  for path,disposition in [('cancel','refused'),('cancel','accepted'),('scan','refused'),('scan','accepted'),('scan','failed-status')]:
   run(label+'-'+path+'-'+disposition,[binary,path,disposition],R,-6 if disposition in ['refused','failed-status'] else 0)
(E/'service-stop-inputs.json').write_text(json.dumps(identities,indent=2)+'\n')
print('Refused-command and failed-status failures reproduce on all three exact revisions. Accepted controls pass.')
