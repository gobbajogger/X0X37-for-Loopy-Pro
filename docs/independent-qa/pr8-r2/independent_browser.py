import asyncio,hashlib,json,subprocess,threading,os
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
from playwright.async_api import async_playwright
R=Path(os.environ.get('PR8_QA_SOURCE','/workspace/pr8-independent-qa')); E=Path(__file__).resolve().parent
def package(product):
 b=bytearray([0x7d]*1056)
 for i,c in enumerate(product):b[48*i+47]=(ord(c)+i+1)&255
 return bytes(b)
RAW={m:package('SMK-37 '+n+'_900') for m,n in [('pro','Pro'),('elite','Elite')]}
LOCAL=package('SMK-37 Elite_016')
C={'schema':1,'latest':'qa-independent','history':[],'versions':[{'version':'qa-independent','status':'dev-checked','config':'dev','hardware_tested':False,'models':{m:{'product':'SMK-37 '+n+'_900','size':len(RAW[m]),'sha256':hashlib.sha256(RAW[m]).hexdigest(),'controller_sha256':hashlib.sha256(b'controller').hexdigest(),'pkg':'https://fixture.invalid/'+hashlib.sha256(RAW[m]).hexdigest()+'/'+m+'.fwsc'}for m,n in [('pro','Pro'),('elite','Elite')]}}]}
SCRIPT="import{renderInstaller}from'./scripts/render-qualified-nightly.mjs';process.stdout.write(renderInstaller(JSON.parse(process.argv[1])));"
HTML=subprocess.check_output(['node','--input-type=module','-e',SCRIPT,json.dumps(C)],cwd=R).decode()
# Additional test hooks only in served test bytes, never repository/generated production bytes.
PROBE='''window.iqa={mutate:f=>f(catalogLive), replace:c=>{catalogLive=c;fillVersions()},
 state:()=>({writing,image:!!image,product:selectedProduct,href:$("download").getAttribute("href"),disabled:$("go").disabled}),
 mock:()=>{window.calls=[];window.confirm=()=>true;midiAccess=()=>new Promise((r,j)=>{window.grant=()=>r({});window.deny=()=>j({code:"denied"})});Updater=class{async find(){return null}};installKeepingSlots=async(u,i,p)=>{calls.push({product:p,bytes:Array.from(i)});return {}}},
 holdFile:(name)=>{selectStockFile({arrayBuffer:()=>new Promise((r,j)=>{window[name]=r;window[name+'Fail']=j})})},
};'''
# Arrow functions cannot be sent via Playwright serialization; evaluate within page.
INSTR=HTML.replace('(async () => {',PROBE+'\n(async () => {',1).encode()
class Handler(BaseHTTPRequestHandler):
 def do_GET(self):
  self.send_response(200);self.send_header('Content-Type','text/html' if self.path!='/' else 'text/plain');self.end_headers()
  p=self.path.split('?')[0]
  if p=='/candidate/':self.wfile.write(HTML.encode())
  elif p=='/probe/':self.wfile.write(INSTR)
  else:
   f=(R/'docs'/p.lstrip('/'))
   if f.is_dir():f=f/'index.html'
   if f.exists():self.wfile.write(f.read_bytes())
 def log_message(self,*a):pass
S=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=S.serve_forever,daemon=True).start()
checks=[];fails=[]
def check(ok,label,detail=None):
 checks.append({'label':label,'pass':bool(ok),'detail':detail});print(('PASS 'if ok else 'FAIL ')+label+((' '+json.dumps(detail))if detail is not None else ''),flush=True)
 if not ok:fails.append(label)
async def page(browser,path='/probe/',width=390):
 p=await browser.new_page(viewport={'width':width,'height':900},accept_downloads=True)
 await p.route('https://fixture.invalid/**',lambda r:r.fulfill(body=RAW[r.request.url.rsplit('/',1)[1].split('.')[0]],content_type='application/octet-stream'))
 await p.goto(f'http://127.0.0.1:{S.server_port}'+path)
 return p
async def main():
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  print('Chromium '+b.version,flush=True)
  # No test hooks or altered fetch/digest/parser for these full-page UI cases.
  for width in [320,375,768,1280]:
   for language in ['en','ja']:
    p=await page(b,'/candidate/',width)
    await p.evaluate('(l)=>{localStorage.setItem("lang",l)}',language)
    # Actual language control; inspect select/buttons below where supported.
    lang=p.locator('#lang')
    if await lang.count():await lang.click() if language=='ja' else None
    for model in ['pro','elite']:
     await p.locator('#models input[value="'+model+'"]').check()
     await p.wait_for_function('document.querySelector("#download").getAttribute("href")?.startsWith("blob:")')
     check(await p.locator('#go').is_disabled(),f'{width}/{language}/{model} pre-ack disabled')
     await p.locator('#devagree').check()
     check(await p.locator('#go').is_enabled(),f'{width}/{language}/{model} UI qualified positive')
     async with p.expect_download() as d:await p.locator('#download').click()
     dl=await d.value;data=Path(await dl.path()).read_bytes()
     check(data==RAW[model],f'{width}/{language}/{model} exact network-fixture download')
    check(await p.evaluate('document.documentElement.scrollWidth<=innerWidth'),f'{width}/{language} no overflow')
    await p.keyboard.press('Tab');check(await p.evaluate('document.activeElement!==document.body'),f'{width}/{language} keyboard focus')
    await p.screenshot(path=str(E/f'candidate-{width}-{language}.png'),full_page=True)
    await p.close()
  # Local manual recovery uses actual file-selection, parser, digest and checkbox handlers.
  p=await page(b,'/install/')
  await p.locator('#stockfile').set_input_files({'name':'synthetic-local.fwsc','mimeType':'application/octet-stream','buffer':LOCAL})
  await p.wait_for_function('!document.querySelector("#stockconfirm").disabled')
  check(await p.locator('#stockgo').is_disabled(),'manual local recovery requires acknowledgement')
  await p.locator('#stockconfirm').check();check(await p.locator('#stockgo').is_enabled(),'manual local recovery accepted with empty catalog')
  await p.locator('#stockfile').set_input_files({'name':'invalid.fwsc','mimeType':'application/octet-stream','buffer':b'invalid'})
  await p.wait_for_timeout(50);check(await p.locator('#stockgo').is_disabled(),'invalid local file clears old authorization');await p.close()
  p=await page(b);await p.evaluate('iqa.mock()')
  await p.locator('#stockfile').set_input_files({'name':'local.fwsc','mimeType':'application/octet-stream','buffer':LOCAL})
  await p.wait_for_function('!document.querySelector("#stockconfirm").disabled');await p.locator('#stockconfirm').check();await p.locator('#stockgo').click()
  check(await p.evaluate('iqa.state().writing'),'actual local reader writer lock before permission')
  await p.evaluate('grant()');await p.wait_for_function('calls.length===1 && !iqa.state().writing')
  expected=b''.join(LOCAL[i*48:i*48+47] for i in range(20))+LOCAL[960:]
  check(await p.evaluate('calls[0]')=={'product':'SMK-37 Elite_016','bytes':list(expected)},'manual local exact parsed target retained');await p.close()
  # A delayed first file must not replace a newer file, including failure completion.
  for rejected in [False,True]:
   p=await page(b);await p.evaluate('iqa.holdFile("oldFile")')
   await p.locator('#stockfile').set_input_files({'name':'new.fwsc','mimeType':'application/octet-stream','buffer':LOCAL});await p.wait_for_function('!document.querySelector("#stockconfirm").disabled')
   before=await p.locator('#dropmeta').inner_text()
   await p.evaluate('oldFileFail(Error("stale"))'if rejected else '(bytes)=>oldFile(new Uint8Array(bytes).buffer)',None if rejected else list(RAW['pro']))
   await p.wait_for_timeout(30)
   check(await p.locator('#dropmeta').inner_text()==before,'stale local file '+('rejection'if rejected else 'success')+' ignored');await p.close()
  # Extra malformed shapes after verification: click guard must cancel before default Blob navigation.
  for mutation in ['catalogLive.versions[0].models=null','catalogLive.versions={}','catalogLive.versions[0]=null']:
   p=await page(b);await p.wait_for_function('iqa.state().href?.startsWith("blob:")');errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
   result=await p.evaluate('''(mutation)=>{iqa.mutate(c=>eval(mutation.replaceAll('catalogLive','c')));const a=document.querySelector('#download');const event=new MouseEvent('click',{cancelable:true,bubbles:true});const allowed=a.dispatchEvent(event);return {allowed,href:a.getAttribute('href')};}''',mutation)
   await p.wait_for_timeout(25);result['errors']=errors
   check(not result['allowed'] and result['href'] is None,'malformed selection fails closed: '+mutation,result);await p.close()
  # Source history rendered from real repo bytes.
  for width in [320,375,768,1280]:
   p=await page(b,'/builds/',width);await p.wait_for_function('document.querySelector("#history").children.length===117')
   check(await p.locator('#builds article').count()==2 and await p.locator('#history article').count()==117,f'{width} full source history retained')
   check(await p.evaluate('document.documentElement.scrollWidth<=innerWidth'),f'{width} history no overflow');await p.close()
  await b.close()
 S.shutdown();(E/'independent-browser-results.json').write_text(json.dumps({'checks':checks,'failures':fails},indent=2));print(json.dumps({'checks':len(checks),'failed':len(fails),'failures':fails}),flush=True)
 raise SystemExit(1 if fails else 0)
if __name__=='__main__':asyncio.run(main())
