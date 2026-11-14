from pathlib import Path
from playwright.sync_api import sync_playwright
from PIL import Image,ImageChops
import json,time,hashlib
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'evidence';PACKET=Path('/workspace/website-v4-inputs/PR899-runtime-431d0db8-handoff')
result={'scope':'Changed HTML teaching renderer and actual controls only','checks':[],'runtime':'431d0db82cbadca761fb32c6548ca4765875324f'}
def check(name,passed,**details):
 result['checks'].append({'name':name,'result':'PASS' if passed else 'FAIL',**details})
 if not passed:raise AssertionError(name)
try:
 with sync_playwright() as p:
  b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
  pg=b.new_page(viewport={'width':1440,'height':1000});errors=[];pg.on('pageerror',lambda e:errors.append(str(e)))
  pg.route('**/*pro-dev.fwsc',lambda r:r.fulfill(path=str(PACKET/'runtime/fixture-pro.fwsc'),content_type='application/octet-stream',headers={'Access-Control-Allow-Origin':'*'}))
  pg.goto('http://127.0.0.1:8674/firmware-lesson/')
  pg.wait_for_function("()=>!document.querySelector('#boot').disabled")
  check('No control input before explicit sound/boot gesture',pg.evaluate('firmwareLesson.state.events.length===0'))
  pg.locator('#boot').click();pg.wait_for_function('()=>firmwareLesson.state.qualified',timeout=60000)
  check('Actual returned 909 BD Pattern1 page1 recognized after boot',pg.evaluate('firmwareLesson.state.qualified'))
  def pixel_check(label):
   # Pause only the website observer for one actual observed-frame comparison.
   # The emulator keeps running. Native logical scale avoids display interpolation.
   prior=pg.evaluate("document.querySelector('#lcd').style.transform")
   pg.evaluate("document.querySelector('#lcd').style.transform='scale(1)'")
   rgba=bytes(pg.evaluate('firmwareLesson.snapshot()'));Image.frombytes('RGBA',(240,240),rgba).save(OUT/(label+'-native.png'))
   pg.locator('#lcd').screenshot(path=str(OUT/(label+'-html.png')))
   actual=Image.open(OUT/(label+'-html.png')).convert('RGBA');expected=Image.frombytes('RGBA',(240,240),rgba)
   diff=ImageChops.difference(actual,expected);bad=sum(a!=c for a,c in zip(actual.getdata(),expected.getdata()))
   check(label+' HTML pixels equal native framebuffer',actual.size==(240,240) and bad==0,different_pixels=bad)
   pg.evaluate("value=>document.querySelector('#lcd').style.transform=value",prior)
   pg.evaluate('firmwareLesson.resume()')
  pixel_check('initial')
  for step in [1,5,9,13]:
   pg.locator(f'.beat-actions [data-step="{step}"]').click()
   try:pg.wait_for_function(f'()=>firmwareLesson.state.bd[{step-1}]===true',timeout=10000)
   except Exception:
    (OUT/'step-failure-state.json').write_text(json.dumps(pg.evaluate('firmwareLesson.state'),indent=2))
    Image.frombytes('RGBA',(240,240),bytes(pg.evaluate('Array.from(firmwareLesson.frame)'))).save(OUT/'step-failure-native.png')
    pg.screenshot(path=str(OUT/'step-failure-page.png'),full_page=True)
    raise
   pg.wait_for_timeout(250)
  check('Actual native four-kick result',pg.evaluate('JSON.stringify(firmwareLesson.state.bd.map((x,i)=>x?i+1:null).filter(Boolean))==="[1,5,9,13]"'))
  n=pg.evaluate('firmwareLesson.state.events.length');pg.locator('.beat-actions [data-step="1"]').click();pg.wait_for_timeout(300)
  check('Placing an existing hit never erases it or emits a toggle',pg.evaluate(f'firmwareLesson.state.events.length==={n}&&firmwareLesson.state.bd[0]===true'))
  pixel_check('four-kicks')
  pg.screenshot(path=str(OUT/'desktop-html-lesson.png'),full_page=True)
  pg.locator('#play').click();pg.wait_for_timeout(1800)
  hashes=[]
  for i in range(4):
   hashes.append(hashlib.sha256(bytes(pg.evaluate('Array.from(firmwareLesson.frame)'))).hexdigest());pg.wait_for_timeout(250)
  check('Actual firmware transport produces advancing display',len(set(hashes))>1,hashes=hashes)
  pg.locator('#play').click();pg.wait_for_timeout(600)
  pixel_check('stopped')
  # Same live session across phase changes.
  before=pg.frame_locator('#native-runtime').locator('#panel').count();pg.locator('.steps a[href="#keep"]').click()
  check('Lesson route preserves the real machine and actual hits',before==1 and pg.evaluate('firmwareLesson.state.bd[0]===true'))
  pg.set_viewport_size({'width':390,'height':844});pg.wait_for_timeout(400)
  check('Phone has no horizontal overflow',pg.evaluate('document.documentElement.scrollWidth<=innerWidth'))
  pg.screenshot(path=str(OUT/'phone-html-lesson.png'),full_page=True)
  # A part change must remove all stale semantic grid labels.
  detents=0
  for i in range(8):
   pg.locator('#encoders .encoder').nth(2).locator('button').nth(1).click();detents+=1;pg.wait_for_timeout(650)
   if pg.evaluate('!firmwareLesson.state.qualified'):break
  check('Different actual part removes909semantic labels',pg.evaluate("!firmwareLesson.state.qualified&&!document.querySelector('#lcd [data-step]')"),single_detents_sent=detents)
  check('Unknown-layout placement actions fail closed',pg.locator('.beat-actions button:enabled').count()==0)
  pixel_check('unclassified')
  pg.emulate_media(reduced_motion='reduce')
  check('Reduced motion suppresses decorative response animation',pg.evaluate("getComputedStyle(document.querySelector('#response-marker')).animationName==='none'"))
  check('No browser errors',not errors,errors=errors)
  result['render_times_ms']=pg.evaluate('firmwareLesson.state.renderTimes')
  result['events']=pg.evaluate('firmwareLesson.state.events');result['errors']=pg.evaluate('firmwareLesson.state.errors')
  result['browser']=b.version;b.close();result['result']='PASS'
except Exception as e:
 result.update({'result':'FAIL','error':str(e)});raise
finally:
 (OUT/'html-checks.json').write_text(json.dumps(result,indent=2)+'\n')
