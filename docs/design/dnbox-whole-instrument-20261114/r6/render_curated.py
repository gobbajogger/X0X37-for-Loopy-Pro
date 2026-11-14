"""Curated native proposal frames. Existing actual LCD inputs are never painted."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
from receipt_tools import save_receipt
R=Path(__file__).parent
RECIPES={
 'drum':"proposal.reset('drum')",
 'miami-native':"proposal.reset('miami');proposal.configure({dirty:false});g.drawImage(clipImages.miami,0,20,240,220,0,20,240,220)",
 'juno':"proposal.reset('juno-envelope')",
 'mix909':"proposal.reset('mix909')",
 'break':"proposal.reset('break')",
 'sequence64':"proposal.prepareJourney('64');proposal.event('OCTAVE+');proposal.event('OCTAVE+');proposal.event('OCTAVE+');proposal.pad(15,true);proposal.event('K2',-1);proposal.release(800);proposal.event('SEQ PLAY');proposal.event('K4');proposal.configure({playheads:[17,1,1,1,1]})",
 'motion':"proposal.reset('chicago');proposal.configure({page:1,playing:true,values:{'3:CHICAGO:2':38},lastMotionKey:'3:CHICAGO:2',motionLanes:{'0:3:3:CHICAGO:2':{pattern:0,part:3,key:'3:CHICAGO:2',values:[40,...Array(63).fill(null)],applied:null,recReq:false,holdReq:false,holdLeft:15,lastRec:null,quiet:0}}})",
 'copy':"proposal.reset('fresh');proposal.configure({view:'HOME',heldSave:true,dirty:true});proposal.configure({patternData:proposal.state.patternData.map((p,i)=>{if(i===0)p.drums[0].steps[0]=[1];if(i===1)p.drums[0].steps[0]=[2];return p})});proposal.pad(1)",
 'cue':"proposal.reset('fresh');proposal.configure({view:'HOME',part:1,playing:true,dirty:true,queuedMutes:false,partMutes:2,pendingPatterns:[1,2,-1,-1,-1],playheads:[38,6,6,6,6],framePhase:16});proposal.configure({patternData:proposal.state.patternData.map((p,i)=>{if(i===0){p.drums[0].length=64;p.drums[0].steps[0]=[1];}return p})})",
 'record':"proposal.prepareJourney('record');proposal.event('SEQ REC');proposal.noteDown(17);proposal.noteUp(17);proposal.buttonDown('NOTE REPEAT');proposal.event('OCTAVE+');proposal.buttonUp('NOTE REPEAT');proposal.noteDown(19);proposal.noteUp(19);proposal.buttonDown('CHORD');proposal.event('OCTAVE+');proposal.buttonUp('CHORD');proposal.noteDown(21);proposal.noteUp(21);proposal.event('SEQ REC');proposal.pad(2,true);proposal.event('OCTAVE+')",
 'song':"proposal.reset('song');proposal.pad(0)",
 'panic':"proposal.prepareJourney('panic');proposal.noteDown(17);proposal.buttonDown('FX');proposal.buttonDown('SEQ')",
 'clip':"proposal.reset('fresh');proposal.event('KNOB BANK')",
}
async def main():
 pairs=json.loads((R/'comparisons.json').read_text());frames=[];images={}
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000});await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  for pair in pairs:
   code=RECIPES[pair['id']];await p.evaluate(code);d=await p.evaluate('proposal.render()');
   if pair['id']=='miami-native':
    await p.evaluate('g.drawImage(clipImages.miami,0,20,240,220,0,20,240,220)');d['state']={'classification':'Static native-body composition; not interactive reducer state','header':{'engine':'MIAMI A','page':0,'power':3,'tempo':125,'playing':False,'dirty':False},'body_source':'reference/elite-default-A-v1-01-default-A-sound-native.png','body_sha256':'e3d4e10fdbe2262fab32323178dd38c6a98c7f563aaf4f5f871dc40870861c31'}
   raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));(R/'native').mkdir(exist_ok=True);(R/'native'/pair['after']).write_bytes(raw);images[pair['id']]='data:image/png;base64,'+base64.b64encode(raw).decode();frames.append({'id':pair['id'],'file':'native/'+pair['after'],'recipe':code,'state':d['state'],'bounds':d['bounds'],'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'classification':'R6 proposal pixels; not firmware execution'})
  # The separate inspection inventory retains every engine page without contact sheets.
  for f in ['chicago','miami','juno','drum-para','808variant','fx','mix','mix808','load','wait']:
   await p.evaluate('f=>proposal.reset(f)',f);count=await p.evaluate('proposal.pageCount()') if f in ['chicago','miami','juno'] else 1
   for page in range(count):
    if f in ['chicago','miami','juno']:await p.evaluate('page=>proposal.configure({page})',page)
    d=await p.evaluate('proposal.render()');raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));name=f+'-page'+str(page+1)+'.png';(R/'native'/name).write_bytes(raw);frames.append({'id':name,'file':'native/'+name,'state':d['state'],'bounds':d['bounds'],'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'classification':'Separate layout inspection inventory; controlled proposal fixture; no current-state equivalence claim'})
  await b.close()
 (R/'proposal-images.json').write_text(json.dumps(images,separators=(',',':'))+'\n');receipt={'renderer_sha256':hashlib.sha256((R/'proposal.js').read_bytes()).hexdigest(),'pairs':len(pairs),'frames':frames,'limits':['Actual-before inputs remain immutable','Current phase/native factory/capture-discrepancy limits are named in comparisons','Inventory is not whole-design acceptance']};save_receipt(R,'CURATED-NATIVE-MANIFEST.json',receipt);print(json.dumps({'pairs':len(pairs),'native_frames':len(frames)}))
asyncio.run(main())
