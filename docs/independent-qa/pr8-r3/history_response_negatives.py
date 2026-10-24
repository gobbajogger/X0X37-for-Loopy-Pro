# Independent replay of the five named merger negatives against immutable PR8.
import asyncio,copy,json,threading,hashlib,re,subprocess
from pathlib import Path
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from functools import partial
from playwright.async_api import async_playwright
R=Path('/workspace/pr8-independent-qa');E=Path(__file__).resolve().parent
class Handler(SimpleHTTPRequestHandler):
 def log_message(self,*a):pass
S=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(R/'docs')));threading.Thread(target=S.serve_forever,daemon=True).start()
# Preserve exact PR8 response fields, but use one build +117 rows for the merger fixture's cardinality.
base=json.loads((R/'docs/build-status.json').read_text());base['builds']=base['builds'][:1]
results=[]
async def run_case(browser,label,payload,width,negative=False):
 p=await browser.new_page(viewport={'width':width,'height':900});errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
 await p.route('**/build-status.json',lambda route:route.fulfill(status=200,content_type='application/json',body=json.dumps(payload)))
 await p.goto(f'http://127.0.0.1:{S.server_port}/builds/',wait_until='networkidle')
 state=await p.evaluate('''()=>({status:document.querySelector('#status').textContent,error:document.querySelector('#status').classList.contains('error'),builds:document.querySelectorAll('#builds article').length,history:document.querySelectorAll('#history article').length,tags:[...document.querySelectorAll('.tag')].map(x=>x.textContent),firmwareLinks:[...document.querySelectorAll('a[href]')].filter(x=>/\.(fwsc|img)([?#]|$)/i.test(x.href)).map(x=>x.href),scrollWidth:document.documentElement.scrollWidth,width:innerWidth})''')
 ok=state['error'] and state['builds']==0 and state['history']==0 if negative else not state['error'] and state['builds']==1 and state['history']==117 and state['scrollWidth']<=width and not state['firmwareLinks']
 result={'label':label,'expected':'error and zero partial cards'if negative else'complete readonly positive','pass':ok,'state':state,'page_errors':errors};results.append(result);print(json.dumps(result),flush=True)
 await p.screenshot(path=str(E/(label+'.png')),full_page=True);await p.close()
async def main():
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox']);print('Chromium '+b.version,flush=True)
  await run_case(b,'positive-375',base,375);await run_case(b,'positive-320',base,320)
  for label,mutate in [('null-history-last',lambda c:c['history'].__setitem__(-1,None)),('null-history-first',lambda c:c['history'].__setitem__(0,None)),('null-pro-model',lambda c:c['builds'][0]['models'].__setitem__('pro',None)),('schema-999',lambda c:c.__setitem__('schema',999)),('forged-device-verified',lambda c:c['history'][0].__setitem__('status','DEVICE VERIFIED'))]:
   c=copy.deepcopy(base);mutate(c);await run_case(b,label,c,375,True)
  # Additional applicability check with actual complete two-build response, no reduced fixture.
  actual=json.loads((R/'docs/build-status.json').read_text());actual['schema']=999
  await run_case(b,'actual-two-builds-schema-999',actual,375,True)
  await b.close()
 S.shutdown();(E/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps({'checks':len(results),'passed':sum(x['pass']for x in results),'failed':sum(not x['pass']for x in results)}),flush=True)
 raise SystemExit(1 if any(not x['pass']for x in results)else 0)
asyncio.run(main())
