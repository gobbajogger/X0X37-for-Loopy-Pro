"""Record the actual Linux worker before the complete DEV validation."""
from pathlib import Path
import hashlib,json,os,platform,subprocess
r=Path.cwd();o=r/'evidence/complete-stage4'
e={'platform':platform.platform(),'machine':platform.machine(),'user':os.getuid(),'group':os.getgid(),'python':platform.python_version(),'cc':subprocess.check_output(['cc','--version'],text=True),'target_cc_sha256':hashlib.sha256(Path('/Users/mac/.jieli/toolchain/pi32v2/bin/clang').read_bytes()).hexdigest(),'sdk_revision':subprocess.check_output(['git','-C','/sdk','rev-parse','HEAD'],text=True).strip(),'sdk_tracked_status':subprocess.check_output(['git','-C','/sdk','status','--porcelain','--untracked-files=no'],text=True).strip()}
assert e['machine']=='x86_64' and e['user']==501 and e['group']==20
assert e['target_cc_sha256']=='c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f'
assert e['sdk_revision']=='d179b4484759423312073f5fbb232501aa491047' and not e['sdk_tracked_status']
(o/'worker-environment.json').write_text(json.dumps(e,indent=2)+'\n')
print(json.dumps(e,indent=2))
