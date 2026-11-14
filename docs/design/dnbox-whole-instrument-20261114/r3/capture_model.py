"""Generate native proposal PNGs and check affected design interactions only."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright

R=Path(__file__).parent
async def main():
 html=(R/'workbench.html').read_bytes();(R/'tested-model-input.html').write_bytes(html);out=R/'native';out.mkdir(exist_ok=True)
 frames=[];checks=[];errors=[];requests=[]
 async with async_playwright() as pw:
  b=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  p=await b.new_page(viewport={'width':1240,'height':1000},device_scale_factor=1);p.set_default_timeout(10000)
  p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url))
  await p.route('**/*',lambda rt:rt.abort());await p.set_content(html.decode());await p.wait_for_function('window.__ready')
  async def save(name,fixture,patch=None):
   await p.evaluate('([f,patch])=>{proposal.reset(f);if(patch)proposal.configure(patch)}',[fixture,patch])
   d=await p.evaluate('proposal.render()');raw=await p.locator('#lcd').evaluate("e=>e.toDataURL('image/png').split(',')[1]");data=base64.b64decode(raw);(out/(name+'.png')).write_bytes(data)
   bad=[x for x in d['bounds'] if x['x']<0 or x['x']+x['width']>240.1 or x['y']<0 or x['y']+x['height']>240]
   frames.append({'id':name,'fixture':fixture,'patch':patch,'path':'native/'+name+'.png','sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'geometry':[240,240],'evidence_class':'R3 PROPOSAL CANVAS; NOT FIRMWARE','state':d['state'],'text_bounds':d['bounds'],'bounds_failures':bad})
  for fixture in ['drum','808variant','chicago','miami','juno','drone','home','wait','savefail','song','break','fx','mix','load']:await save(fixture,fixture)
  await save('song-neighbors','song',{'songBars':10,'page':1})
  await save('drum-page49','drum',{'length':64,'stepPage':3,'playing':False})
  for engine,fixture in [('CHICAGO','chicago'),('MIAMI','miami'),('JUNO','juno'),('DRONE','drone')]:
   await p.evaluate('(f)=>proposal.reset(f)',fixture);n=await p.evaluate('proposal.pageCount()')
   for page in range(n):await save(engine.lower()+'-page'+str(page+1),fixture,{'page':page,'soundPage':page})
  def check(name,ok,detail=None):
   checks.append({'name':name,'pass':bool(ok),'detail':detail});assert ok,name
  # Topology is checked against independently extracted current source metadata.
  for fixture,expected in [('chicago',8),('miami',6),('juno',5),('drone',3)]:
   await p.evaluate('(f)=>proposal.reset(f)',fixture);await p.evaluate('proposal.configure({page:0,soundPage:0})')
   await p.evaluate("proposal.event('K2',-1)");s=await p.evaluate('proposal.state');check(fixture+' reverse wrap includes SOUND',s['page']==expected-1)
   await p.evaluate("proposal.event('PARA')");check(fixture+' PARA wraps to first row',(await p.evaluate('proposal.state'))['page']==0)
   await p.evaluate("proposal.event('SCALE')");check(fixture+' SCALE opens full deep list',(await p.evaluate('proposal.state'))['overlay']=='LIST')
   await p.evaluate("proposal.event('PATCH')");check(fixture+' PATCH restores matrix',(await p.evaluate('proposal.state'))['overlay'] is None)
  # Source route: remembered FILTER1 -> SEQ -> original step hold -> PARA -> K5.
  await p.evaluate("proposal.reset('chicago');proposal.event('SEQ');proposal.pad(4,true);proposal.event('PARA');proposal.event('K5')")
  s=await p.evaluate('proposal.state');check('Held step cutoff lock leaves base unchanged',s['page']==1 and len(s['locks'])==1 and not s['values'])
  await p.evaluate('proposal.release();proposal.event("K5")');s=await p.evaluate('proposal.state');check('Release restores base scope without clearing step lock',len(s['locks'])==1 and len(s['values'])==1)
  await p.evaluate("proposal.reset('drum');proposal.event('K7')");check('Unmapped drum K7 is unchanged and clean',not (await p.evaluate('proposal.state'))['dirty'])
  await p.evaluate("proposal.reset('drum');proposal.configure({length:16,stepPage:1,playing:false});proposal.pad(4)");s=await p.evaluate('proposal.state');check('Source out-of-length hidden write preserved',21 in s['steps'][0] and s['length']==16)
  await p.evaluate("proposal.reset('wait');proposal.event('K8')");s=await p.evaluate('proposal.state');check('WAIT retains direct Home pump input',s['view']=='HOME' and s['wait'] and s['pump']>4)
  text=await p.evaluate('proposal.render().bounds.map(x=>x.text)');check('WAIT keeps all four performance labels',all(x in text for x in ['TEMPO','SWING','FILTER','PUMP']))
  await p.evaluate("proposal.reset('drum');proposal.configure({playing:false})");lights=await p.evaluate('proposal.lights()');check('Stopped kick pads reflect only four stored hits',lights['pads']==[2 if i in [0,4,8,12] else 0 for i in range(16)])
  await p.evaluate('proposal.configure({playing:true,playhead:4})');lights=await p.evaluate('proposal.lights()');check('Stored-hit playhead inverts pad4 off',lights['pads'][4]==0)
  await p.evaluate('proposal.pad(4,true)');lights=await p.evaluate('proposal.lights()');check('Held input overrides playhead inversion',lights['pads'][4]==2)
  await p.evaluate('proposal.configure({lightsEnabled:false})');lights=await p.evaluate('proposal.lights()');check('LIGHTS off retains held-input source layer',lights['pads']==[2 if i==4 else 0 for i in range(16)])
  await p.evaluate("proposal.reset('drum');proposal.configure({playing:false,muted:[0]})");lights=await p.evaluate('proposal.lights()');check('Mute does not erase selected-track pad content',lights['pads'][0]==2 and lights['pads'][4]==2)
  await p.evaluate('proposal.configure({rec:true})');lights=await p.evaluate('proposal.lights()');check('Stopped REC remains separate from PLAY',lights['buttons']['SEQ REC'] and not lights['buttons']['SEQ PLAY'])
  await p.evaluate("proposal.reset('wait');proposal.configure({queued:2,framePhase:0})");a=await p.evaluate('proposal.lights()');await p.evaluate('proposal.configure({framePhase:8})');b8=await p.evaluate('proposal.lights()');check('Home cue pad retains frame8 layer independently of WAIT',a['pads'][1]==0 and b8['pads'][1]==2 and a['pads'][0]==2 and b8['pads'][0]==2)
  await p.evaluate('proposal.configure({framePhase:32})');a=await p.evaluate('proposal.lights()');check('WAIT PLAY uses separate frame32 snapshot',a['buttons']['SEQ PLAY'])
  check('All native text inside240square',not any(f['bounds_failures'] for f in frames),[{'id':f['id'],'failures':f['bounds_failures']} for f in frames if f['bounds_failures']])
  check('No page errors',not errors,errors);check('No external requests',not requests,requests)
  await b.close()
 (R/'frame-manifest.json').write_text(json.dumps(frames,indent=2)+'\n')
 (R/'model-checks.json').write_text(json.dumps({'input_html_sha256':hashlib.sha256(html).hexdigest(),'scope':'R3 model topology/held-lock scope/bottom-control invariants, not firmware or current emulator','checks':checks,'errors':errors,'requests':requests,'acceptance':'NOT ACCEPTED; parent fresh-review route and current capture intake pending'},indent=2)+'\n')
 pairs={name:'data:image/png;base64,'+base64.b64encode((out/(name+'.png')).read_bytes()).decode() for name in ['drum','chicago','home','song']}
 (R/'proposal-images.json').write_text(json.dumps(pairs)+'\n')
 print(json.dumps({'frames':len(frames),'model_checks':len(checks),'failed':sum(not x['pass'] for x in checks),'text_bounds_failures':sum(len(f['bounds_failures']) for f in frames),'evidence':'proposal only'}))

if __name__=='__main__':asyncio.run(main())
