# Independent actual renderer/validator/browser boundary checks; synthetic data only.
import asyncio,json,copy,subprocess,tempfile,shutil,os
from pathlib import Path
from playwright.async_api import async_playwright
import independent_browser as q
R=Path('/workspace/pr8-independent-qa');E=Path(__file__).resolve().parent
results=[]
def record(name,ok,details):
 result={'case':name,'pass':bool(ok),'details':details};results.append(result);print(json.dumps(result),flush=True)
def manifest_checks():
 fixture=E/'make_qualification_fixture.mjs'
 # Extract existing explicit synthetic fixture definitions, not its test results or fake approvals.
 prefix=(R/'tests/qualified-catalog.test.mjs').read_text().split('let negatives = 0;')[0].replace("'../scripts/render-qualified-nightly.mjs'",json.dumps((R/'scripts/render-qualified-nightly.mjs').as_uri()))
 fixture.write_text(prefix+'''\nvalidateCatalog(catalog,qualification);console.log('BASELINE_VALID');
const markers=['SYNTHETIC_PRIVATE_BINARY_BYTES_DO_NOT_PUBLISH','SYNTHETIC_PRIVATE_TOKEN','SYNTHETIC_PRIVATE_LIBRARY_LOCATOR','SYNTHETIC_PRIVATE_CTL_IMG'];
catalog.internal_controller_image_base64=markers[0];catalog.versions[0].models.pro.auth_token=markers[1];
qualification.internal_library_locator=markers[2];qualification.packages[digest].controller.vendor_payload=markers[3];
let admitted=true;try{validateCatalog(catalog,qualification)}catch{admitted=false}
const fs=await import('node:fs');fs.writeFileSync(process.argv[2],JSON.stringify({catalog,qualification,raw:[...raw],markers,admitted}));
''')
 generated=E/'qualification_fixture.json';p=subprocess.run(['node',str(fixture),str(generated)],cwd=R,capture_output=True,text=True);assert p.returncode==0,p.stderr
 data=json.loads(generated.read_text());record('PR7 manifest unknown fields reject',not data['admitted'],{'baseline_valid':True,'mutated_admitted':data['admitted']})
 with tempfile.TemporaryDirectory(prefix='pr8-projection-')as td:
  td=Path(td);(td/'catalog.json').write_text(json.dumps(data['catalog']));(td/'qualification.json').write_text(json.dumps(data['qualification']));
  shim=td/'fetch.mjs';shim.write_text("const data="+json.dumps(data)+";globalThis.fetch=async()=>new Response(new Uint8Array(data.raw));")
  output=td/'prepared';cmd=['node','--import',str(shim),str(R/'scripts/render-qualified-nightly.mjs'),'--catalog',str(td/'catalog.json'),'--qualification',str(td/'qualification.json'),'--out',str(output)]
  p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);leaks={}
  for f in ['install/index.html','versions.json','qualification.json']:
   text=(output/f).read_text()if(output/f).exists()else'';leaks[f]=[m for m in data['markers']if m in text]
  record('PR7 actual local candidate output redacts private sentinels',not any(leaks.values()),{'cli_exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr.strip(),'leaked_synthetic_markers':leaks,'network':'mocked synthetic Response, actual package parser/verifier and CLI outputs'})
def provenance_checks():
 required=['scripts/verify-build-status.mjs','build-status.json','docs/build-status.json','preview-catalog.json','docs/versions.json','docs/builds/index.html','docs/index.html','docs/scheduler-handoffs/catalog-sync-20261024.json','docs/scheduler-handoffs/catalog-sync-20261024-r25.json','tests/fixtures/history-appendonly-baseline-20261024.json']
 official='https://github.com/gobbajogger/X0X37-for-Loopy-Pro';sha='1'*40
 row={'id':'synthetic-future-evidence-commit-qa','title':'Synthetic future source receipt','date':'2026-10-25','status':'source-only','models':['pro'],'source_repo':official,'source_commit':sha,'source_tree':'2'*40,'device_verified':False,'installable':False,'evidence_commit':sha,'evidence_url':official+'/commit/'+sha,'description':'Synthetic future noninstallable source evidence'}
 for label,value,expect in [('pinned-match',sha,True),('mutable-main','main',False),('mismatched-sha','f'*40,False),('object-value',{'ref':'main'},False)]:
  with tempfile.TemporaryDirectory(prefix='pr8-provenance-')as td:
   td=Path(td)
   for f in required:(td/f).parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(R/f,td/f)
   data=json.loads((td/'build-status.json').read_text());added=copy.deepcopy(row);added['evidence_commit']=value;data['history'].append(added);data['counts']['source_or_historical_checkpoints']=len(data['history'])
   for f in ['build-status.json','docs/build-status.json']:(td/f).write_text(json.dumps(data,indent=2)+'\n')
   (td/'package.json').write_text('{"type":"module"}')
   p=subprocess.run(['node',str(td/'scripts/verify-build-status.mjs')],cwd=td,capture_output=True,text=True)
   record('PR3 R27 evidence_commit '+label,(p.returncode==0)==expect,{'exit':p.returncode,'expected_accept':expect,'stdout':p.stdout.strip(),'stderr':p.stderr[-700:]})
async def bounded_fetch_checks():
 async with async_playwright()as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  for mode in ['bodyless','oversized-stream']:
   p=await b.new_page(viewport={'width':390,'height':900});errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
   script='''window.boundary={buffers:0,pulls:0,cancelled:0};window.fetch=async()=>('''+('''{ok:true,redirected:false,body:null,arrayBuffer:async()=>{boundary.buffers++;return new Uint8Array(2097153).buffer}}'''if mode=='bodyless'else'''new Response(new ReadableStream({pull(c){boundary.pulls++;if(boundary.pulls<=4)c.enqueue(new Uint8Array(1024));else c.close()},cancel(){boundary.cancelled++}}))''')+');'
   await p.add_init_script(script);await p.goto(f'http://127.0.0.1:{q.S.server_port}/probe/',wait_until='networkidle');await p.wait_for_timeout(80)
   state=await p.evaluate('({boundary,state:iqa.state()})')
   ok=state['boundary']['buffers']==0 if mode=='bodyless'else state['boundary']['cancelled']>0 and state['boundary']['pulls']<5
   record('PR6 bounded response '+mode,ok,{**state,'page_errors':errors,'safe_fixture':'max2MiB+1 bytes; no actual unbounded allocation or device','admission_still_denied':state['state']['href']is None and state['state']['disabled']});await p.close()
  await b.close()
 q.S.shutdown()
manifest_checks();provenance_checks();asyncio.run(bounded_fetch_checks())
(E/'dependency-boundary-results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps({'checks':len(results),'failed':sum(not r['pass']for r in results)}),flush=True)
raise SystemExit(1 if any(not r['pass']for r in results)else 0)
