"""Affected final R7 delivery/guide/Settings-host checks; proposal only."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
def sha(b):return hashlib.sha256(b).hexdigest()
async def main():
 checks=[];errors=[];requests=[];traces=[]
 def ck(name,ok,detail=None):checks.append({'name':name,'pass':bool(ok),'detail':detail})
 async with async_playwright() as pw:
  browser=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  p=await browser.new_page(viewport={'width':1220,'height':1000});p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url) if r.url.startswith(('http:','https:')) else None)
  await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  async def ev(s):return await p.evaluate(s)
  await p.locator('[data-tab="model"]').click();guides=await ev('D.guides')
  for i,guide in enumerate(guides):
   await p.locator('#guideChoice').select_option(str(i));await p.locator('#startGuide').click()
   for j,stage in enumerate(guide['stages']):
    await p.locator('#nextGuide').click();s=await ev('proposal.state');pic=await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]")
    traces.append({'guide':guide['id'],'stage':j+1,'label':stage['label'],'view':s['view'],'part':s['sourcePart'],'pattern':s['pattern'],'held':s['held'],'playing':s['playing'],'rec':s['rec'],'lcd_sha256':sha(base64.b64decode(pic)),'lights':await ev('proposal.lights()')})
   s=await ev('proposal.state');name=guide['id'];ok={'64':s['stepPage']==3 and s['held']==-1,'motion':not s['motionLanes'],'copy':s.get('lastCopy') is not None,'cue':s['pendingPatterns']==[-1]*5,'record':s['patternData'][0]['bass'][1]['notes'][2]['note']==56,'panic':not s['panicLatched'],'gen':s['view']=='GEN' and not s['playing'] and s['held']==-1 and bool(s['genUndo'])}[name]
   ck('Published control-gesture guide completes: '+name,ok,{'progress':await p.locator('#guideProgress').inner_text(),'view':s['view']})
  ck('All18 physical button positions retained; only unmodeled FADER BANK disabled',await p.locator('#panelButtons button').count()==18 and await p.locator('#panelButtons button:disabled').count()==1 and await p.locator('#panelButtons [data-ev="FADER BANK"]:disabled').count()==1)
  ck('37 physical keys and16 pads remain available',await p.locator('[data-physical]').count()==37 and await p.locator('[data-pad]').count()==16)
  await ev("proposal.reset('load');proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Normal stopped LOAD PLAY cancels and starts command',s['overlay'] is None and s['playing'])
  await ev("proposal.reset('load');proposal.configure({playing:true});proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Normal running LOAD PLAY cancels and stops command',s['overlay'] is None and not s['playing'])
  await ev("proposal.reset('load');proposal.event('KNOB BANK');proposal.event('SCALE');proposal.event('K5');proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Clip LOAD masks SCALE/scalars and PLAY remains host request',s['overlay']=='ASK' and s['clipPage'] is None and s['clipRequests'][-1]['type']=='SEQ PLAY host request' and not s['playing']);await ev("proposal.event('PATCH')");ck('Clip PATCH cancels LOAD',await ev('proposal.state.overlay') is None)
  settings=await ev('D.settings');old=json.loads((R.parent/'revision-r6/settings-composition.json').read_text());component=base64.b64decode(settings['component_b64']);ck('Settings component remains exact original bytes',sha(component)=='edaf4eb27b6fc2c8199d8d00fbc2492d10600dabe25870ccbb8491f6a23bb2a6');expected_cases=old['cases'];expected_cases[1]['cost']=expected_cases[1]['cost'].replace('CP1 owns that source-bound layer.','Preset and held-note policy still owns that source-bound layer.');ck('Settings pair pixels/cases retained; one host dependency label clarified',settings['cases']==expected_cases)
  await p.locator('[data-tab="settings"]').click()
  for i,c in enumerate(settings['cases']):
   await p.locator('#settingsChoice').select_option(str(i));await p.locator('#openSettings').click();await p.wait_for_function("document.getElementById('settingsStage').textContent.startsWith('Original R2 inspection task:')")
   ck('Original Settings task stages through host: '+c['task'],await p.locator('#settingsFrame').count()==1 and c['task'] in await p.locator('#settingsStage').inner_text())
   await p.locator('#closeSettings').click();ck('Settings browsing context disposed: '+c['task'],await p.locator('#settingsFrame').count()==0)
  await p.locator('#openSettings').click();await p.wait_for_function("document.getElementById('settingsStage').textContent.startsWith('Original R2 inspection task:')")
  for route,expected in settings['references'].items():
   await p.frame_locator('#settingsFrame').locator('a').evaluate_all("(as,path)=>{const a=as.find(a=>a.getAttribute('href')===path);if(a)a.click()}",route)
   await p.wait_for_function("expected=>document.getElementById('settingsReferenceText').textContent===expected",arg=expected)
   ck('Settings sanitized local reference opens: '+route,(await p.locator('#settingsReferenceText').inner_text())==expected)
  await p.locator('[data-tab="model"]').scroll_into_view_if_needed();await p.locator('[data-tab="model"]').click();ck('Tab exit disposes original Settings frame',await p.locator('#settingsFrame').count()==0)
  await ev("proposal.reset('chicago');proposal.event('GLOBE')");ck('Normal Global keeps reserved Settings host route',await p.locator('#settings').is_visible());await p.locator('#showLoad').evaluate('e=>e.click()');ck('Settings LOAD bridge retains normal question',(await ev('proposal.state.overlay'))=='ASK' and await p.locator('#model').is_visible())
  await p.locator('[data-tab="compare"]').click();pairs=await ev('D.pairs')
  for i,pair in enumerate(pairs):
   await p.locator('#pairChoice').select_option(str(i));await p.wait_for_function("Array.from(document.querySelectorAll('#comparison img')).every(i=>i.complete&&i.naturalWidth===240&&i.naturalHeight===240)");ck('Curated native pair decodes: '+pair['id'],await p.locator('#comparison img').count()==2)
  for width in [390,768]:
   await p.set_viewport_size({'width':width,'height':844});await p.locator('[data-tab="model"]').click();ck('Walkthrough fits viewport '+str(width),await ev('document.documentElement.scrollWidth<=innerWidth'))
  ck('Changed delivery/guide/host routes have no browser exception',not errors,errors);ck('Self-contained walkthrough makes no external network request',not requests,requests);await browser.close()
 out={'classification':'Producer local Chromium proposal checks; not independent acceptance, firmware, iOS or device execution','html_sha256':sha((R/'workbench.html').read_bytes()),'inputs':{n:sha((R/n).read_bytes()) for n in ['proposal.js','physical-role-router.js','generator-math.js','generator-proposal.js','settings-host.js']},'checks':checks,'passed':sum(c['pass'] for c in checks),'total':len(checks),'trace_count':len(traces),'traces':traces,'reuse':'Original105 Settings producer and prior54 composition checks remain scoped unchanged proof; only current host/guide routes checked here.'}
 (R/'DELIVERY-DELTA-CHECKS.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'passed':out['passed'],'total':out['total'],'trace_count':len(traces),'failures':[c for c in checks if not c['pass']]}));assert out['passed']==out['total']
asyncio.run(main())
