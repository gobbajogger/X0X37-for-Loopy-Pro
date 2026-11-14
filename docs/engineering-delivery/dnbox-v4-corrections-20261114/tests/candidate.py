from playwright.sync_api import sync_playwright
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'evidence';results=[]
routes=['home','listen','try','companion','files','files/project','files/slots','files/preset','files/preview/project','files/preview/slots','files/preview/preset','files/confirm/project','files/confirm/slots','files/confirm/preset','files/result/project','files/result/slots','files/result/preset','install','install/plan','install/outcome','learn','learn/setup','learn/view','learn/controls','learn/beat','learn/listen','learn/silent','learn/save','learn/next','learn/bass','learn/break','learn/movement','learn/arrangement','learn/versions','setups','setups/computer','setups/mobile','setups/gear','setups/smc','help','help/recover','demo','demo/kicks','demo/chicago','demo/miami','demo/break','sources','no-such-route']
with sync_playwright() as p:
 b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
 for w in [1440,390,320]:
  pg=b.new_page(viewport={'width':w,'height':900});errors=[];pg.on('pageerror',lambda e:errors.append(str(e)))
  for route in routes:
   pg.goto('http://127.0.0.1:4321/#'+route);pg.wait_for_timeout(40)
   data=pg.evaluate('''()=>({title:document.title,h1:document.querySelectorAll('h1').length,overflow:document.documentElement.scrollWidth>innerWidth,broken:[...document.images].filter(i=>!i.complete||!i.naturalWidth).map(i=>i.src),links:[...document.querySelectorAll('main a')].map(a=>a.getAttribute('href'))})''')
   data.update(route=route,width=w,errors=errors[:]);assert data['h1']==1 and not data['overflow'] and not data['broken'] and not errors,data;results.append(data)
   if route in ['home','listen','install','learn/save','learn/break','learn/movement','learn/arrangement','learn/versions']:
    pg.screenshot(path=str(OUT/(str(w)+'-'+route.replace('/','-')+'.png')),full_page=True)
  pg.close()
 pg=b.new_page();pg.goto('http://127.0.0.1:4321/#listen');playback=[]
 for i in range(pg.locator('audio').count()):
  a=pg.locator('audio').nth(i);a.evaluate('(a)=>a.play()');pg.wait_for_timeout(200);state=a.evaluate('(a)=>({src:a.currentSrc,time:a.currentTime,ready:a.readyState,error:a.error?.message,paused:a.paused,duration:a.duration})');assert state['time']>0 and not state.get('error'),state;playback.append(state)
  assert pg.locator('audio').evaluate_all('(aa)=>aa.filter(a=>!a.paused).length')==1
 pg.keyboard.press('Escape');assert pg.locator('audio').evaluate_all('(aa)=>aa.every(a=>a.paused)')
 a=pg.locator('audio').first;a.evaluate('(a)=>a.play()');old=a.element_handle();pg.locator('main a[href="#learn/next"]').click();assert old.evaluate('(a)=>a.paused')
 pg.emulate_media(reduced_motion='reduce');pg.goto('http://127.0.0.1:4321/#learn/beat');assert pg.locator('#teacher-play').is_disabled();pg.locator('#teacher-next').click()
 b.close()
(OUT/'candidate-route-checks.json').write_text(json.dumps(results,indent=2));(OUT/'candidate-audio-checks.json').write_text(json.dumps(playback,indent=2));print(len(results),'route/viewport PASS;',len(playback),'recording playback PASS; audio exclusivity/Escape/route-stop and reduced-motion PASS')
