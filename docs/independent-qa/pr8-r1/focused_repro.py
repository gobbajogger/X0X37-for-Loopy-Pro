import asyncio,json,hashlib
from pathlib import Path
import independent_browser as q
from playwright.async_api import async_playwright
async def main():
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  p=await q.page(b);await p.wait_for_function('iqa.state().href?.startsWith("blob:")')
  errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
  await p.evaluate('iqa.mutate(c=>c.versions[0].models=null)')
  async with p.expect_download() as dl:await p.locator('#download').click()
  d=await dl.value;data=Path(await d.path()).read_bytes()
  print(json.dumps({'case':'malformed-models-real-click','downloaded':True,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'equals_previous_pro_package':data==q.RAW['pro'],'page_errors':errors,'href':await p.locator('#download').get_attribute('href')}),flush=True)
  await p.close()
  for width in [320,375,768,1280]:
   p=await q.page(b,'/builds/',width);await p.wait_for_function('document.querySelector("#history").children.length===117')
   result=await p.evaluate('''()=>({width:innerWidth,scrollWidth:document.documentElement.scrollWidth,overflow:[...document.querySelectorAll('article,h3,strong,dd')].map(e=>({tag:e.tagName,text:e.textContent.slice(0,240),left:e.getBoundingClientRect().left,right:e.getBoundingClientRect().right,scrollWidth:e.scrollWidth,clientWidth:e.clientWidth})).filter(e=>e.right>innerWidth||e.scrollWidth>e.clientWidth).slice(0,12)})''')
   print(json.dumps({'case':'real-history-overflow',**result}),flush=True)
   await p.screenshot(path=str(q.E/f'history-{width}.png'),full_page=True);await p.close()
  await b.close()
 q.S.shutdown()
asyncio.run(main())
