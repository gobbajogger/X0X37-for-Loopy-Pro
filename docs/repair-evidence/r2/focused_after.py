# SPDX-License-Identifier: GPL-3.0-only
# Adapt the pinned independent diagnostic's success expectation after fixing F1.
# Supply --qa-evidence path/to/docs/independent-qa/pr8-r1 from evidence commit
# 76e059e17db12265bf2062144eb7feee1df7e5f8; PR8_QA_SOURCE selects source checkout.
import argparse, asyncio, json, sys
from pathlib import Path
from playwright.async_api import async_playwright
ap=argparse.ArgumentParser();ap.add_argument('--qa-evidence',required=True);args=ap.parse_args()
sys.path.insert(0,str(Path(args.qa_evidence).resolve()))
import independent_browser as q
async def main():
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  for mutation in ['c.versions[0].models=null','c.versions={}','c.versions[0]=null']:
   p=await q.page(b);await p.wait_for_function('iqa.state().href?.startsWith("blob:")')
   errors=[];downloads=[];p.on('pageerror',lambda e:errors.append(str(e)));p.on('download',lambda d:downloads.append(d))
   old=await p.locator('#download').get_attribute('href')
   await p.evaluate('iqa.mutate(c=>{'+mutation+'})')
   await p.locator('#download').click();await p.wait_for_timeout(100)
   state=await p.evaluate('iqa.state()')
   revoked=await p.evaluate('(url)=>fetch(url).then(()=>false,()=>true)',old)
   result={'case':'malformed-selection-real-click','mutation':mutation,'downloads':len(downloads),'page_errors':errors,'state':state,'old_blob_revoked':revoked}
   print(json.dumps(result),flush=True)
   assert not downloads and not errors and state['href'] is None and not state['image'] and state['disabled'] and revoked
   await p.close()
  for width in [320,375,768,1280]:
   p=await q.page(b,'/builds/',width);await p.wait_for_function('document.querySelector("#history").children.length===117')
   result=await p.evaluate('''()=>({width:innerWidth,scrollWidth:document.documentElement.scrollWidth,history:document.querySelectorAll('#history article').length,builds:document.querySelectorAll('#builds article').length,overflow:[...document.querySelectorAll('.item,.item h3,.item p,.item dd,.tag')].filter(e=>e.scrollWidth>e.clientWidth+1).map(e=>e.textContent.slice(0,80))})''')
   print(json.dumps({'case':'real-history-layout',**result}),flush=True)
   assert result['scrollWidth']<=width and not result['overflow'] and result['history']==117 and result['builds']==2
   await p.close()
  await b.close()
 q.S.shutdown()
 print('PASS: all three actual-click regressions revoke old Blob without download/errors; all four full-history widths and card contents fit',flush=True)
asyncio.run(main())
