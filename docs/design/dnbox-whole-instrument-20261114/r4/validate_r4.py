"""Affected R4 producer checks. No firmware/emulator/hardware execution."""
import asyncio,base64,hashlib,json
from pathlib import Path
from playwright.async_api import async_playwright
R=Path(__file__).parent
async def main():
 html=(R/'workbench.html').read_bytes();rows=[];frames=[];errors=[];requests=[]
 async with async_playwright() as pw:
  browser=await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage']);p=await browser.new_page(viewport={'width':1240,'height':1000},device_scale_factor=1)
  p.on('pageerror',lambda e:errors.append(str(e)));p.on('request',lambda r:requests.append(r.url));await p.route('**/*',lambda r:r.abort());await p.set_content(html.decode());await p.wait_for_function('window.__ready')
  async def ev(code):return await p.evaluate(code)
  def check(name,ok,detail=None):rows.append({'name':name,'pass':bool(ok),'detail':detail})
  async def snap(id,fixture,patch=None):
   await p.evaluate('(f)=>proposal.reset(f)',fixture)
   if patch:await p.evaluate('(x)=>proposal.configure(x)',patch)
   d=await ev('proposal.render()');b64=await p.locator('#lcd').evaluate("e=>e.toDataURL('image/png').split(',')[1]");raw=base64.b64decode(b64);(R/'native').mkdir(exist_ok=True);(R/'native'/(id+'.png')).write_bytes(raw)
   bad=[x for x in d['bounds'] if x['x']<-.1 or x['x']+x['width']>240.1 or x['y']<0 or x['y']+x['height']>240]
   frames.append({'id':id,'fixture':fixture,'patch':patch,'path':'native/'+id+'.png','sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'geometry':[240,240],'evidence_class':'R4 PROPOSAL; NOT FIRMWARE','state':d['state'],'text_bounds':d['bounds'],'bounds_failures':bad})
  for name in ['drum','808variant','chicago','chicago-envelope','miami','juno','juno-envelope','home','wait','song','mix','mix909','mix808']:
   await snap(name,name)
  await snap('mix-master','mix',{'page':3})
  await snap('mix-wave','mix',{'page':4})
  await snap('song-neighbors','song',{'songBars':10,'page':1})
  # Reported edit: effective state, formatted label and curve must agree.
  await ev("proposal.reset('chicago-envelope')");a=await ev("({q:proposal.control(3,0).q,labels:proposal.render().bounds.map(x=>x.text),pixels:Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,187,240,37).data)})")
  await ev("proposal.event('K5')");b=await ev("({q:proposal.control(3,0).q,labels:proposal.render().bounds.map(x=>x.text),pixels:Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,187,240,37).data)})")
  check('Envelope K5 updates q and readout',b['q']==a['q']+1 and 'K5 A 28%' in b['labels'],{'before_q':a['q'],'after_q':b['q'],'labels':b['labels']})
  check('Envelope changed region follows accepted edit',a['pixels']!=b['pixels'])
  stage_changes=[]
  for k in range(4):
   await ev("proposal.reset('chicago-envelope')")
   a=await ev(f"({{q:proposal.control(3,{k}).q,pixels:Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,200,240,24).data)}})")
   await ev(f"proposal.event('K{k+5}',16)")
   b=await ev(f"({{q:proposal.control(3,{k}).q,labels:proposal.render().bounds.map(x=>x.text),pixels:Array.from(document.getElementById('lcd').getContext('2d').getImageData(0,200,240,24).data)}})")
   expected='K'+str(k+5)+' '+['A','D','S','R'][k]+' '+str((b['q']*100+63)//127)+'%'
   stage_changes.append(b['q']==a['q']+16 and expected in b['labels'] and a['pixels']!=b['pixels'])
  check('All four ADSR stages bind values and curve geometry',all(stage_changes),stage_changes)
  await ev("proposal.reset('chicago');proposal.event('SEQ');proposal.pad(4,true);proposal.event('PARA');proposal.event('K5')");a=await ev('({control:proposal.control(1,0),state:proposal.state})');await ev('proposal.release()');b=await ev('proposal.control(1,0)')
  check('Held lock changes shown scope then release restores base',a['control']['scope']=='step' and b['scope']=='base' and b['q']==32 and a['control']['q']==33)
  # q127 source domain and continuous Juno WAVE, rather than label-wide enum gate.
  await ev("proposal.reset('juno');proposal.event('K5')");c=await ev('proposal.control(0,0)');check('Juno WAVE remains continuous',c['q']==43)
  await ev("proposal.event('K5',1000)");check('Juno controls clamp127',(await ev('proposal.control(0,0)'))['q']==127)
  await ev("proposal.event('K5',-1000)");check('Juno controls clamp0',(await ev('proposal.control(0,0)'))['q']==0)
  await ev("proposal.reset('juno')");check('Juno factory0 exact q vector',(await ev('proposal.state.controls[4]'))==[42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100])
  # Native cue pixels at low/high values, all16 proposed Juno meanings.
  cue_changes=[]
  for page in range(4):
   for knob in range(4):
    await ev("proposal.reset('juno')");await p.evaluate('x=>proposal.configure({page:x})',page)
    await ev(f"proposal.event('K{knob+5}',-1000)");low=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(4+"+str(knob)+"*59+5,24+"+str(page)+"*50+27,23,18).data)")
    await ev(f"proposal.event('K{knob+5}',1000)");hi=await ev("Array.from(document.getElementById('lcd').getContext('2d').getImageData(4+"+str(knob)+"*59+5,24+"+str(page)+"*50+27,23,18).data)")
    cue_changes.append(low!=hi)
  check('All16 Juno glyph-only regions respond at bounds',all(cue_changes),cue_changes)
  # Source clamp and memory, including display order versus stable source IDs.
  await ev("proposal.reset('home');proposal.event('K3',-1)");check('K3 lower endpoint clamps909',(await ev('proposal.state'))['part']==0)
  await ev("proposal.event('K3',50)");s=await ev('proposal.state');check('K3 magnitude selects one neighbor',s['part']==1 and s['sourcePart']==1)
  await ev("proposal.event('K3');proposal.event('K3');proposal.event('K3');proposal.event('K3')");s=await ev('proposal.state');check('Display order ends at canonical A slot',s['part']==4 and s['sourcePart']==2)
  await ev("proposal.reset('chicago');proposal.event('PARA');proposal.event('PARA');proposal.event('K3');proposal.event('K2');proposal.event('K3',-1)");check('K3 restores B page memory',(await ev('proposal.state'))['page']==3)
  await ev("proposal.event('K3')");check('K3 restores A page memory',(await ev('proposal.state'))['page']==1)
  await ev("proposal.reset('chicago');proposal.event('FX');proposal.event('FX');proposal.event('PARA');proposal.event('FX')");check('FX shared memory survives return',(await ev('proposal.state'))['page']==1)
  await ev("proposal.reset('chicago');proposal.event('NOTE REPEAT');proposal.event('K2',4);proposal.event('PARA');proposal.event('NOTE REPEAT')");check('MIX shared memory survives return',(await ev('proposal.state'))['page']==4)
  await ev("proposal.reset('mix909');proposal.configure({page:3});proposal.event('K3',-1)");s=await ev('proposal.state');check('MIX follows clamped source part',s['part']==0 and s['page']==1)
  await ev("proposal.reset('miami');proposal.event('PATCH')");labels=await ev('proposal.render().bounds.map(x=>x.text)');check('Miami Home identity is not Chicago A','MIAMI' in labels and 'CHICAGO A' not in labels)
  await ev("proposal.reset('chicago')");check('B-slot header remains B','CHICAGO B' in (await ev('proposal.render().bounds.map(x=>x.text)')))
  # Kit content is independent from selected global part, MIX page and other kit.
  await ev("proposal.reset('drum');proposal.configure({playing:false});proposal.event('NOTE REPEAT')");check('PARTS no unheld content lights',not any((await ev('proposal.lights()'))['pads']))
  await ev("proposal.event('NOTE REPEAT')");a=await ev('proposal.lights()');check('909 MIX retains four-kick pads',a['pads']==[2 if i in [0,4,8,12] else 0 for i in range(16)] and a['owner']==0)
  await ev("proposal.black(1);proposal.pad(2)");s=await ev('proposal.state');check('MIX track/write stay on visible kit',s['view']=='MIX' and s['page']==1 and s['kits'][0]['track']==1 and s['kits'][0]['steps'][1]==[3] and s['kits'][1]['track']==0)
  await ev("proposal.configure({part:3,page:2});proposal.pad(5)");a=await ev('proposal.lights()');s=await ev('proposal.state');check('808 MIX owns data/hue with melodic selection',s['part']==3 and s['kits'][1]['steps'][0]==[6] and a['owner']==1 and a['rgb']==[127,36,32])
  await ev("proposal.pad(4,true)");s=await ev('proposal.state');check('MIX held input does not acquire p-lock scope',s['held']==-1 and s['heldInput']==5)
  await ev("proposal.release();proposal.configure({page:3})");check('MASTER no drum content',not any((await ev('proposal.lights()'))['pads']))
  await ev("proposal.configure({page:4})");check('WAVE no drum content',not any((await ev('proposal.lights()'))['pads']))
  await ev("proposal.reset('drum');proposal.event('PARA');proposal.configure({playing:false})");check('Drum PART retains step layer',any((await ev('proposal.lights()'))['pads']))
  await ev("proposal.event('FX')");check('FX does not inherit drum pads',not any((await ev('proposal.lights()'))['pads']))
  # Distinct conga silhouettes/pitch ticks, no identical rank symbol.
  ranks=[]
  for n in [20,21,22]:
   ranks.append(await ev(f"(()=>{{const old=g.getImageData(0,0,24,12);g.clearRect(0,0,24,12);drumGlyph({n},0,0,'white');const out=Array.from(g.getImageData(0,0,24,12).data);g.putImageData(old,0,0);return JSON.stringify(out)}})()"))
  check('Conga low/mid/high pictures distinct',len(set(ranks))==3)
  await ev("proposal.reset('mix909');proposal.event('K5',1000)")
  check('909 gain uses1.35 maximum','2.6dB' in (await ev('proposal.render().bounds.map(x=>x.text)')))
  await ev("proposal.reset('mix808');proposal.event('K5',1000)")
  check('808 gain uses2.0 maximum','6.0dB' in (await ev('proposal.render().bounds.map(x=>x.text)')))
  await ev("proposal.event('K5',-1000)")
  check('Zero gain reads OFF','OFF' in (await ev('proposal.render().bounds.map(x=>x.text)')))
  await ev("proposal.reset('mix909')")
  check('909 source fresh-project levels exact',(await ev('proposal.state.kits[0].mix.map(x=>x[0])'))==[61,58,52,52,52,103,122,89,60,47,42])
  check('808 source fresh-project levels exact',(await ev('proposal.state.kits[1].mix.map(x=>x[0])'))==[64]*11)
  await ev("proposal.reset('chicago');proposal.event('SEQ');proposal.pad(4,true);proposal.event('FX')")
  s=await ev('proposal.state');check('View change retires lock scope but keeps held physical feedback',s['held']==-1 and s['heldInput']==5 and (await ev('proposal.lights()'))['pads'][4]==2)
  await ev('proposal.release();proposal.configure({lightsEnabled:false})')
  check('Released LIGHTS OFF has no pad layer',not any((await ev('proposal.lights()'))['pads']))
  await ev("proposal.reset('chicago')")
  labels=await ev("[0,64,127].map(q=>proposal.controlText({q,id:14,label:'MODE'}))")
  check('Chicago edited MODE matches native selector order',labels==['DOWN','UP','UP/DN'])
  labels=await ev("[0,64,127].map(q=>proposal.controlText({q,id:15,label:'RANGE'}))")
  check('Chicago edited RANGE preserves FULL',labels==['1 OCT','2 OCT','FULL'])
  labels=await ev("[0,64,127].map(q=>proposal.controlText({q,id:4,label:'ENV'}))")
  check('Chicago ENV exact centered neutral',labels==['-100%','0%','+100%'])
  await ev("proposal.reset('miami')")
  labels=await ev("[0,63,64,65,127].map(q=>proposal.controlText({q,id:7,label:'TUNE'}))")
  check('Miami TUNE exact centered signed hundredths',labels==['-5.00','-0.08','0.00','+0.08','+5.00'])
  for fixture in ['chicago','miami','juno']:
   await p.evaluate('f=>proposal.reset(f)',fixture);n=await ev('proposal.pageCount()')
   for page in range(n):await snap(fixture+'-page'+str(page+1),fixture,{'page':page})
  check('Affected native text bounds inside240',not any(x['bounds_failures'] for x in frames),[{'id':x['id'],'bad':x['bounds_failures']} for x in frames if x['bounds_failures']])
  check('No page errors',not errors,errors);check('No external requests',not requests,requests)
  await browser.close()
 # Preserve failure receipts; do not silently turn an old failure into a pass.
 result={'input_html_sha256':hashlib.sha256(html).hexdigest(),'scope':'Affected proposal controls/render/light/source-navigation checks only; not independent review or firmware','checks':rows,'errors':errors,'requests':requests,'acceptance':'PENDING same reviewer affected recheck'}
 (R/'model-checks.json').write_text(json.dumps(result,indent=2)+'\n');(R/'frame-manifest.json').write_text(json.dumps(frames,indent=2)+'\n')
 pairs={name:'data:image/png;base64,'+base64.b64encode((R/'native'/(name+'.png')).read_bytes()).decode() for name in ['drum','chicago','home','song']};(R/'proposal-images.json').write_text(json.dumps(pairs)+'\n')
 print(json.dumps({'checks':len(rows),'failed':[x for x in rows if not x['pass']],'frames':len(frames),'input_sha256':result['input_html_sha256']}));assert all(x['pass'] for x in rows),'Affected checks failed; inspect receipt'
if __name__=='__main__':asyncio.run(main())
