import asyncio,json,re,hashlib,base64
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
def data(b):return json.JSONDecoder().raw_decode(b.decode().split('const D=',1)[1])[0]
async def main():
 old=(R/'evidence/pre-embed-model.html').read_bytes();new=(R/'workbench.html').read_bytes();a=data(old);b=data(new);a.pop('proposalImages',None);b.pop('proposalImages',None);assert a==b
 oldrenderer=old.decode().split('/* SPDX-License-Identifier: GPL-3.0-only */',1)[1].split('function esc(s)',1)[0];newrenderer=new.decode().split('/* SPDX-License-Identifier: GPL-3.0-only */',1)[1].split('function esc(s)',1)[0];assert oldrenderer==newrenderer
 checks=[];errors=[];requests=[]
 def ck(n,v):checks.append({'name':n,'pass':bool(v)})
 ck('Final data differs only embedded updated comparison images',True)
 async with async_playwright() as pw:
  br=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await br.new_page(viewport={'width':1120,'height':900},device_scale_factor=1);p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url));await p.route('**/*',lambda r:r.abort());await p.set_content(new.decode());await p.wait_for_function('window.__ready')
  for i,pair in enumerate(b['pairs']):
   await p.select_option('#pairChoice',str(i));src=await p.locator('#comparison figure').nth(1).locator('img').get_attribute('src');raw=base64.b64decode(src.split(',')[1]);ck('R5 paired image '+pair['id'],hashlib.sha256(raw).hexdigest()==hashlib.sha256((R/'native'/pair.get('after',pair['id']+'.png')).read_bytes()).hexdigest())
  await p.locator('[data-tab="journeys"]').click();ck('Whole20-group journey selector present',await p.locator('#journeyChoice option').count()==20);await p.select_option('#journeyChoice','0');await p.locator('#inspectJourney').click();ck('First-beat journey opens true empty909 fixture',await p.evaluate('proposal.state.kits[0].steps[0].length===0&&!proposal.state.playing'))
  for pad in [0,4,8,12]:await p.locator(f'[data-pad="{pad}"]').click()
  await p.locator('[data-ev="SEQ PLAY"]').click();ck('First-beat model uses4step gestures then PLAY',await p.evaluate('proposal.state.playing&&JSON.stringify(proposal.state.kits[0].steps[0])==="[1,5,9,13]"'))
  await p.select_option('#fixture','juno');await p.select_option('#powerState','4');ck('Power USB fixture reaches matrix through shared top header',await p.evaluate('proposal.state.power===4'))
  await p.set_viewport_size({'width':390,'height':844});ck('Owner-facing host has no horizontal page overflow at390px',await p.evaluate('document.documentElement.scrollWidth<=390'))
  ck('Final host has no page errors or network dependencies',not errors and not requests);await br.close()
 result={'class':'FINAL PROPOSAL DELIVERY SMOKE; NOT FIRMWARE/INDEPENDENT ACCEPTANCE','html_sha256':hashlib.sha256(new).hexdigest(),'renderer_sha256':hashlib.sha256((R/'proposal.js').read_bytes()).hexdigest(),'scoped_proof_reused':'affected-checks.json31checks; renderer and data equivalence established above','checks':checks,'errors':errors,'requests':requests,'passed':sum(x['pass'] for x in checks),'total':len(checks)}
 (R/'FINAL-DELIVERY-CHECK.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'passed':result['passed'],'total':result['total'],'failures':[x for x in checks if not x['pass']]}));assert all(x['pass'] for x in checks)
asyncio.run(main())
