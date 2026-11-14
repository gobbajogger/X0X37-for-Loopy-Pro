from pathlib import Path
import json, hashlib
from playwright.sync_api import sync_playwright

ROOT=Path(__file__).resolve().parent
PACKET=Path('/workspace/website-v4-inputs/PR899-runtime-431d0db8-handoff')
OUT=ROOT/'evidence';OUT.mkdir(exist_ok=True)
receipt={'scope':'New website adapter LCD readback only; unchanged runtime qualification reused','runtime':'431d0db82cbadca761fb32c6548ca4765875324f','browser_receiver_retry':False}
try:
 with sync_playwright() as p:
  b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
  pg=b.new_page(viewport={'width':1440,'height':1000})
  pg.route('**/*pro-dev.fwsc',lambda r:r.fulfill(path=str(PACKET/'runtime/fixture-pro.fwsc'),content_type='application/octet-stream',headers={'Access-Control-Allow-Origin':'*'}))
  pg.add_init_script("document.addEventListener('dnbox:booted',e=>window.nativeBoot=e.detail)")
  pg.goto('http://127.0.0.1:8674/firmware-runtime/?fw=dnbox-pro-0251')
  pg.wait_for_function("document.documentElement.dataset.ready==='true'")
  pg.locator('#start').click()
  pg.wait_for_function('window.nativeBoot?.board===1',timeout=60000)
  pg.wait_for_timeout(16000)
  frame=pg.evaluate('''()=>{const c=document.querySelector('#panel'),v=c.getContext('2d').getImageData(262,44,480,480).data,out=[];let mismatch=0;for(let y=0;y<240;y++)for(let x=0;x<240;x++){const i=((2*y)*480+2*x)*4;for(let dy=0;dy<2;dy++)for(let dx=0;dx<2;dx++)for(let k=0;k<4;k++)if(v[i+k]!==v[i+(dy*480+dx)*4+k])mismatch++;out.push(...v.slice(i,i+4));}return {width:c.width,height:c.height,mismatch,rgba:out}}''')
  assert (frame['width'],frame['height'])==(1920,880)
  assert frame['mismatch']==0
  data=bytes(frame.pop('rgba'))
  (OUT/'pro-initial.rgba').write_bytes(data)
  receipt.update(frame)
  receipt.update({'result':'PASS','browser':b.version,'native_rgba_sha256':hashlib.sha256(data).hexdigest(),'boot':pg.evaluate('window.nativeBoot'),'fixture_sha256':hashlib.sha256((PACKET/'runtime/fixture-pro.fwsc').read_bytes()).hexdigest()})
  pg.locator('#panel').screenshot(path=str(OUT/'native-panel-initial.png'))
  b.close()
except Exception as e:
 receipt.update({'result':'FAIL','error':str(e),'retry':'NOT_ATTEMPTED'})
 raise
finally:
 (OUT/'native-readback.json').write_text(json.dumps(receipt,indent=2)+'\n')
