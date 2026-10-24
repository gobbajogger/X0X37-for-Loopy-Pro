# Compare unchanged exact R4 and existing R28 validator; no producer edits.
import tempfile,subprocess,json,shutil
from pathlib import Path
R=Path('/workspace/pr8-independent-qa');E=Path(__file__).resolve().parent
files=['scripts/verify-build-status.mjs','build-status.json','docs/build-status.json','preview-catalog.json','docs/versions.json','docs/builds/index.html','docs/index.html','docs/scheduler-handoffs/catalog-sync-20261024.json','docs/scheduler-handoffs/catalog-sync-20261024-r25.json','tests/fixtures/history-appendonly-baseline-20261024.json']
sha='1'*40;wrong='f'*40;repo='https://github.com/gobbajogger/X0X37-for-Loopy-Pro';out=[]
for ref in ['R4','R28']:
 for case in ['matching','claimed-SHA-in-path']:
  with tempfile.TemporaryDirectory(prefix='pr8-r28-compare-')as t:
   t=Path(t)
   for f in files:(t/f).parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(R/f,t/f)
   if ref=='R28':(t/files[0]).write_bytes(subprocess.check_output(['git','show','9d03fe036220c02b224a91a90c0bb09b66fd29fe:'+files[0]],cwd=R))
   data=json.loads((t/'build-status.json').read_text());url=repo+'/blob/'+(sha if case=='matching'else wrong)+'/docs/'+sha+'/receipt.json';data['history'].append({'id':'synthetic-url-ref-comparison','title':'Synthetic source evidence','date':'2026-10-25','status':'source-only','models':['pro'],'source_repo':repo,'source_commit':sha,'source_tree':'2'*40,'installable':False,'device_verified':False,'evidence_commit':sha,'evidence_url':url});data['counts']['source_or_historical_checkpoints']=len(data['history'])
   for f in ['build-status.json','docs/build-status.json']:(t/f).write_text(json.dumps(data,indent=2)+'\n')
   (t/'package.json').write_text('{"type":"module"}')
   p=subprocess.run(['node',str(t/files[0])],cwd=t,capture_output=True,text=True)
   result={'source':ref,'case':case,'exit':p.returncode,'accepted':p.returncode==0,'url':url,'stdout':p.stdout.strip(),'stderr':p.stderr[-500:]};out.append(result);print(json.dumps(result),flush=True)
(E/'r28-reference-results.json').write_text(json.dumps(out,indent=2)+'\n')
assert [(x['source'],x['case'],x['accepted'])for x in out]==[('R4','matching',True),('R4','claimed-SHA-in-path',True),('R28','matching',True),('R28','claimed-SHA-in-path',False)]
