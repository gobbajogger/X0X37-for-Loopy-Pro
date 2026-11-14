"""Read-only browser exercise of the owner-supplied historical emulator.

No device connection, package modification, reset, save or destructive confirmation.
Native LCD and outgoing adapter events are observed; emulator code is unchanged.
"""
from pathlib import Path
import hashlib, json, os
from PIL import Image
from playwright.sync_api import sync_playwright

OUT = Path(os.environ.get('A4_EMULATOR_OUTPUT','/workspace/a4-inputs/emulator-observed'))
OUT.mkdir(parents=True,exist_ok=True)
URL = 'https://joakimeriksson.github.io/fm1-emu/smk37/?package=https%3A%2F%2Fgobbajogger.github.io%2Fx0x37-for-Loopy-Pro_nightly%2Fartifacts%2Fnightly-dev%2F5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a%2Fx0x37-elite-dev.fwsc&name=DnBOX+0.0.25.1+Elite&mhz=360&embed=1'
with sync_playwright() as w:
    b=w.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--autoplay-policy=no-user-gesture-required'])
    page=b.new_page(ignore_https_errors=True,viewport={'width':1400,'height':1000})
    page.add_init_script('''(()=>{
      const Original=window.AudioWorkletNode;
      window.__observed={events:[],ms:null,lcd:null};
      window.AudioWorkletNode=class extends Original {
        constructor(...args){super(...args);const post=this.port.postMessage.bind(this.port);
          this.port.postMessage=(m,...rest)=>{if(m.type==='key'||m.type==='enc')window.__observed.events.push({wall:performance.now(),...m});return post(m,...rest)};
          this.port.addEventListener('message',e=>{const m=e.data;if(m.type==='frame'){window.__observed.ms=m.ms;if(m.lcd)window.__observed.lcd=Array.from(m.lcd)}});this.port.start();
        }
      };
    })()''')
    assets=[]
    def response(r):
        if r.url.endswith(('.js','.wasm','.fwsc')):
            try:
                raw=r.body();assets.append({'url':r.url,'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'status':r.status})
            except Exception:pass
    page.on('response',response)
    page.goto(URL,wait_until='networkidle',timeout=45000)
    page.get_by_role('button',name='Start',exact=True).click()
    page.wait_for_timeout(8500)
    frames=[]
    def shot(name,action):
        page.wait_for_timeout(500)
        o=page.evaluate('window.__observed')
        lcd=o.pop('lcd')
        if lcd:
            im=Image.new('RGB',(240,240));im.putdata([(((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in lcd]);im.save(OUT/(name+'-native.png'))
        page.screenshot(path=str(OUT/(name+'-panel.png')),full_page=True)
        frames.append({'id':name,'action':action,'observation':o,'lcd_sha256':hashlib.sha256((OUT/(name+'-native.png')).read_bytes()).hexdigest() if lcd else None})
    def tap(x,y):
        page.mouse.move(x,y);page.mouse.down();page.wait_for_timeout(180);page.mouse.up()
    shot('00-boot','Start user gesture, await boot')
    tap(1266,392);shot('01-global','GLOBE physical pointer down180ms then release')
    page.mouse.move(171,408);page.mouse.down();page.wait_for_timeout(1400);shot('02-held-enter','Physical > held1400ms, not yet released');page.mouse.up();page.wait_for_timeout(200)
    tap(171,408);shot('03-fresh-increment','Release entry hold; fresh > down180ms then release')
    
    for _ in range(4):page.keyboard.press('ArrowUp',delay=100);page.wait_for_timeout(100)
    shot('04-select-next','Documented K2 adapter: four ArrowUp pulses,100ms down/100ms gap')
    
    for _ in range(4):page.keyboard.press('.',delay=100);page.wait_for_timeout(100)
    shot('05-algorithm','Documented K3 adapter: four period pulses,100ms down/100ms gap')
    tap(104,408);shot('06-back','Physical < down180ms then release')
    tap(1266,392);shot('07-exit','GLOBE down180ms then release')
    worklet=page.evaluate("async()=>{const r=await fetch('worklet.js');const a=await r.arrayBuffer();const h=await crypto.subtle.digest('SHA-256',a);return {status:r.status,bytes:a.byteLength,sha256:Array.from(new Uint8Array(h),x=>x.toString(16).padStart(2,'0')).join('')}}")
    receipt={'evidence':'HISTORICAL_REAL_FIRMWARE_EMULATOR','url':URL,'browser':b.version,'clock_MHz':360,'runtime_version':'Published runtime; no semantic version displayed. Exact JS/WASM/worklet digests identify observed version.','assets':assets,'worklet':worklet,'frames':frames,'limits':['Historical25.1 only; no proposed/current firmware execution','Observed LCD and adapter events; no audio/tail/timing qualification','First zero-duration click attempt did not change LCD; rerun used180ms holds','No device inputs or writes','Browser launch used ignore_https_errors because initial public-page load reported ERR_CERT_AUTHORITY_INVALID in this managed environment']}
    (OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'browser':b.version,'native_frames':sum(x['lcd_sha256'] is not None for x in frames),'adapter_events':len(frames[-1]['observation']['events']),'worklet':worklet}))
    b.close()
