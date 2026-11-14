"""New R5-findings-only producer checks. No runtime or independent acceptance."""
import asyncio,json,hashlib,base64
from pathlib import Path
from playwright.async_api import async_playwright
from receipt_tools import save_receipt
R=Path(__file__).parent
async def main():
 checks=[];frames=[];errors=[]
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000},device_scale_factor=1)
  p.on('pageerror',lambda e:errors.append(str(e)));await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  async def ev(s):return await p.evaluate(s)
  def check(n,ok,d=None):checks.append({'name':n,'pass':bool(ok),'detail':d})
  async def frame(id):
   raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));(R/'native').mkdir(exist_ok=True);(R/'native'/f'{id}.png').write_bytes(raw);frames.append({'file':f'native/{id}.png','bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'state':await ev('proposal.state'),'class':'R6 working proposal only'})
  await ev("proposal.reset('juno-envelope')")
  # Neighboring group values at y50..70 retain bright source ink; labels remain muted.
  pix=await ev("(()=>{const c=document.getElementById('lcd').getContext('2d');return Array.from(c.getImageData(4,50,232,21).data)})()")
  n=sum(pix[i:i+3]==[222,230,222] for i in range(0,len(pix),4));check('Juno inactive numeric full ink restored to source RGB222,230,222',n>30,{'full_ink_pixels':n});await frame('juno-envelope-corrected')
  for part,engine in [(0,'909'),(1,'808'),(3,'CHICAGO'),(4,'MIAMI'),(4,'JUNO')]:
   await ev("proposal.reset('drum')");await p.evaluate('x=>proposal.configure({part:x[0],engine:x[1],view:"SEQ",page:0})',[part,engine]);bd=await ev('proposal.render().bounds[0]');check('Explicit '+engine+' SEQ view identity fits before pattern/status',bd['text'].endswith(' SEQ') and bd['x']+bd['width']<98,bd)
  await ev("proposal.reset('song');proposal.event('K5',1)");s=await ev('proposal.state');check('Empty Song K5 clamps without creating bar',s['songBars']==0 and s['songSel']==0)
  await ev('proposal.pad(0)');s=await ev('proposal.state');check('Song pad writes all5 P1 then selects BAR2 END',s['songBars']==1 and s['songSel']==1 and s['songData'][0]=={'pat':[0]*5,'mute':0} and s['queued']==0,s['songData']);check('END usedP1 remains dim',(await ev('proposal.lights().pads[0]'))==1);await frame('song-first-bar-end')
  await ev("proposal.event('K5',-1)");s=await ev('proposal.state');check('BAR K5 selects stored bar; Song P1 bright',s['songSel']==0 and (await ev('proposal.lights().pads[0]'))==2);await frame('song-select-bar1')
  await ev("proposal.event('K6',3)");s=await ev('proposal.state');check('BAR1 K6 edits only909 P1→P4',s['songData'][0]['pat']==[3,0,0,0,0]);await ev("proposal.event('K2',1);proposal.event('K8',3);proposal.black(2)");s=await ev('proposal.state');check('Repeat x4 and BREAK mute occupy independent bitfields',s['songData'][0]['mute']==112,s['songData'][0]);await frame('song-repeat-mute')
  await ev("proposal.event('K5',1);proposal.event('K7',2)");s=await ev('proposal.state');check('END part edit copies full previous bar then changes onlyBASS A, holds cursor',s['songBars']==2 and s['songSel']==1 and s['songData'][1]=={'pat':[3,0,2,0,0],'mute':112},s['songData'])
  await ev("proposal.event('K5',1);proposal.event('K8',1)");s=await ev('proposal.state');check('END REPEAT reads x1 then writes x2 preserving low mute bits',s['songData'][2]['mute']==48,s['songData'][2])
  await ev("proposal.reset('song');proposal.configure({heldPatch:true});proposal.pad(4)");s=await ev('proposal.state');check('PATCH+Song pad cues without authoring',s['songBars']==0 and s['queued']==5)
  await ev("proposal.reset('song');proposal.event('OCTAVE+');proposal.pad(15)");s=await ev('proposal.state');check('Song second bank pad15 writes P32 all5',s['songData'][0]['pat']==[31]*5)
  await ev("proposal.reset('song');proposal.configure({songBars:192,songSel:191});proposal.pad(2)");s=await ev('proposal.state');check('Song capacity192 does not add END or wrap cursor',s['songBars']==192 and s['songSel']==191 and s['songData'][191]['pat']==[2]*5)
  await ev("proposal.reset('song');proposal.pad(0);proposal.event('SEQ PLAY')");s=await ev('proposal.state');check('PLAY from Song END starts stored bar0',s['playing'] and s['songOn'] and s['songPos']==0)
  check('No browser exception in changed cases',not errors,errors)
  await b.close()
 receipt={'classification':'Producer browser proposal checks; not firmware/emulator/device execution or independent acceptance','renderer_sha256':hashlib.sha256((R/'proposal.js').read_bytes()).hexdigest(),'checks':checks,'passed':sum(c['pass'] for c in checks),'total':len(checks),'frames':frames,'limits':['No broad unchanged R4/R5 proof rerun','R6 working successor, not frozen','Song duration/playback engine not simulated']};save_receipt(R,'review-correction-checks.json',receipt);print(json.dumps({'passed':receipt['passed'],'total':receipt['total'],'failures':[c for c in checks if not c['pass']]}))
asyncio.run(main())
