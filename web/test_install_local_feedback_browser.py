#!/usr/bin/env python3
"""Browser acceptance for local .fwsc chooser feedback. No firmware/device writes."""
from __future__ import annotations
import json, os, re, sys, tempfile
from pathlib import Path
from playwright.sync_api import sync_playwright

src=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).with_name('x0x_installer.html')
mode=sys.argv[2] if len(sys.argv)>2 else 'after'
out=Path(os.environ.get('X0X_SCREEN_DIR',tempfile.mkdtemp(prefix='x0x-local-feedback-')))
out.mkdir(exist_ok=True)
s=src.read_text('utf8')
assert s.count('/*LIB*/')==1 and s.count('/*CATALOG*/')==1
s=s.replace('/*LIB*/', '''
function productOf(raw) { if(raw[0]!==88) throw Error('invalid fwsc'); return 'SMK-37 Pro_900'; }
function logicalImage(raw) { return raw; }
function sameKeyboard(){return true;}
function ctlProblem(){return null;}
class Updater { async find(){return null;} }
''')
s=s.replace('/*CATALOG*/',json.dumps({'repo':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro','latest':None,'versions':[],'history':[]}))
with sync_playwright() as p:
    browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-gpu'])
    try:
      for w in (320,390,1280):
        page=browser.new_page(viewport={'width':w,'height':840},device_scale_factor=1)
        errors=[]
        page.on('pageerror',lambda e:errors.append(str(e)))
        block=re.search(r'<script type="module">(.*?)</script>',s,re.S)
        assert block
        html_only=s[:block.start()]+s[block.end():]
        page.set_content(html_only,wait_until='load')
        page.evaluate('''
Object.defineProperty(window,'crypto',{configurable:true,value:{subtle:{digest:async()=>new Uint8Array(32).buffer}}});
Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map(),outputs:new Map()})});
''')
        page.evaluate('(code) => { new Function(code)(); }', block.group(1))
        page.wait_for_function('!document.querySelector("#localempty").hidden')
        assert page.locator('#go').is_hidden() and page.locator('#stockgo').is_disabled()
        page.locator('#chooseLocal').scroll_into_view_if_needed()
        with page.expect_file_chooser() as chooser: page.locator('#chooseLocal').click()
        chooser.value.set_files({'name':'bad.fwsc','mimeType':'application/octet-stream','buffer':b'INVALID_TEST_BYTES'})
        page.wait_for_function('document.querySelector("#status").textContent.includes("not a usable")')
        page.screenshot(path=str(out/f'before-invalid-{w}.png' if mode=='before' else out/f'after-invalid-{w}.png'),full_page=True)
        feedback=page.locator('#localfeedback')
        assert feedback.is_visible(), 'validation error must be visible beside local file chooser'
        assert 'could not be used' in feedback.inner_text()
        assert feedback.get_attribute('role')=='status' and feedback.get_attribute('aria-live')=='polite'
        assert feedback.get_attribute('data-state')=='error'
        coords=page.evaluate('''()=>{
const button=document.querySelector('#chooseLocal').getBoundingClientRect();
const box=document.querySelector('#localfeedback').getBoundingClientRect();
return {distance:box.top-button.bottom, size:parseFloat(getComputedStyle(document.querySelector('#localfeedback')).fontSize), width:document.documentElement.scrollWidth};}''')
        assert 0 <= coords['distance'] < 140,coords
        assert coords['size']>=13,coords
        assert coords['width']<=w, coords
        assert page.locator('#stockgo').is_disabled()
        page.locator('#lang').click()
        assert '使用できません' in feedback.inner_text()
        page.locator('#lang').click()
        assert 'could not be used' in feedback.inner_text()
        # A real button retry removes stale errors and transitions to in-place checking.
        page.evaluate('''()=>{const orig=File.prototype.arrayBuffer;File.prototype.arrayBuffer=function(){if(this.name==='slow.fwsc')return new Promise(resolve=>window.__finishSlow=()=>orig.call(this).then(resolve));return orig.call(this)}}''')
        with page.expect_file_chooser() as chooser: page.locator('#chooseLocal').click()
        chooser.value.set_files({'name':'slow.fwsc','mimeType':'application/octet-stream','buffer':b'INVALID_SLOW'})
        page.wait_for_function('document.querySelector("#localfeedback").textContent.includes("Checking")')
        assert feedback.get_attribute('data-state')=='pending'
        # A later selection is authoritative. The stale first result must never win.
        with page.expect_file_chooser() as chooser: page.locator('#chooseLocal').click()
        chooser.value.set_files({'name':'good.fwsc','mimeType':'application/octet-stream','buffer':b'XGOOD_FOR_MOCK'})
        page.wait_for_function('!document.querySelector("#stockconfirm").disabled')
        assert feedback.is_hidden(), 'validated file clears inline pending state'
        page.evaluate('window.__finishSlow()')
        page.wait_for_timeout(150)
        assert feedback.is_hidden(), 'obsolete file completion must not show stale error'
        assert page.locator('#stockgo').is_disabled(), 'confirmation remains required'
        assert not page.locator('#stockconfirm').is_checked()
        assert page.evaluate('document.activeElement.id')=='stockconfirm'
        assert not errors,errors
        print('PASS',w,'inline invalid feedback + JA/EN + no overflow + pending + stale rejection + valid confirmation')
        page.close()
    finally: browser.close()
