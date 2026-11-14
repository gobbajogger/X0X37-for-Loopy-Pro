from playwright.sync_api import sync_playwright
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1];out=[]
def record(name,**kw):out.append(dict(check=name,**kw));print(json.dumps(out[-1]),flush=True)
with sync_playwright() as p:
 b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
 for w in [1440,390,320]:
  pg=b.new_page(viewport={'width':w,'height':900});errors=[];pg.on('pageerror',lambda e:errors.append(str(e)))
  pg.goto('http://127.0.0.1:4321/#home');pg.keyboard.press('Tab');assert pg.locator('.skip').evaluate('(e)=>e===document.activeElement');record('initial first Tab is skip',width=w)
  pg.keyboard.press('Enter');assert pg.evaluate('document.activeElement.id')=='main'
  pg.locator('.path-list a[href="#companion"]').first.scroll_into_view_if_needed();before=pg.evaluate('scrollY');pg.locator('.path-list a[href="#companion"]').first.click();pg.wait_for_timeout(100);assert pg.evaluate('document.activeElement.tagName')=='H1';assert pg.evaluate('scrollY')==0
  pg.evaluate('scrollTo(0,300)');pg.wait_for_timeout(100);forward=pg.evaluate('scrollY');pg.go_back();pg.wait_for_timeout(150);back=pg.evaluate('scrollY');assert abs(back-before)<2,(w,before,back);record('Back restores home position',width=w,expected=before,actual=back,focus=pg.evaluate('document.activeElement.outerHTML.slice(0,150)'))
  pg.go_forward();pg.wait_for_timeout(150);assert abs(pg.evaluate('scrollY')-forward)<2;record('Forward restores companion',width=w,expected=forward,actual=pg.evaluate('scrollY'))
  # Two visits to home must retain distinct history positions.
  pg.locator('footer a[href="#home"]').click();pg.wait_for_timeout(80);pg.evaluate('scrollTo(0,800)');pg.wait_for_timeout(100)
  pg.evaluate("document.querySelector('main a[href=\"#try\"]').click()");pg.wait_for_timeout(80);pg.go_back();pg.wait_for_timeout(100);assert abs(pg.evaluate('scrollY')-800)<2
  pg.go_back();pg.wait_for_timeout(100);pg.go_back();pg.wait_for_timeout(100);assert abs(pg.evaluate('scrollY')-before)<2;record('repeated home visits have independent positions',width=w)
  pg.goto('http://127.0.0.1:4321/#install');assert 'may continue installing after its sample backup fails' in pg.locator('main').inner_text();record('install backup-failure warning',width=w)
  if w<760:
   pg.locator('#menu').click();pg.keyboard.press('Escape');assert pg.evaluate('document.activeElement.id')=='menu';record('menu Escape retains focus',width=w)
  for route in ['learn/next','learn/bass','learn/break','learn/movement','learn/arrangement','learn/versions','learn/save']:
   pg.goto('http://127.0.0.1:4321/#'+route);assert pg.locator('h1').count()==1;assert pg.evaluate('document.documentElement.scrollWidth<=innerWidth'),(w,route);record('journey fits',width=w,route=route)
  assert not errors,errors;pg.close()
 b.close()
(ROOT/'evidence'/'after-navigation.json').write_text(json.dumps(out,indent=2))
