"""Render only changed/curated R7 owner frames; retain exact source references."""
import ast,asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent;OLD=R.parent/'revision-r6'
tree=ast.parse((OLD/'render_curated.py').read_text());RECIPES=ast.literal_eval(next(n.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='RECIPES' for t in n.targets)))
RECIPES.update({'record':"proposal.reset('chicago');proposal.event('SEQ');proposal.event('SEQ REC');proposal.buttonDown('NOTE REPEAT');proposal.physicalDown(17);proposal.physicalUp(17,100);proposal.buttonUp('NOTE REPEAT');proposal.event('SEQ REC');proposal.physicalDown(5)",'gen-normal':"proposal.reset('gen-known')",'gen-held':"proposal.reset('gen-known');proposal.physicalDown(9);proposal.event('OCTAVE+')"})
async def main():
 pairs=json.loads((R/'comparisons.json').read_text());frames=[];images={};errors=[]
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await b.new_page(viewport={'width':1220,'height':1000});p.on('pageerror',lambda e:errors.append(str(e)));await p.route('**/*',lambda r:r.abort());await p.set_content((R/'workbench.html').read_text());await p.wait_for_function('window.__ready')
  for pair in pairs:
   await p.evaluate(RECIPES[pair['id']]);d=await p.evaluate('proposal.render()')
   if pair['id']=='miami-native':await p.evaluate('g.drawImage(clipImages.miami,0,20,240,220,0,20,240,220)')
   raw=base64.b64decode(await p.locator('#lcd').evaluate("e=>e.toDataURL().split(',')[1]"));target=R/'native'/pair['after'];target.write_bytes(raw);images[pair['id']]='data:image/png;base64,'+base64.b64encode(raw).decode();old=OLD/'native'/pair['after'];unchanged=old.exists() and old.read_bytes()==raw
   frames.append({'id':pair['id'],'file':'native/'+pair['after'],'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'recipe':RECIPES[pair['id']],'bounds':d['bounds'],'classification':'R7 proposal browser render; not firmware execution','unchanged_R6_pixels':unchanged,'body_composition':'Original native body/lower pixels' if pair['id'] in ['miami-native','clip'] else 'Source-derived proposal drawing'})
  await b.close()
 inputs={n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in ['proposal.js','physical-role-router.js','generator-math.js','generator-proposal.js']};receipt={'classification':'Curated R7 proposal native frames, separate from unchanged actual-before captures','inputs':inputs,'pairs':len(pairs),'frames':frames,'browser_errors':errors,'limits':['No executed R7 firmware','Motion/copy/clock/native factory phases remain explicitly bounded','Native reference files are unmodified; not all pairs claim identical hidden state']};(R/'CURATED-NATIVE-MANIFEST.json').write_text(json.dumps(receipt,indent=2)+'\n');(R/'proposal-images.json').write_text(json.dumps(images,separators=(',',':'))+'\n');print(json.dumps({'pairs':len(pairs),'unchanged_R6_pixels':sum(f['unchanged_R6_pixels'] for f in frames),'errors':errors}))
asyncio.run(main())
