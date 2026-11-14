"""Changed physical/key input paths exercised through the actual proposal DOM."""
import asyncio,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
async def main():
 checks=[];errors=[]
 def ck(n,v,d=None):checks.append({'name':n,'pass':bool(v),'detail':d})
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000});p.on('pageerror',lambda e:errors.append(str(e)));await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready');await p.locator('[data-tab="model"]').click()
  async def ev(s):return await p.evaluate(s)
  async def pointer_down(selector):
   e=p.locator(selector);await e.scroll_into_view_if_needed();r=await e.bounding_box();await p.mouse.move(r['x']+r['width']/2,r['y']+r['height']/2);await p.mouse.down()
  await ev("proposal.prepareJourney('64');proposal.event('OCTAVE+');proposal.event('OCTAVE+');proposal.event('OCTAVE+')");await pointer_down('[data-physical="31"]');ck('37-key DOM pointer uses physical31→held64',(await ev('proposal.state.held'))==64);await p.wait_for_timeout(330);await p.mouse.move(1,1);await p.mouse.up();ck('Captured physical key release outside clears GRID owner without deleting64',(await ev('proposal.state.held'))==-1 and (await ev('proposal.state.steps[0]'))==[1,64])
  await p.locator('[data-physical="31"]').focus();await p.keyboard.down('Space');ck('Keyboard press uses same GRID64 role',(await ev('proposal.state.held'))==64);await p.locator('[data-ev="K2"][data-d="-1"]').click();ck('Held physical key stays original owner while K2 edits chance',(await ev('proposal.state.chance'))==95);await p.keyboard.up('Space');ck('Key release after focus moves to knob releases original key',(await ev('proposal.state.held'))==-1 and not (await ev('proposal.state.physicalOwners')))
  await ev("proposal.reset('drum');proposal.configure({playing:false})");await p.locator('[data-physical="8"]').click();ck('Physical black G#3 selects source snare rather than synth note',(await ev('proposal.state.track'))==1 and not (await ev('proposal.state.liveNotes')))
  await ev("proposal.reset('gen-known')");await p.locator('[data-physical="9"]').focus();await p.keyboard.down('Space');await p.locator('[data-ev="K5"]').last.click();ck('Published GEN physical hold + knob edits only intended cell',(await ev('proposal.state.patternData[0].bass[1].notes[2].note'))==56 and (await ev('proposal.state.view'))=='GEN');await p.keyboard.up('Space');ck('Published GEN keyboard release restores generator controls',(await ev('proposal.state.held'))==-1 and len(await ev('proposal.state.genUndo'))==1)
  await ev("proposal.reset('gen-known')");await p.locator('[data-pad="2"]').focus();await p.keyboard.down('Space');await p.locator('[data-ev="K5"]').last.click();await p.keyboard.up('Space');ck('Pad keyboard owner also survives focus transfer',(await ev('proposal.state.held'))==-1 and (await ev('proposal.state.patternData[0].bass[1].notes[2].note'))==56)
  await ev("proposal.reset('drum');proposal.configure({playing:false})");await p.locator('[data-physical="12"]').focus();await p.keyboard.down('Space');await ev("dispatchEvent(new Event('blur'))");ck('Browser cancellation ends physical GRID owner without short-tap deletion',not (await ev('proposal.state.physicalOwners')) and (await ev('proposal.state.steps[0]'))==[1,5,9,13]);await p.keyboard.up('Space')
  ck('Changed DOM routes have no browser exception',not errors,errors);await b.close()
 out={'classification':'Producer actual local Chromium DOM checks; proposal only, not hardware/firmware timing','html_sha256':hashlib.sha256((R/'workbench.html').read_bytes()).hexdigest(),'passed':sum(x['pass'] for x in checks),'total':len(checks),'checks':checks,'limits':['Keyboard/pointer focus behavior qualified in local Chromium only','Not iOS/device or independent acceptance']};(R/'PHYSICAL-DOM-CHECKS.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'passed':out['passed'],'total':out['total'],'failures':[x for x in checks if not x['pass']]}))
asyncio.run(main())
