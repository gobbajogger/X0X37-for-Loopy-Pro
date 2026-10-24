# Independent history validation and atomic DOM boundary extension; actual served HTML.
import asyncio,json,copy,threading
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from playwright.async_api import async_playwright
R=Path('/workspace/pr8-independent-qa');E=Path(__file__).resolve().parent;BASE=json.loads((R/'docs/build-status.json').read_text());results=[]
class Handler(SimpleHTTPRequestHandler):
 def log_message(self,*a):pass
S=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(R/'docs')));threading.Thread(target=S.serve_forever,daemon=True).start()
async def case(b,label,data,negative=True,inject=None):
 p=await b.new_page(viewport={'width':375,'height':900});errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
 if inject:await p.add_init_script(inject)
 await p.route('**/build-status.json',lambda r:r.fulfill(status=200,content_type='application/json',body=json.dumps(data)))
 await p.goto(f'http://127.0.0.1:{S.server_port}/builds/',wait_until='networkidle')
 state=await p.evaluate('''()=>({error:document.querySelector('#status').classList.contains('error'),builds:document.querySelectorAll('#builds article').length,history:document.querySelectorAll('#history article').length,executable:document.querySelectorAll('#history script,#history img').length,ran:window.attack===true,width:innerWidth,scrollWidth:document.documentElement.scrollWidth})''')
 ok=state['error']and state['builds']==0 and state['history']==0 if negative else not state['error']and state['builds']==len(data['builds'])and state['history']==len(data['history'])
 ok=ok and not errors and not state['executable']and not state['ran']
 results.append({'case':label,'pass':bool(ok),'state':state,'page_errors':errors});print(json.dumps(results[-1]),flush=True);await p.close()
async def main():
 mutations=[
 ('cross-list-duplicate-id',lambda c:c['history'][0].__setitem__('id',c['builds'][0]['id'])),
 ('duplicate-last-history-id',lambda c:c['history'][-1].__setitem__('id',c['history'][0]['id'])),
 ('top-installable-string-false',lambda c:c.__setitem__('installable','false')),
 ('history-installable-string-false',lambda c:c['history'][0].__setitem__('installable','false')),
 ('history-device-verified-string-false',lambda c:c['history'][0].__setitem__('device_verified','false')),
 ('build-hardware-string-false',lambda c:c['builds'][0].__setitem__('hardware_tested','false')),
 ('model-device-string-false',lambda c:c['builds'][0]['models']['pro'].__setitem__('device_verified','false')),
 ('model-physical-claim',lambda c:c['builds'][0]['models']['pro'].__setitem__('physical_evidence',{'owner':True})),
 ('model-size-unsafe-integer',lambda c:c['builds'][0]['models']['pro'].__setitem__('size',9007199254740992)),
 ('model-size-negative',lambda c:c['builds'][0]['models']['pro'].__setitem__('size',-1)),
 ('model-size-fraction',lambda c:c['builds'][0]['models']['pro'].__setitem__('size',1.5)),
 ('model-product-other-family',lambda c:c['builds'][0]['models']['pro'].__setitem__('product','FM-1_900')),
 ('history-title-array',lambda c:c['history'][0].__setitem__('title',['DEVICE VERIFIED'])),
 ('history-description-object',lambda c:c['history'][-1].__setitem__('description',{'status':'DEVICE VERIFIED'})),
 ('history-models-object',lambda c:c['history'][0].__setitem__('models',{'pro':True})),
 ('history-models-unknown',lambda c:c['history'][0].__setitem__('models',['FM-1'])),
 ('history-source-object',lambda c:c['history'][0].__setitem__('source_commit',{'ref':'main'})),
 ('history-source-short-sha',lambda c:c['history'][0].__setitem__('source_commit','a'*39)),
 ('history-id-whitespace',lambda c:c['history'][0].__setitem__('id','  ')),
 ('build-status-DEVICE-VERIFIED',lambda c:c['builds'][0].__setitem__('status','DEVICE VERIFIED')),
 ('model-map-array',lambda c:c['builds'][0].__setitem__('models',[])),
 ('unknown-model-key',lambda c:c['builds'][0]['models'].__setitem__('constructor',copy.deepcopy(c['builds'][0]['models']['pro']))),
 ('model-sha-object',lambda c:c['builds'][0]['models']['pro'].__setitem__('sha256',{'sha':'a'*64})),
 ]
 async with async_playwright()as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox']);print('Chromium '+b.version,flush=True)
  for label,mut in mutations:c=copy.deepcopy(BASE);mut(c);await case(b,label,c)
  await case(b,'root-null',None);await case(b,'root-array',[])
  # Failure after the first section is committed: catch must clear BOTH lists.
  await case(b,'second-list-commit-throws',BASE,True,"const old=Element.prototype.replaceChildren;let once=false;Element.prototype.replaceChildren=function(...nodes){if(this.id==='history'&&!once){once=true;throw Error('synthetic second-list commit failure')}return old.apply(this,nodes)};")
  await case(b,'last-card-construction-throws',BASE,True,"const old=Document.prototype.createElement;let n=0;Document.prototype.createElement=function(tag,...a){if(tag==='article'&&++n===119)throw Error('synthetic last-card failure');return old.call(this,tag,...a)};")
  for name,mut in [('actual-full-positive',lambda c:None),('empty-lists-positive',lambda c:(c.__setitem__('builds',[]),c.__setitem__('history',[]))),('text-markup-inert-positive',lambda c:c['history'][0].__setitem__('title','<img src=x onerror="window.attack=true"><script>window.attack=true</script>'))]:
   c=copy.deepcopy(BASE);mut(c);await case(b,name,c,False)
  c=copy.deepcopy(BASE);new={'id':'future-synthetic-positive','source_commit':'1'*40,'title':'Synthetic future source record','status':'source-only','models':['elite','pro'],'installable':False,'device_verified':False,'description':'Presentation-only future append'};c['history'].append(new);await case(b,'future-typed-append-positive',c,False)
  await b.close()
 S.shutdown();(E/'history-extension-results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps({'checks':len(results),'failed':sum(not x['pass']for x in results)}),flush=True)
 raise SystemExit(1 if any(not x['pass']for x in results)else 0)
asyncio.run(main())
