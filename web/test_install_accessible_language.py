#!/usr/bin/env python3
"""X0X installer EN/JA accessible-name regression (browser mock; never flashes)."""
import json
import re
import sys
from pathlib import Path
from playwright.sync_api import sync_playwright

SOURCE=Path(sys.argv[1]).resolve()
OUT=Path(sys.argv[2]).resolve()
OUT.mkdir(parents=True,exist_ok=True)
s=SOURCE.read_text()
assert s.count('/*LIB*/')==s.count('/*CATALOG*/')==1
s=s.replace('/*LIB*/','''
function productOf(raw) {if (raw[0] !== 88) throw Error('invalid fwsc'); return 'SMK-37 Pro_900';}
function logicalImage(raw) {return raw;}
function sameKeyboard(){return true;}
class Updater {async find(test){return window.__fakeFind(test);}}
''')
s=s.replace('/*CATALOG*/',json.dumps({'repo':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro','latest':None,'versions':[],'history':[]}))
m=re.search(r'<script type="module">(.*?)</script>',s,re.S)
assert m
html=s[:m.start()]+s[m.end():]
script=m.group(1)
expected={
    '.home':('X0X37 home','X0X37 ホーム'),
    '.head-nav':('Site navigation','サイトナビゲーション'),
    '.journey':('Install workflow','インストール手順'),
    '#connectbar':('Keyboard connection','キーボード接続'),
    '#version':('Firmware version','ファームウェアのバージョン'),
    '#models':('Keyboard model','キーボードの機種'),
    '#bar':('Firmware installation progress','ファームウェアのインストール進行状況'),
    '.install-sidebar':('Installation guidance','インストール前の案内'),
    '.resource-links':('Other resources','関連リンク'),
}
with sync_playwright() as p:
    browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-gpu'])
    try:
        for w in (320,390,1280):
            page=browser.new_page(viewport={'width':w,'height':820},device_scale_factor=1)
            errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.set_content(html,wait_until='load')
            page.evaluate('''() => {
              window.__fakeFind=async()=>null;
              Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map(),outputs:new Map(),onstatechange:null})});
            }''')
            page.evaluate('(code) => new Function(code)()',script)
            page.wait_for_function('!document.querySelector("#localempty").hidden')
            assert page.locator('#go').is_hidden() and page.locator('#stockgo').is_disabled(), 'no downloadable firmware must remain nonflashable'
            assert page.locator('#localempty').is_visible(), 'local-file CTA remains visible with empty picker'
            assert not page.locator('#stockconfirm').is_checked()
            langbtn=page.locator('#lang')
            for cycle in range(3):
                idx=0 if cycle in (0,2) else 1
                assert page.evaluate('document.documentElement.lang')==('en' if idx==0 else 'ja')
                for selector,pair in expected.items():
                    observed=page.locator(selector).get_attribute('aria-label')
                    assert observed==pair[idx], (w,cycle,selector,observed,pair[idx])
                if cycle<2:
                    langbtn.focus()
                    langbtn.click()
                    assert langbtn.evaluate('(el) => document.activeElement === el'),'language switch must retain keyboard focus'
            # Wrong model and unsafe upload: never enable install through translation.
            with page.expect_file_chooser() as fc:page.locator('#chooseLocal').click()
            fc.value.set_files({'name':'bad.fwsc','mimeType':'application/octet-stream','buffer':b'WRONG_MODEL'})
            page.wait_for_function('document.querySelector("#localfeedback").dataset.state==="error"')
            assert page.locator('#stockgo').is_disabled() and page.locator('#go').is_hidden()
            assert not page.locator('#stockconfirm').is_checked()
            assert page.evaluate('document.documentElement.scrollWidth')<=w, ('horizontal overflow',w)
            assert not errors, errors
            page.screenshot(path=str(OUT/f'accessibility-{w}.png'),full_page=True)
            print('PASS',w,'9 English/Japanese accessible region/control labels, language focus, empty picker, model guard, no overflow')
            page.close()
    finally:
        browser.close()
