"""Read-only native-layout references for selecting BD from a different drum row.
Uses normal published firmware controls; never writes a framebuffer or package.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright
from PIL import Image
import json,hashlib,time
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'evidence/row-family';OUT.mkdir(exist_ok=True)
PACKET=Path('/workspace/website-v4-inputs/PR899-runtime-431d0db8-handoff')
ROWS=[('BD',6),('SD',8),('LT',10),('MT',13),('HT',15),('RS',18),('CP',20),('CH',22),('OH',25),('CR',27),('RD',30)]
receipt={'runtime':'431d0db82cbadca761fb32c6548ca4765875324f','firmware':'79280e763341d03ba43266d3e25dad66f12de860','model':'Pro','scope':'Actual row-label/layout signatures only','files':[],'events':[]}
with sync_playwright() as p:
 b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
 pg=b.new_page();pg.route('**/*pro-dev.fwsc',lambda r:r.fulfill(path=str(PACKET/'runtime/fixture-pro.fwsc'),content_type='application/octet-stream',headers={'Access-Control-Allow-Origin':'*'}))
 pg.add_init_script("document.addEventListener('dnbox:booted',e=>window.actualBoot=e.detail)")
 pg.goto('http://127.0.0.1:8674/firmware-runtime/?fw=dnbox-pro-0251');pg.wait_for_function("()=>document.documentElement.dataset.ready==='true'");pg.locator('#start').click();pg.wait_for_function('()=>window.actualBoot?.board===1',timeout=60000);pg.wait_for_timeout(16000)
 for name,key in ROWS:
  # Source79280e76 black-key drum selection. A short paired press selects, not edits.
  for down in [True,False]:
   event={'kind':'key','id':key,'down':down};pg.evaluate("m=>document.dispatchEvent(new CustomEvent('dnbox:input',{detail:m}))",event);receipt['events'].append(event);pg.wait_for_timeout(180)
  pg.wait_for_timeout(400)
  data=bytes(pg.evaluate("()=>{const v=document.querySelector('#panel').getContext('2d').getImageData(262,44,480,480).data,a=[];for(let y=0;y<240;y++)for(let x=0;x<240;x++)a.push(...v.slice((2*y*480+2*x)*4,(2*y*480+2*x)*4+4));return a}"))
  path=OUT/(name+'.rgba');path.write_bytes(data);Image.frombytes('RGBA',(240,240),data).save(OUT/(name+'.png'))
  receipt['files'].append({'row':name,'physical_key_id':key,'file':path.name,'sha256':hashlib.sha256(data).hexdigest()})
 receipt['browser']=b.version;b.close()
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
