#!/usr/bin/env python3
"""Browser-only Web MIDI capability UX. No hardware or firmware flash."""
from pathlib import Path
from playwright.sync_api import sync_playwright
import sys, re, json
source=Path(sys.argv[1]).read_text()
out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
assert source.count('/*LIB*/')==source.count('/*CATALOG*/')==1
source=source.replace('/*LIB*/','''
function productOf(){throw Error('unapproved');}
function logicalImage(raw){return raw;}
function sameKeyboard(){return false;}
class Updater { async find(){return null;} }
''').replace('/*CATALOG*/',json.dumps({'repo':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro','latest':None,'versions':[],'history':[]}))
m=re.search(r'<script type="module">(.*?)</script>',source,re.S)
assert m
script=m.group(1)
html=source[:m.start()]+source[m.end():]
with sync_playwright() as p:
  browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-gpu'])
  try:
    for width in (320,390,1280):
      for supported in (False,True):
        page=browser.new_page(viewport={'width':width,'height':820},device_scale_factor=1)
        errors=[]
        page.on('pageerror',lambda e: errors.append(str(e)))
        page.set_content(html,wait_until='load')
        page.evaluate('''(supported)=>{
          Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:supported ? async()=>({inputs:new Map(),outputs:new Map()}) : undefined});
        }''',supported)
        page.evaluate('(code)=>new Function(code)()',script)
        page.wait_for_function('!document.querySelector("#localempty").hidden')
        # Non-approved remote firmware must never become installable.
        assert page.locator('#go').is_hidden() and page.locator('#stockgo').is_disabled()
        assert page.locator('#localempty').is_visible() and not page.locator('#stockconfirm').is_checked()
        note=page.locator('#browserhelp')
        if not supported:
          assert note.is_visible(),'unsupported browser must explain the problem before Connect'
          assert 'Web MIDI' in note.inner_text() and 'Chrome or Edge' in note.inner_text()
          assert page.locator('#conn').inner_text()=='Web MIDI unavailable'
          assert page.locator('#connbtn').is_disabled()
          assert page.locator('#connbtn').get_attribute('aria-disabled')=='true'
          assert not page.locator('#connDetail').is_visible()
          # Click must not invoke device search in an unsupported browser.
          page.locator('#connbtn').evaluate('(e)=>e.click()')
          assert page.locator('#conn').inner_text()=='Web MIDI unavailable'
          if width==390:page.screenshot(path=str(out/'unsupported-en-390.png'),full_page=True)
          page.locator('#lang').focus()
          page.locator('#lang').click()
          assert page.locator('#browserhelp').is_visible()
          assert 'Chrome または Edge' in page.locator('#browserhelp').inner_text()
          assert page.locator('#conn').inner_text()=='Web MIDI を使用できません'
          assert page.locator('#connbtn').inner_text()=='接続不可'
          assert page.locator('#lang').evaluate('(e)=>document.activeElement===e')
          if width==390:page.screenshot(path=str(out/'unsupported-ja-390.png'),full_page=True)
        else:
          assert not note.is_visible(),'supported browser must not show unsupported warning'
          assert page.locator('#connbtn').is_enabled()
          assert page.locator('#connbtn').get_attribute('aria-disabled')=='false'
          assert page.locator('#conn').inner_text()=='Not connected'
          if width==390:page.screenshot(path=str(out/'supported-en-390.png'),full_page=True)
        assert page.evaluate('document.documentElement.scrollWidth')<=width,('horizontal overflow',width,supported)
        assert not errors,errors
        print('PASS',width,'SUPPORTED' if supported else 'UNSUPPORTED','EN/JA/help/focus/empty-picker/no-overflow')
        page.close()
  finally:
    browser.close()
