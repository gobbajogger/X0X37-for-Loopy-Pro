import asyncio, hashlib, json, pathlib, struct, threading, zlib, urllib.parse
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from playwright.async_api import async_playwright

BASE=pathlib.Path('/workspace/final-qualified-private')
SITE=BASE/'prepared-site'; OUT=BASE/'browser-evidence'
CAT=json.loads((SITE/'versions.json').read_text())
MODELS=CAT['versions'][0]['models']
RECEIPT=json.loads((BASE/'independent-live-https-receipt.json').read_text())
RAW={r['model']:pathlib.Path(r['local_path']).read_bytes() for r in RECEIPT['files'] if r['kind']=='package'}
for m,b in RAW.items():
 assert hashlib.sha256(b).hexdigest()==MODELS[m]['sha256'] and len(b)==MODELS[m]['size']
HTML_HASH=hashlib.sha256((SITE/'install/index.html').read_bytes()).hexdigest()
assert HTML_HASH=='152e56f49f383dd69fa079e27477febb629d41ddbdd79d8160b05a56594249ba'
class Handler(SimpleHTTPRequestHandler):
 def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(SITE),**kwargs)
 def log_message(self,*a):pass
 def do_GET(self):
  if urllib.parse.urlsplit(self.path).query=='qaProbe=1':
   raw=(SITE/'install/index.html').read_text().replace('</script>',"\nwindow.__qaEval = expression => eval(expression);\n</script>")
   body=raw.encode();self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
  else:super().do_GET()
server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
checks=[];traffic=[];pages=[]
def check(ok,name,details=None):
 row={'case':name,'pass':bool(ok)}
 if details is not None:row['details']=details
 checks.append(row);print(json.dumps(row,ensure_ascii=False),flush=True)
INIT="""window.__midiCalls=0;Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>{window.__midiCalls++;return{inputs:new Map(),outputs:new Map()};}});"""
STATE="""()=>({product:selectedProduct,image:!!image,model:chosenModel,href:$('download').getAttribute('href'),hidden:$('download').hidden,disabled:$('go').disabled,status:$('status').textContent,midiCalls:window.__midiCalls,writing,ctl:!!ctl,ack:$('devagree').checked})"""
class ProbePage:
 def __init__(self,page):self.page=page
 def __getattr__(self,name):return getattr(self.page,name)
 async def evaluate(self,expression):return await self.page.evaluate("expression=>{const result=window.__qaEval(expression);return typeof result==='function'?result():result;}",expression)
 async def wait_for_function(self,expression):return await self.page.wait_for_function("expression=>{const result=window.__qaEval(expression);return typeof result==='function'?result():result;}",arg=expression)
async def wait_ready(p):await p.wait_for_function("()=>{const a=document.getElementById('download');return !a.hidden && (a.getAttribute('href')||'').startsWith('blob:');}")
async def new_page(browser,width=390,lang='en',mode='valid',hold=False,probe=True):
 p=await browser.new_page(viewport={'width':width,'height':900},accept_downloads=True)
 pages.append(p);errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
 p.on('dialog',lambda dialog:asyncio.create_task(dialog.accept()))
 await p.add_init_script(INIT)
 pending=[]
 async def route_handler(route):
  req=route.request;model=next(k for k,v in MODELS.items() if req.url==v['pkg'])
  traffic.append({'model':model,'url':req.url,'transport':'Playwright-controlled response; not external browser HTTPS','mode':mode})
  if hold:
   event=asyncio.Event();pending.append((model,event));await event.wait()
  data=RAW[model];status=200
  if mode=='corrupt':data=bytes([data[0]^1])+data[1:]
  elif mode=='truncated':data=data[:-1]
  elif mode=='oversized':data=data+b'\0'
  elif mode=='swapped':data=RAW['elite' if model=='pro' else 'pro']
  elif mode=='http503':status=503
  elif mode=='interrupted':await route.abort('failed');return
  await route.fulfill(status=status,headers={'Content-Type':'application/octet-stream','Access-Control-Allow-Origin':'*'},body=data)
 await p.route('https://raw.githubusercontent.com/**',route_handler)
 await p.goto(f'http://127.0.0.1:{server.server_port}/install/'+('?qaProbe=1' if probe else ''),wait_until='domcontentloaded')
 if lang=='ja':await p.locator('#lang').click()
 return ProbePage(p) if probe else p,errors,pending
async def choose(p,model):
 if not await p.locator('input[name=model][value='+model+']').is_checked():await p.locator('input[name=model][value='+model+']').check()
 await wait_ready(p)
def valid_wrong_controller():
 b=bytearray(96)
 struct.pack_into('<IHH',b,0,0x43423058,2,68)
 for off,value in [(12,96),(20,2),(24,0x02087000+68),(28,0x02087000+72),(32,0x01c02000),(36,0x01c02100),(40,0x01c01800),(44,0x01c03000)]:struct.pack_into('<I',b,off,value)
 struct.pack_into('<I',b,16,zlib.crc32(b)&0xffffffff)
 return bytes(b)
WRONG_CTL=valid_wrong_controller()
async def main():
 async with async_playwright() as pw:
  browser=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  print(json.dumps({'chromium':browser.version,'prepared_html_sha256':HTML_HASH,'transport':'Unchanged local prepared page; exact public package bytes supplied by controlled request fulfilment; MIDI mocked; no TLS override'}),flush=True)
  for width in [320,390,1280]:
   for lang in ['en','ja']:
    for model in ['elite','pro']:
     label=f'{width}/{lang}/{model}'
     p,errors,_=await new_page(browser,width,lang,probe=False);await choose(p,model)
     s=await p.evaluate("()=>({href:document.getElementById('download').getAttribute('href'),disabled:document.getElementById('go').disabled,ack:document.getElementById('devagree').checked})")
     b=RAW[model]
     check((await p.locator('#pkgmeta').inner_text()).startswith(MODELS[model]['product']),label+' exact real package model displayed on unmodified page')
     check(s['disabled'] and not s['ack'] and await p.locator('#devagreewrap').is_visible(),label+' explicit DEV consent required')
     text=await p.locator('#buildsummary').inner_text();badges=await p.locator('#buildbadges').inner_text()
     check('DEVICE NOT TESTED' in text and ('HARDWARE UNTESTED' in badges if lang=='en' else '実機未テスト' in badges),label+' truthful hardware-untested labels')
     check(await p.evaluate('document.documentElement.scrollWidth<=innerWidth'),label+' responsive no horizontal overflow')
     check(s['href'].startswith('blob:'),label+' only verified Blob download exposed')
     async with p.expect_download() as info:await p.locator('#download').click()
     d=await info.value;downloaded=pathlib.Path(await d.path()).read_bytes()
     check(downloaded==b,label+' downloaded exact actual bare package',{'sha256':hashlib.sha256(downloaded).hexdigest(),'bytes':len(downloaded)})
     await p.locator('#devagree').check();await p.locator('#go').click()
     await p.wait_for_function("()=>document.getElementById('status').textContent.includes("+json.dumps('Select the exact ctl.img checked for this build.' if lang=='en' else 'このビルドで検証された ctl.img を選択してください。')+")")
     check(await p.evaluate('__midiCalls===0'),label+' missing exact controller refuses before MIDI')
     check(not errors,label+' no page exceptions',errors)
     await p.close()
  # Genuine file input -> real structural validator -> real exact-hash refusal.
  for model in ['elite','pro']:
   p,errors,_=await new_page(browser);await choose(p,model)
   functions=await p.evaluate('()=>[ctlProblem.toString(),insertCtl.toString(),productOf.toString(),Updater.prototype.write.toString()]')
   await p.locator('#ctlfile').set_input_files({'name':'synthetic-invalid.img','mimeType':'application/octet-stream','buffer':b'not a controller'})
   await p.wait_for_function("$('status').textContent.includes(t('ctlbad'))")
   check(await p.evaluate('ctl===null && __midiCalls===0'),model+' malformed local controller refused')
   await p.locator('#ctlfile').set_input_files({'name':'synthetic-valid-but-wrong-hash.img','mimeType':'application/octet-stream','buffer':WRONG_CTL})
   await p.wait_for_function('ctl!==null')
   check(await p.evaluate('ctlProblem(ctl)===null'),model+' wrong-hash fixture passes genuine structural validator')
   await p.locator('#devagree').check();await p.locator('#go').click()
   await p.wait_for_function("$('status').textContent.includes(t('ctlmatch'))")
   check(await p.evaluate('__midiCalls===0 && !writing'),model+' structurally valid wrong controller refuses before MIDI')
   await p.evaluate("()=>{$('go').dispatchEvent(new MouseEvent('click'));$('go').dispatchEvent(new MouseEvent('click'));}")
   await p.wait_for_timeout(60)
   check(await p.evaluate('__midiCalls===0 && !writing'),model+' repeated clicks cannot bypass controller hash')
   after=await p.evaluate('()=>[ctlProblem.toString(),insertCtl.toString(),productOf.toString(),Updater.prototype.write.toString()]')
   check(functions==after,model+' real parser/controller/Updater functions remain unchanged')
   # Actual Updater.write/bind bodies, mock MIDI links only; positives stop at first send.
   result=await p.evaluate("""async()=>{
    const rows=[];const row=(name,pass,details)=>rows.push({name,pass,details});
    const access=names=>({inputs:new Map(names.map((name,i)=>[String(i),{name,state:'connected'}])),outputs:new Map()});
    for(const name of ['SMK-37 Elite','SMK-37 Pro','ota-SMK-37 Pro']){
     let sends=0,closed=0;const up=new Updater(access([name]));up.product=selectedProduct;
     const dev={name,id:{model:name,text:name+'_900'},link:{send(){sends++;throw Error('QA_STOP_BEFORE_MIDI_WRITE');},close(){closed++;}}};
     let error;try{up.bind(dev);await up.write(dev,image,()=>{});}catch(e){error=e.message;}
     row('legitimate SMK alias '+name,sends===1 && error==='QA_STOP_BEFORE_MIDI_WRITE',{sends,stopped_before_io:true});
    }
    {let sends=0,closed=0;const name='FM-1',up=new Updater(access([name]));up.product=selectedProduct;
     const dev={name,id:{model:'ota-FM-1',text:'ota-FM-1_900'},link:{send(){sends++;throw Error('unexpected send');},close(){closed++;}}};
     let code;try{await up.write(dev,image,()=>{});}catch(e){code=e.code;}
     row('cross-family loader rejected before UPGRADE',code==='model'&&sends===0&&closed===1,{code,sends,closed});}
    {let closed=0;const up=new Updater(access(['SMK-37 Pro','SMK-37 Elite']));const dev={name:'SMK-37 Pro',id:{model:'SMK-37 Pro'},link:{close(){closed++;}}};let code;try{up.bind(dev);}catch(e){code=e.code;}row('concurrent MIDI devices cannot bind',code==='ambiguous'&&closed===1,{code,closed});}
    {let closed=0;const up=new Updater(access(['SMK-37 Pro']));up.bind({name:'SMK-37 Pro',id:{model:'SMK-37 Pro'},link:{close(){}}});up.access=access(['SMK-37 Elite']);let code;try{up.bind({name:'SMK-37 Elite',id:{model:'SMK-37 Elite'},link:{close(){closed++;}}});}catch(e){code=e.code;}row('changed MIDI port cannot rebind',code==='ambiguous'&&closed===1,{code,closed});}
    return rows;
   }""")
   for r in result:check(r['pass'],model+' actual binding: '+r['name'],r['details'])
   check(not errors,model+' controller/binding no page exceptions',errors);await p.close()
  for mode in ['corrupt','truncated','oversized','swapped','http503','interrupted']:
   p,errors,_=await new_page(browser,mode=mode)
   await p.wait_for_function("!image && ($('status').textContent===t('badpkg') || $('status').textContent.startsWith(t('loadfail')))")
   s=await p.evaluate(STATE)
   check(not s['image'] and s['disabled'] and not s['href'] and s['midiCalls']==0,mode+' actual package response fails closed',s)
   check(not errors,mode+' refusal no page exception',errors);await p.close()
  # Current verified bytes revoked/malformed at the actual click boundary.
  for mutation in ['revoked','malformed','empty']:
   p,errors,_=await new_page(browser);await wait_ready(p);downloads=[];p.on('download',lambda d:downloads.append(d))
   if mutation=='revoked':await p.evaluate('catalogLive.versions[0].revoked=true')
   elif mutation=='malformed':await p.evaluate('catalogLive.versions=null')
   else:await p.evaluate('()=>{catalogLive.versions=[];fillVersions();}')
   if await p.locator('#download').is_visible():await p.locator('#download').click()
   await p.wait_for_timeout(60);s=await p.evaluate(STATE)
   check(not downloads and not s['href'] and s['disabled'] and s['midiCalls']==0,mutation+' verified selection cannot retain an admitted download/install')
   check(not errors,mutation+' no page exception',errors);await p.close()
  # Delay initial Pro request, complete Elite first, then release stale Pro.
  p,errors,pending=await new_page(browser,hold=True)
  await p.wait_for_function("$('go').disabled && !$('download').getAttribute('href')")
  while not pending:await asyncio.sleep(.01)
  check(await p.evaluate('!image && __midiCalls===0'),'pending actual fetch has no admission')
  await p.locator('input[name=model][value=elite]').check()
  while len(pending)<2:await asyncio.sleep(.01)
  next(event for model,event in pending if model=='elite').set();await wait_ready(p)
  next(event for model,event in pending if model=='pro').set();await p.wait_for_timeout(100)
  s=await p.evaluate(STATE)
  check(s['model']=='elite' and s['product']==MODELS['elite']['product'] and not s['ack'],'stale real Pro callback cannot overwrite selected Elite or restore consent',s)
  async with p.expect_download() as info:await p.locator('#download').click()
  check(pathlib.Path(await (await info.value).path()).read_bytes()==RAW['elite'],'post-switch downloaded bytes remain exact Elite')
  check(not errors,'stale fetch no page exception',errors);await p.close()
  await browser.close()
 finally_result={'checks':len(checks),'passed':sum(r['pass'] for r in checks),'failed':sum(not r['pass'] for r in checks),'results':checks,'traffic':traffic,'prepared_html_sha256':HTML_HASH,'unmodified_page_positive_combinations':12,'diagnostic_probe':'Additional state/mutation/binding negatives use one appended module-scope eval accessor; no application/parser/controller/MIDI Updater function replaced. Files on disk untouched.','private_controller_positive':'NOT freshly executed; exact-byte independent join evidence reused separately','external_browser_https':'NOT tested: package requests deliberately fulfilled from independently verified public bytes','hardware_tested':False,'source_functions_replaced':False}
 (OUT/'qualified-real-page-results.json').write_text(json.dumps(finally_result,indent=2,ensure_ascii=False)+'\n')
 print(json.dumps({k:finally_result[k] for k in ['checks','passed','failed','external_browser_https','private_controller_positive']}),flush=True)
 server.shutdown()
 raise SystemExit(1 if finally_result['failed'] else 0)
try:asyncio.run(main())
except BaseException as e:
 if not isinstance(e,SystemExit):
  (OUT/'harness-error.json').write_text(json.dumps({'error':repr(e),'completed_checks':checks},indent=2)+'\n')
 raise
