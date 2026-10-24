# Bounded independent checks of already-reported receipt binding and browser guards.
import copy,json,tempfile,subprocess,shutil
from pathlib import Path
R=Path('/workspace/pr8-independent-qa');E=Path(__file__).resolve().parent;results=[]
def record(name,ok,details):
 results.append({'case':name,'pass':bool(ok),'details':details});print(json.dumps(results[-1]),flush=True)
required=['scripts/verify-build-status.mjs','build-status.json','docs/build-status.json','preview-catalog.json','docs/versions.json','docs/builds/index.html','docs/index.html','docs/scheduler-handoffs/catalog-sync-20261024.json','docs/scheduler-handoffs/catalog-sync-20261024-r25.json','tests/fixtures/history-appendonly-baseline-20261024.json']
official='https://github.com/gobbajogger/X0X37-for-Loopy-Pro';sha='1'*40;wrong='f'*40
row={'id':'synthetic-final-matrix-source-receipt','title':'Synthetic future source receipt','date':'2026-10-25','status':'source-only','models':['pro'],'source_repo':official,'source_commit':sha,'source_tree':'2'*40,'device_verified':False,'installable':False,'evidence_commit':sha,'description':'Synthetic source evidence, not firmware qualification'}
for label,url,expected in [('matched-ref',official+'/blob/'+sha+'/docs/receipt.json',True),('claimed-sha-only-in-path',official+'/blob/'+wrong+'/docs/'+sha+'/receipt.json',False),('wrong-tree-ref',official+'/tree/'+wrong+'/docs/receipt.json',False),('mutable-ref-sha-in-path',official+'/blob/main/docs/'+sha+'/receipt.json',False)]:
 with tempfile.TemporaryDirectory(prefix='pr8-r4-ref-')as td:
  td=Path(td)
  for f in required:(td/f).parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(R/f,td/f)
  data=json.loads((td/'build-status.json').read_text());added=copy.deepcopy(row);added['evidence_url']=url;data['history'].append(added);data['counts']['source_or_historical_checkpoints']=len(data['history'])
  for f in ['build-status.json','docs/build-status.json']:(td/f).write_text(json.dumps(data,indent=2)+'\n')
  (td/'package.json').write_text('{"type":"module"}')
  p=subprocess.run(['node',str(td/'scripts/verify-build-status.mjs')],cwd=td,capture_output=True,text=True)
  record('F7 existing R28 URL-ref '+label,(p.returncode==0)==expected,{'exit':p.returncode,'expected_accept':expected,'url':url,'stdout':p.stdout.strip(),'stderr':p.stderr[-500:]})
# Baseline synthetic qualification is declared by the existing native fixture, not a claim of real QA.
prefix=(R/'tests/qualified-catalog.test.mjs').read_text().split('let negatives = 0;')[0].replace("'../scripts/render-qualified-nightly.mjs'",json.dumps((R/'scripts/render-qualified-nightly.mjs').as_uri()))
code=prefix+'''\nconst cases=[];const result=(name,pass)=>cases.push({name,pass});
validateCatalog(catalog,qualification);await verifyDownloads(catalog,async()=>new Response(raw));result('legitimate matching metadata and bytes',true);
for(const size of [pkg.size+1,Number.MAX_SAFE_INTEGER]){const c=structuredClone(catalog);c.versions[0].models.pro.size=size;let rejected=false;try{validateCatalog(c,qualification)}catch{rejected=true}result('catalog size '+size+' cannot bypass fixed receipt',rejected)}
const c=structuredClone(catalog),q=structuredClone(qualification);c.versions[0].models.pro.size++;q.packages[digest].size++;validateCatalog(c,q);let rejected=false;try{await verifyDownloads(c,async()=>new Response(raw))}catch{rejected=true}result('matching altered metadata still requires actual exact bytes',rejected);
console.log(JSON.stringify(cases));process.exitCode=cases.every(x=>x.pass)?0:1;
'''
script=E/'size_binding_check.mjs';script.write_text(code);p=subprocess.run(['node',str(script)],cwd=R,capture_output=True,text=True)
record('PR102 R25 equivalent PR8 size/receipt/actual-byte binding',p.returncode==0,{'exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr})
# Actual Updater.install/write remain intact; mock only discovery, I/O and timers.
script=E/'additional_browser_write_guards.mjs';script.write_text('''import{readFileSync}from'node:fs';import{Script}from'node:vm';import assert from'node:assert/strict';
const source=readFileSync('/workspace/pr8-independent-qa/web/fm1ota.js','utf8').replace(/^export\\s+/gm,'');const ctx={setTimeout:f=>{f();return 0},clearTimeout(){},TextEncoder};new Script(source+'\\nthis.api={Updater};').runInNewContext(ctx);const{Updater}=ctx.api;let n=0;
for(const [app,product]of [['FM-1','SMK-37 Elite_900'],['SMK-37 Pro','FM-1_900']]){let sends=0,served=0,closed=0;const dev={name:app,id:{model:app,text:app+'_900'},link:{send(){sends++;return true},close(){closed++}}};const up=new Updater({inputs:new Map([['i',{name:app,state:'connected'}]]),outputs:new Map()});up.find=async()=>dev;up.serve=async()=>{served++;return{finished:true}};await assert.rejects(up.install(new Uint8Array(1),product),e=>e.code==='model');assert.equal(sends,0);assert.equal(served,0);assert.equal(closed,1);n++}
{let sends=0,served=0;const dev={name:'SMK absent',id:{model:'ota-SMK-37 Pro',text:'ota-SMK-37 Pro_900'},link:{send(){sends++;return true},close(){}}};const up=new Updater({inputs:new Map(),outputs:new Map()});up.product='SMK-37 Elite_900';up.serve=async()=>{served++;return{finished:true}};await assert.rejects(up.write(dev,new Uint8Array(1),()=>{}),e=>e.code==='ambiguous');assert.equal(sends,0);assert.equal(served,0);n++}
console.log(JSON.stringify({checks:n,failed:0,scope:'actual browser Updater bodies, no Python or device'}));
''')
p=subprocess.run(['node',str(script)],cwd=R,capture_output=True,text=True)
record('actual browser wrong-app family and disconnected loader before UPGRADE',p.returncode==0,{'exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr})
(E/'final-matrix-extension-results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps({'checks':len(results),'failed':sum(not x['pass']for x in results)}),flush=True)
raise SystemExit(1 if any(not x['pass']for x in results)else 0)
