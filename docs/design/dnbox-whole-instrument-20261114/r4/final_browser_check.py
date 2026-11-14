"""Final delivered-HTML checks only; unchanged core reuses43 scoped producer cases."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
async def main():
 raw=(R/'workbench.html').read_bytes();errors=[];requests=[];checks=[]
 def check(name,ok):checks.append({'name':name,'pass':bool(ok)});assert ok,name
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1240,'height':1000},device_scale_factor=1)
  p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url));await p.route('**/*',lambda r:r.abort());await p.set_content(raw.decode());await p.wait_for_function('window.__ready')
  check('Four explicit current reference comparisons',await p.locator('#pairChoice option').count()==4)
  check('No active Drone fixture offered',await p.locator('#fixture option[value="drone"]').count()==0)
  await p.get_by_role('button',name='Use the controls',exact=True).click();await p.select_option('#fixture','juno');await p.get_by_role('button',name='K5 +',exact=True).click()
  check('Actual HTML button edits continuous Juno WAVE',await p.evaluate('proposal.control(0,0).q')==43)
  await p.select_option('#fixture','mix909');await p.locator('[data-track="1"]').click();await p.locator('[data-pad="2"]').click()
  check('Actual HTML MIX controls retain kit/page and write',await p.evaluate('proposal.state.view==="MIX"&&proposal.state.page===1&&proposal.state.kits[0].track===1&&proposal.state.kits[0].steps[1].includes(3)'))
  check('MIX view button exposes lit feedback',await p.locator('#panelButtons [data-ev="NOTE REPEAT"]').get_attribute('data-lit')=='true')
  for name in ['drum','chicago','juno','home','wait','mix909']:
   await p.select_option('#fixture',name);png=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL('image/png').split(',')[1]"));check(name+' final HTML matches native file',png==(R/'native'/(name+'.png')).read_bytes())
  check('No final page errors',not errors);check('No final external requests',not requests);await b.close()
 result={'input_html_sha256':hashlib.sha256(raw).hexdigest(),'scope':'Final delivered HTML actual button/dropdown and native/static image validation; no firmware','checks':checks,'errors':errors,'requests':requests,'independent_review':'PENDING same reviewer'};(R/'FINAL-BROWSER-CHECK.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'checks':len(checks),'failed':0,'html_sha256':result['input_html_sha256']}))
if __name__=='__main__':asyncio.run(main())
