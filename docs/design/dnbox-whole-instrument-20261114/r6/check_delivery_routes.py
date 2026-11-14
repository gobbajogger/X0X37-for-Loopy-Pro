"""Affected R6 DOM/input/composition checks; no firmware, audio or storage."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
def sha(b):return hashlib.sha256(b).hexdigest()
async def main():
 checks=[];errors=[];requests=[]
 def ck(name,ok,detail=None):checks.append({'name':name,'pass':bool(ok),'detail':detail})
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000})
  p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url) if r.url.startswith(('http:','https:')) else None)
  await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  async def ev(s):return await p.evaluate(s)
  await ev("proposal.reset('load');proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Normal stopped LOAD PLAY cancels and commands start',s['overlay'] is None and s['playing'])
  await ev("proposal.reset('load');proposal.configure({playing:true});proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Normal running LOAD PLAY cancels and commands stop',s['overlay'] is None and not s['playing'])
  await ev("proposal.reset('load');proposal.event('KNOB BANK');proposal.event('SCALE');proposal.event('K5')");s=await ev('proposal.state');ck('Clip LOAD masks SCALE and scalar controls; no confirmation',s['overlay']=='ASK' and s['clipPage'] is None)
  await ev("proposal.event('SEQ PLAY')");s=await ev('proposal.state');ck('Clip PLAY is host request and leaves LOAD question',s['overlay']=='ASK' and s['clipRequests'][-1]['type']=='SEQ PLAY host request' and not s['playing'])
  await ev("proposal.event('PATCH')");ck('Clip PATCH cancels LOAD question',(await ev('proposal.state.overlay')) is None)
  await ev("proposal.reset('drum');proposal.configure({playing:false});proposal.buttonDown('CHORD');proposal.pad(4)");s=await ev('proposal.state');l=await ev('proposal.lights()');ck('SEQ CHORD toggles shared accent and projects accent pads without changing hits',s['kits'][0]['accent']==[5] and s['steps'][0]==[1,5,9,13] and l['pads']==[0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0]);await ev("proposal.buttonUp('CHORD');proposal.buttonDown('CHORD');proposal.pad(4);proposal.buttonUp('CHORD')");ck('Second CHORD gesture clears only shared accent',not (await ev('proposal.state.kits[0].accent')) and (await ev('proposal.state.steps[0]'))==[1,5,9,13])
  await ev("proposal.reset('mix808');proposal.buttonDown('CHORD');proposal.pad(8);proposal.buttonUp('CHORD')");s=await ev('proposal.state');ck('Drum MIX shared accent uses visible808 kit only',s['kits'][1]['accent']==[9] and not s['kits'][0]['accent'])
  await ev("proposal.reset('drum-para');proposal.buttonDown('CHORD');proposal.pad(4);proposal.buttonUp('CHORD')");ck('Drum PART CHORD pad still auditions without hidden hit or accent',not (await ev('proposal.state.kits[0].accent')) and (await ev('proposal.state.steps[0]'))==[1,5,9,13])
  await ev("proposal.reset('fresh');proposal.configure({dirty:true,framePhase:0})");a=await ev('proposal.lights().buttons.BT');await ev('proposal.configure({framePhase:8})');z=await ev('proposal.lights().buttons.BT');ck('Unsaved BT follows source frame-bit8',not a and z)
  await ev("proposal.configure({save:'SAVE FAILED',framePhase:0})");a=await ev('proposal.lights().buttons.BT');await ev('proposal.configure({framePhase:2})');z=await ev('proposal.lights().buttons.BT');ck('Failed BT follows distinct source frame-bit2',not a and z)
  await ev("proposal.configure({saveQueued:true,framePhase:0})");a=await ev('proposal.lights().buttons.BT');await ev('proposal.configure({framePhase:8})');z=await ev('proposal.lights().buttons.BT');ck('Queued BT stays on; source priority precedes failure',a and z)
  await ev("proposal.reset('song');proposal.pad(0);proposal.event('SEQ PLAY');proposal.event('PARA');proposal.configure({framePhase:0})");a=await ev('proposal.lights().buttons.PATCH');await ev('proposal.configure({framePhase:8})');z=await ev('proposal.lights().buttons.PATCH');ck('Active Song PATCH follows bit8 outside HOME/SONG',not a and z)
  await ev("proposal.reset('chicago');proposal.configure({lightsEnabled:false});proposal.noteDown(11)");l=await ev('proposal.lights()');ck('Central white-key held input projects to its pad even with LIGHTSoff',l['pads'][3]==2);await ev('proposal.noteUp(11)');ck('Central white-key release clears only that held pad',(await ev('proposal.lights().pads[3]'))==0)
  await p.locator('[data-tab=\"model\"]').click();await ev("proposal.reset('chicago')");await p.locator('#panelButtons [data-ev=\">\"]').click();a=await ev('proposal.state.page');await p.locator('#panelButtons [data-ev=\"<\"]').click();ck('Published physical arrows preserve ordinary fast parameter navigation',a==1 and (await ev('proposal.state.page'))==0)
  await ev("proposal.reset('drum');proposal.configure({playing:false});proposal.pad(4,true);proposal.buttonDown('<');proposal.buttonUp('<')");ck('Ordinary arrow held-step route reaches source chance95',(await ev('proposal.state.chance'))==95)
  await ev("proposal.reset('chicago');proposal.buttonDown('PATCH');proposal.buttonDown('>');proposal.buttonUp('>');proposal.buttonUp('PATCH')");ck('PATCH-held arrows do not leak a parameter page edit',(await ev('proposal.state.page'))==0)
  ck('All18 physical button positions are visible, unfinished routes explicitly disabled',await p.locator('#panelButtons button').count()==18 and await p.locator('#panelButtons button:disabled').count()==2)
  await ev("proposal.reset('song');proposal.pad(0)");filled=await ev("(()=>{const a=g.getImageData(22,189,6,5).data;return Array.from(a).filter((v,i)=>i%4<3&&v>60).length})()");ck('Native Song identity cue uses filled7x7 tiles, not hollow substitutes',filled>30)
  # Drive each published guide through its actual DOM navigation once.
  await p.locator('[data-tab="model"]').click();guides=await ev('D.guides')
  for i,guide in enumerate(guides):
   await p.locator('#guideChoice').select_option(str(i));await p.locator('#startGuide').click()
   for stage in guide['stages']:await p.locator('#nextGuide').click()
   s=await ev('proposal.state');name=guide['id'];ok={'64':s['stepPage']==3 and s['held']==-1,'motion':not s['motionLanes'],'copy':s.get('lastCopy') is not None,'cue':s['pendingPatterns']==[-1]*5,'record':s['patternData'][0]['bass'][1]['notes'][2]['note']==56,'panic':not s['panicLatched']}[name]
   ck('Published DOM guide completes: '+name,ok,{'progress':await p.locator('#guideProgress').inner_text(),'view':s['view']})
  # Browser pointer capture is tested on this local prototype, never hardware timing.
  await ev("proposal.reset('drum');proposal.configure({playing:false})");pad=p.locator('[data-pad="4"]');await pad.scroll_into_view_if_needed();bb=await pad.bounding_box();await p.mouse.move(bb['x']+bb['width']/2,bb['y']+bb['height']/2);await p.mouse.down();await p.wait_for_timeout(340);await p.mouse.move(1,1);await p.mouse.up();s=await ev('proposal.state')
  ck('Browser captured pointer release outside pad preserves held hit and clears input',s['steps'][0]==[1,5,9,13] and s['held']==-1 and not s['physicalPadsHeld'])
  await pad.focus();await p.keyboard.down('Space');await p.keyboard.up('Space');s=await ev('proposal.state');ck('Keyboard pad tap has one down/up and deletes existing hit',s['steps'][0]==[1,9,13] and not s['physicalPadsHeld'])
  await ev("proposal.reset('chicago')");key=p.locator('[data-physical="17"]');await key.focus();await p.keyboard.down('Space');await p.keyboard.up('Space');ck('Physical-key keyboard release clears its live owner',not (await ev('proposal.state.liveNotes')))
  settings=await ev('D.settings');component=base64.b64decode(settings['component_b64']);ck('Reserved Settings component bytes remain exact',sha(component)=='edaf4eb27b6fc2c8199d8d00fbc2492d10600dabe25870ccbb8491f6a23bb2a6')
  await p.locator('[data-tab="settings"]').click()
  for i,c in enumerate(settings['cases']):
   await p.locator('#settingsChoice').select_option(str(i));await p.locator('#openSettings').click();await p.wait_for_function("document.getElementById('settingsStage').textContent.startsWith('Original R2 inspection task:')")
   ck('R2 original task staged through bridge: '+c['task'],await p.locator('#settingsFrame').count()==1 and c['task'] in await p.locator('#settingsStage').inner_text())
   await p.locator('#closeSettings').click();ck('R2 task disposed without hidden live frame: '+c['task'],await p.locator('#settingsFrame').count()==0)
  await p.locator('#openSettings').click();await p.wait_for_function("document.getElementById('settingsStage').textContent.startsWith('Original R2 inspection task:')");await p.locator('[data-tab="model"]').click();ck('Leaving Settings destroys its browsing context',await p.locator('#settingsFrame').count()==0)
  await ev("proposal.event('GLOBE')");ck('Global routes to reserved Settings presentation',await p.locator('#settings').is_visible())
  await p.locator('#showLoad').evaluate('(e)=>e.click()');ck('Settings LOAD host bridge opens existing normal question',await p.locator('#model').is_visible() and (await ev('proposal.state.overlay'))=='ASK')
  # All curated comparisons must decode; hashes are checked separately in final asset verification.
  await p.locator('[data-tab="compare"]').click();pairs=await ev('D.pairs')
  for i,pair in enumerate(pairs):
   await p.locator('#pairChoice').select_option(str(i));await p.wait_for_function("Array.from(document.querySelectorAll('#comparison img')).every(i=>i.complete&&i.naturalWidth===240&&i.naturalHeight===240)");ck('Curated pair decodes at native240: '+pair['id'],await p.locator('#comparison img').count()==2)
  for width in [390,768]:
   await p.set_viewport_size({'width':width,'height':844});await p.locator('[data-tab="model"]').click();ck('Owner model layout fits viewport '+str(width),await ev('document.documentElement.scrollWidth<=innerWidth'))
  ck('Affected DOM/model/composition routes have no browser exception',not errors,errors);ck('Self-contained artifact uses no external network',not requests,requests);await b.close()
 receipt={'classification':'Producer local-browser proposal delivery/input checks; no emulator, firmware, physical, audio, storage or independent acceptance','html_sha256':sha((R/'workbench.html').read_bytes()),'renderer_sha256':sha((R/'proposal.js').read_bytes()),'checks':checks,'passed':sum(x['pass'] for x in checks),'total':len(checks),'limits':['Pointer capture qualified only in local Chromium; not physical firmware timing','No real iOS device opening','Reuses reserved Settings producer proof; tests only changed host composition','Whole instrument and host/peer/saved lifecycle remain incomplete']};(R/'delivery-route-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'passed':receipt['passed'],'total':receipt['total'],'failures':[x for x in checks if not x['pass']]}))
asyncio.run(main())
