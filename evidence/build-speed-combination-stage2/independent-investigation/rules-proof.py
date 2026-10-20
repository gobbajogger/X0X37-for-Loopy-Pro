#!/usr/bin/env python3
"""Check candidate rule-commit guard in a private local clone; no builds run."""
import json,os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];E=Path(__file__).resolve().parent
candidate='f53309ce43c7624a83291b1773ae1f86ec9db484'
with tempfile.TemporaryDirectory(prefix='x0x-combined-rule-guard-') as temp:
 repo=Path(temp)/'repo'
 subprocess.run(['git','clone','--shared','--no-checkout',str(ROOT),str(repo)],check=True,capture_output=True)
 subprocess.run(['git','-C',str(repo),'checkout','--detach',candidate],check=True,capture_output=True)
 env={k:v for k,v in os.environ.items() if not k.startswith(('X0X_','GIT_'))}
 env.update(X0X_REPO=str(repo),X0X_QC_BASE='1165396199210889fa460d26cbdd0b66183267e1',X0X_QUICK_ONLY='rules')
 argv=['bash',str(ROOT/'tools/quickcheck.sh'),str(repo)]
 p=subprocess.run(argv,env=env,capture_output=True,text=True,timeout=30)
 (E/'rules-proof.log').write_text(p.stdout+p.stderr)
 result={'candidate':candidate,'command':['X0X_QUICK_ONLY=rules','X0X_QC_BASE=1165396199210889fa460d26cbdd0b66183267e1','bash',str(ROOT/'tools/quickcheck.sh'),'<private clone>'],'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr,'build_execution':False}
 (E/'rules-proof.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result,indent=2))
