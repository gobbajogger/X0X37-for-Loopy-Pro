from pathlib import Path
from playwright.sync_api import sync_playwright
import json,hashlib
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'evidence/delta';OUT.mkdir(exist_ok=True)
PACKET=Path('/workspace/website-v4-inputs/PR899-runtime-431d0db8-handoff')
result={'scope':'Seven independently found changed-input defects; unchanged15browser+8geometry evidence reused','checks':[]}
def check(name,ok,**details):
 result['checks'].append({'name':name,'result':'PASS' if ok else 'FAIL',**details})
 if not ok:raise AssertionError(name)
def fixture(r):r.fulfill(path=str(PACKET/'runtime/fixture-pro.fwsc'),content_type='application/octet-stream',headers={'Access-Control-Allow-Origin':'*'})
try:
 with sync_playwright() as p:
  b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
  pg=b.new_page(viewport={'width':1440,'height':1000});pg.route('**/*pro-dev.fwsc',fixture)
  pg.goto('http://127.0.0.1:8674/firmware-lesson/');pg.wait_for_function("()=>!document.querySelector('#boot').disabled")
  pg.frame_locator('#native-runtime').locator('#start').click()
  pg.wait_for_function('()=>firmwareLesson.state.qualified',timeout=60000)
  check('Native-panel start hides stale parent Start CTA',pg.locator('#boot').is_hidden())
  def key(id,down):pg.frame_locator('#native-runtime').locator('body').evaluate("(el,m)=>document.dispatchEvent(new CustomEvent('dnbox:input',{detail:m}))",{'kind':'key','id':id,'down':down})
  key(8,True);pg.wait_for_timeout(180);key(8,False);pg.wait_for_function('()=>!firmwareLesson.state.qualified&&firmwareLesson.state.canSelectBD')
  check('Actual SD selection retains a safely enabled BD return',pg.locator('#select-bd').is_enabled())
  pg.locator('#select-bd').click();pg.wait_for_function('()=>firmwareLesson.state.qualified')
  check('Return helper confirms actual BD selection',pg.evaluate('firmwareLesson.state.canSelectBD&&firmwareLesson.state.qualified'))
  pg.locator('.steps a[href="#hear"]').click()
  check('Intentional phase navigation focuses actual changed heading',pg.evaluate("document.activeElement.id==='lesson-heading'&&document.activeElement.textContent==='Hear the space between.'"))
  pg.locator('.steps a[href="#keep"]').click();pg.go_back();pg.wait_for_timeout(250)
  check('Back restores phase and preserves real machine',pg.evaluate("location.hash==='#hear'&&firmwareLesson.state.booted&&document.querySelector('#lesson-heading').textContent==='Hear the space between.'"))
  pg.locator('.beat-actions [data-step="1"]').click();pg.wait_for_function('()=>firmwareLesson.state.bd[0]===true')
  pg.locator('#play').click();pg.wait_for_timeout(700);pg.locator('#save').click()
  pg.wait_for_function("()=>firmwareLesson.state.saveMessage==='SAVES WHEN STOPPED'",timeout=10000)
  check('Waiting save reply exposes exactly observed firmware text',pg.locator('#screen-state').inner_text().startswith('Firmware reports: SAVES WHEN STOPPED.'))
  pg.locator('#play').click();pg.wait_for_function("()=>firmwareLesson.state.saveMessage==='SAVED'",timeout=10000)
  check('Actual SAVED reply is accessible and distinct from browser backup','Firmware save confirmed' in pg.locator('#screen-state').inner_text())
  with pg.expect_download() as download_info:pg.locator('#backup').click()
  download=download_info.value;path=OUT/'browser-backup.fm1save';download.save_as(path)
  data=json.loads(path.read_text());check('Original browser-backup action returns readable local backup',data.get('version')==1,bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),keys=list(data))
  pg.screenshot(path=str(OUT/'desktop-corrected.png'),full_page=True)
  pg.set_viewport_size({'width':390,'height':844});pg.wait_for_timeout(250);pg.screenshot(path=str(OUT/'phone-corrected.png'),full_page=True)
  check('Changed backup/status/focus UI fits phone viewport',pg.evaluate('document.documentElement.scrollWidth<=innerWidth'))
  pg.close()
  # Worklet network failures are qualified separately by check-retry.py's real HTTP503 server.
  result['browser']=b.version;b.close();result['result']='PASS'
except Exception as e:result.update({'result':'FAIL','error':str(e)});raise
finally:(OUT/'checks.json').write_text(json.dumps(result,indent=2)+'\n')
