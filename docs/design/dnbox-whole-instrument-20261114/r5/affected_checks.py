"""Focused R5 proposal browser checks, not firmware execution."""
import asyncio,json,hashlib,base64
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
async def main():
 checks=[];frames=[];errors=[]
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000},device_scale_factor=1)
  p.on('pageerror',lambda e:errors.append(str(e)));await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  async def ev(s):return await p.evaluate(s)
  def check(n,ok,d=None):checks.append({'name':n,'pass':bool(ok),'detail':d})
  await ev("proposal.reset('drum');proposal.event('PARA')")
  a=await ev('({c:proposal.control(0,0),labels:proposal.render().bounds.map(x=>x.text)})')
  check('Drum Tune keeps raw domain',a['c']['domain']=='drum' and a['c']['q']==34 and '34' in a['labels'],a)
  check('Drum Attack and Decay units',all(x in a['labels'] for x in ['10%','1.0s']),a['labels'])
  await ev("proposal.event('K5',4)");c=await ev('proposal.control(0,0)');check('Drum Tune accepts edit34→38',c['q']==38 and (await ev('proposal.controlText(proposal.control(0,0))'))=='38')
  await ev('proposal.black(1)');v=await ev('proposal.controlText(proposal.control(0,2))');await ev("proposal.event('K7',4)");w=await ev('proposal.controlText(proposal.control(0,2))');check('Snare typed Decay changes340→361ms',v=='340ms' and w=='361ms',{'before':v,'after':w})
  await ev("proposal.reset('mix909');proposal.pad(4,true)");s=await ev('proposal.state');check('MIX existing hit retained on down',s['kits'][0]['steps'][0]==[1,5,9,13] and s['held']==-1)
  await ev('proposal.release(800)');check('MIX800ms hold/release retains4hits',(await ev('proposal.state.kits[0].steps[0]'))==[1,5,9,13])
  await ev('proposal.pad(4)');check('Existing short tap removes only on release',(await ev('proposal.state.kits[0].steps[0]'))==[1,9,13])
  await ev('proposal.pad(4)');check('Empty short tap creates without same-release removal',(await ev('proposal.state.kits[0].steps[0]'))==[1,9,13,5])
  await ev("proposal.reset('mix909');proposal.pad(4,true);proposal.black(1);proposal.release(50)");check('Owner change cancels deferred deletion',(await ev('proposal.state.kits[0].steps[0]'))==[1,5,9,13])
  await ev("proposal.reset('song')");l=await ev('proposal.lights()');check('Empty Song usedP1 dim not bright',l['pads'][0]==1 and [x//4 for x in l['rgb']]==[31,18,5],l)
  await ev("proposal.reset('chicago');proposal.configure({motion:{'3:CHICAGO:2':0}});proposal.event('SEQ');proposal.pad(4,true);proposal.event('PARA')");c=await ev('proposal.control(1,0)');check('Held parameter without lock uses base, suppresses motion',c['q']==32 and c['scope']=='base',c)
  await ev('proposal.release()');c=await ev('proposal.control(1,0)');check('Release restores active motion owner',c['q']==0 and c['scope']=='motion',c)
  # Exhaust actual mapped drum page controls, including KIT and every voice ID.
  owners=[]
  for kit in range(2):
   for voice in range(11):
    await ev("proposal.reset('drum')");await p.evaluate('x=>proposal.configure({part:x[0],track:x[1],view:"PART",page:0})',[kit,voice]);pages=await ev('proposal.pages()')
    for pi,page in enumerate(pages):
     for ki,cell in enumerate(page['cells']):
      if not cell.get('label'):continue
      await p.evaluate('x=>proposal.configure({page:x})',pi)
      c=await ev(f'proposal.control({pi},{ki})');before=c['q'];text=await ev(f'proposal.controlText(proposal.control({pi},{ki}))');await ev(f"proposal.event('K{ki+5}',1)");after=await ev(f'proposal.control({pi},{ki})')
      owners.append(c['domain']=='drum' and c['meta']['local_id']==cell['parameter_id'] and c['voice']==cell.get('voice',voice) and after['q']==min(c['meta']['maximum_raw'],before+1) and bool(text))
  check('Every mapped drum page uses typed owner and accepts clamped edit',all(owners),{'controls':len(owners)})
  # Newly source-backed oscillator must follow accepted edits and real scope ownership.
  await ev("proposal.reset('chicago')");a=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(9,51,29,19).data)")
  await ev("proposal.event('K5',127)");bwave=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(9,51,29,19).data)");check('Native one-cycle WAVE changes at127 without changing parameter identity',a!=bwave and (await ev('proposal.control(0,0).q'))==127)
  await ev("proposal.configure({page:1});proposal.event('K5',50);proposal.configure({page:0})");cwave=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(9,51,29,19).data)");check('Non-WAVE control does not change fixed-profile oscillator projection',bwave==cwave)
  await ev("proposal.reset('chicago');proposal.event('SEQ');proposal.pad(4,true);proposal.event('PARA');proposal.event('K5',127)");c=await ev('proposal.control(0,0)');check('WAVE held edit uses explicit step owner',c['q']==127 and c['scope']=='step');await ev('proposal.release()');c=await ev('proposal.control(0,0)');check('WAVE original release restores base0',c['q']==0 and c['scope']=='base')
  await ev("proposal.reset('home');proposal.configure({playing:false,queued:2,framePhase:0})");a=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,0,240,20).data)");await ev('proposal.configure({framePhase:16})');bb=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,0,240,20).data)");check('HOME STOP pending LCD stays steady at phases0/16',a==bb)
  await ev('proposal.configure({wait:true,framePhase:0})');a=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,0,240,20).data)");await ev('proposal.configure({framePhase:16})');bb=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,0,240,20).data)");check('HOME WAIT pending LCD stays steady at phases0/16',a==bb)
  await ev("proposal.reset('drum');proposal.event('PARA');proposal.event('SCALE');proposal.configure({list:7})");a=await ev('proposal.controlText(proposal.control(-1,7))');await ev("proposal.event('K3')");bb=await ev('proposal.controlText(proposal.control(-1,7))');check('MORE drum Dist enum uses local descriptor and accepts bounded edit',a=='Diode' and bb=='Clip',{'before':a,'after':bb})
  # Personally exercise DOM pointer capture and keyboard release; no old overlap qualification is borrowed.
  await ev("proposal.reset('mix909')");await p.locator('[data-tab="model"]').click();box=await p.locator('[data-pad="4"]').bounding_box();await p.mouse.move(box['x']+15,box['y']+15);await p.mouse.down();await p.wait_for_timeout(350);await p.mouse.move(1100,950);await p.mouse.up();sstate=await ev('proposal.state');check('DOM captured-pointer outside release afterhold preserves existing hit and clears owner',sstate['kits'][0]['steps'][0]==[1,5,9,13] and sstate['heldInput']==-1 and sstate['press'] is None)
  await p.locator('[data-pad="4"]').focus();await p.keyboard.down('Space');await p.wait_for_timeout(350);await p.keyboard.up('Space');check('DOM keyboard held release preserves existing hit',(await ev('proposal.state.kits[0].steps[0]'))==[1,5,9,13])
  await p.mouse.move(box['x']+15,box['y']+15);await p.mouse.down();await p.locator('[data-pad="4"]').dispatch_event('pointercancel',{'pointerId':1,'bubbles':True});await p.mouse.up();check('Pointer cancel clears logical owner without deferred deletion',(await ev('proposal.state.press')) is None)
  await ev("proposal.reset('chicago-envelope');proposal.event('K5')");labels=await ev('proposal.render().bounds.map(x=>x.text)');check('Existing Chicago envelope keeps edited-stage caption/readout',any(x.startswith('ATTACK ') for x in labels) and any(x.startswith('K5 A ') for x in labels),labels)
  await p.wait_for_timeout(950);check('Edited-stage highlight expires without another input',(await ev('proposal.state.touched')) is None)
  await ev("proposal.reset('juno-envelope')");bds=await ev('proposal.render().bounds');check('Juno preserves large18px numeric values and label positions',all(any(x['text']==v and x['height']==18 for x in bds) for v in ['6%','31%','72%']))
  await ev("proposal.reset('home');proposal.event('K6',1)");check('HOME SWING retains source raw/4+50 law',(await ev('proposal.state.swing'))==50 and (await ev('proposal.state.swingRaw'))==1);await ev("proposal.event('K6',3)");check('Four raw SWING steps show51percent',(await ev('proposal.state.swing'))==51)
  await ev("proposal.reset('home');proposal.event('K8',1)");check('HOME PUMP retains source24dB/127 law',abs((await ev('proposal.state.pump'))-24*22/127)<1e-9)
  # Static top status is always present; six truthful states. No measured percentage invented.
  (R/'native').mkdir(exist_ok=True)
  for f in ['fresh','drum-para','snare-para','drum','808variant','chicago','chicago-envelope','miami','juno','juno-envelope','home','wait','song','mix','mix909','mix808','break','fx','load']:
   await p.evaluate('f=>proposal.reset(f)',f);d=await ev('proposal.render()');raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));(R/'native'/f'{f}.png').write_bytes(raw)
   bad=[x for x in d['bounds'] if x['x']<0 or x['x']+x['width']>240.1 or x['y']<0 or x['y']+x['height']>240.1]
   frames.append({'id':f,'path':f'native/{f}.png','sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'bounds_failures':bad,'state':d['state'],'class':'R5 PROPOSAL; NOT FIRMWARE'})
  for engine,fixture in [('CHICAGO','chicago'),('MIAMI','miami'),('JUNO','juno')]:
   await p.evaluate('f=>proposal.reset(f)',fixture);count=await ev('proposal.pageCount()')
   for page in range(count):
    await p.evaluate('x=>proposal.configure({page:x})',page);d=await ev('proposal.render()');raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));id=engine.lower()+'-page-'+str(page+1);(R/'native'/f'{id}.png').write_bytes(raw);bad=[x for x in d['bounds'] if x['x']<0 or x['x']+x['width']>240.1 or x['y']<0 or x['y']+x['height']>240.1];frames.append({'id':id,'path':f'native/{id}.png','sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'bounds_failures':bad,'state':d['state'],'class':'VERIFICATION INVENTORY; R5 PROPOSAL; not firmware or CP1 transaction acceptance'})
  for value in [-1,0,1,2,3,4]:
   await ev("proposal.reset('juno')");await p.evaluate('v=>proposal.configure({power:v})',value);raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));(R/'native'/f'power-{value}.png').write_bytes(raw)
  check('Native frames stay240×240 and text in bounds',all(not x['bounds_failures'] for x in frames),[{'id':x['id'],'bad':x['bounds_failures']} for x in frames if x['bounds_failures']])
  check('Browser has no uncaught exceptions',not errors,errors)
  await b.close()
 receipt={'class':'PRODUCER_BROWSER_PROPOSAL_ONLY','html_sha256':hashlib.sha256((R/'workbench.html').read_bytes()).hexdigest(),'renderer_sha256':hashlib.sha256((R/'proposal.js').read_bytes()).hexdigest(),'checks':checks,'passed':sum(x['pass'] for x in checks),'total':len(checks),'frames':frames,'limits':['Not independent acceptance','Not firmware/emulator/audio/device execution','R5 is changing and not frozen']}
 (R/'affected-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'passed':receipt['passed'],'total':receipt['total'],'failures':[x for x in checks if not x['pass']]}))
asyncio.run(main())
