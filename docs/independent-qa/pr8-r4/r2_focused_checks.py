# Independent R2 acceptance checks, synthetic fixtures; no device or publication.
import asyncio,json,hashlib
from pathlib import Path
import independent_browser as q
from playwright.async_api import async_playwright
checks=[]
def check(ok,label,data=None):
 checks.append({'label':label,'pass':bool(ok),'data':data});print(('PASS 'if ok else 'FAIL ')+label+' '+json.dumps(data),flush=True)
async def main():
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox']);print('Chromium '+b.version,flush=True)
  for mutation in ['c.versions[0].models=null','c.versions={}','c.versions[0]=null','c.versions.push(c.versions[0])','c.versions[0].models.pro={...c.versions[0].models.pro}','c.versions[0].self=c.versions[0]']:
   p=await q.page(b);await p.wait_for_function('iqa.state().href?.startsWith("blob:")')
   errors=[];downloads=[];p.on('pageerror',lambda e:errors.append(str(e)));p.on('download',lambda d:downloads.append(d))
   old=await p.locator('#download').get_attribute('href');await p.locator('#devagree').check()
   await p.evaluate('iqa.mutate(c=>{'+mutation+'})');await p.locator('#download').click();await p.wait_for_timeout(120)
   st=await p.evaluate('iqa.state()');revoked=await p.evaluate('u=>fetch(u).then(()=>false,()=>true)',old)
   check(not downloads and not errors and st['href'] is None and not st['image'] and st['disabled'] and revoked,'real click denies and revokes '+mutation,{'downloads':len(downloads),'errors':errors,'state':st,'revoked':revoked})
   # Reload a valid catalog in the same page; failure must not permanently disable valid use.
   await p.evaluate('(c)=>iqa.replace(c)',q.C);await p.wait_for_function('iqa.state().href?.startsWith("blob:")')
   await p.locator('#devagree').check();check(await p.locator('#go').is_enabled(),'valid retry restores install admission '+mutation)
   await p.locator('#download').focus()
   async with p.expect_download() as dl:await p.keyboard.press('Enter')
   data=Path(await (await dl.value).path()).read_bytes()
   check(data==q.RAW['pro'],'keyboard valid retry downloads exact bytes '+mutation,{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()});await p.close()
  for width in [320,375,768,1280]:
   p=await q.page(b,'/builds/',width);await p.wait_for_function('document.querySelector("#history").children.length===117')
   geo=await p.evaluate('''()=>({width:innerWidth,scrollWidth:document.documentElement.scrollWidth,history:document.querySelectorAll('#history article').length,builds:document.querySelectorAll('#builds article').length,overflow:[...document.querySelectorAll('.item,.item h3,.item p,.item dd,.tag')].filter(e=>e.scrollWidth>e.clientWidth+1).map(e=>e.textContent.slice(0,80))})''')
   check(geo['scrollWidth']==width and not geo['overflow'] and geo['history']==117 and geo['builds']==2,'real history viewport and card contents '+str(width),geo);await p.close()
  await b.close()
 q.S.shutdown();q.E.joinpath('r2-focused-results.json').write_text(json.dumps(checks,indent=2));print(json.dumps({'checks':len(checks),'failed':sum(not c['pass'] for c in checks)}),flush=True)
 raise SystemExit(1 if any(not c['pass']for c in checks)else 0)
asyncio.run(main())
