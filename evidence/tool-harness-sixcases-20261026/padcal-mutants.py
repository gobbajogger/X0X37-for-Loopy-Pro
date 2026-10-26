from pathlib import Path
import subprocess,json
root=Path(__file__).resolve().parent.parent;repo=root/'tool-harness-fixes';out=root/'tool-harness-evidence';rows=[]
mutants=[('missing-padcal','build.py',', "X0X_DEV_PADCAL"',''),('release-flag-dropped','build.py','if v in ("0", "1"):','if v == "1":'),('always-zero','build.py','flags.append(f"-D{flag}={v}")','flags.append(f"-D{flag}=0")'),('dev-flag-wrong','dev_validate.py',"X0X_DEV_PADCAL='1'","X0X_DEV_PADCAL='0'")]
code='''from pathlib import Path
from unittest.mock import patch
import runpy,sys
root=Path(sys.argv[1]);name,old,new=sys.argv[2:];original=Path.read_text
def altered(p,*a,**k):
 text=original(p,*a,**k)
 if p==root/'tools'/name:
  assert old in text
  return text.replace(old,new,1)
 return text
with patch.object(Path,'read_text',altered):
 runpy.run_path(str(root/'tests/padcal_flag_test.py'),run_name='__main__')
'''
for label,name,old,new in mutants:
 p=subprocess.run(['python3','-c',code,str(repo),name,old,new],capture_output=True,text=True);(out/(label+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL ' in p.stdout and 'Traceback' not in p.stderr,label;rows.append({'mutant':label,'exit':p.returncode,'expectedFailure':True})
(out/'padcal-mutants.json').write_text(json.dumps(rows,indent=2)+'\n');print('PASS four negative production-code mutants rejected by actual revised test; original source files unchanged')
