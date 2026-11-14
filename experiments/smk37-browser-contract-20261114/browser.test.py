#!/usr/bin/env python3
"""Real Chromium, synthetic endpoint only; blocks Web MIDI requests."""
import http.server, json, pathlib, threading, importlib.util, time
from playwright.sync_api import sync_playwright

ROOT=pathlib.Path(__file__).parent
spec=importlib.util.spec_from_file_location('fixture_server',ROOT/'serve.py')
server_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(server_module)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),server_module.Handler)
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
origin=f'http://127.0.0.1:{server.server_port}'
results=[]
def check(name,condition):
    assert condition,name
    results.append({'case':name,'result':'PASS'})

try:
 with sync_playwright() as p:
    browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox'])
    context=browser.new_context(viewport={'width':1280,'height':900})
    context.add_init_script("""window.midiRequests=0;Object.defineProperty(navigator,'requestMIDIAccess',{value:()=>{window.midiRequests++;throw new Error('Web MIDI access forbidden by acceptance fixture');}});""")
    page=context.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    page.route('**/*',lambda route:route.continue_() if route.request.url.startswith(origin+'/') else route.abort())
    response=page.goto(origin+'/');page.wait_for_function('!!window.fixture')
    check('permission blocked by response policy',response.headers.get('permissions-policy')=='midi=()')
    status=lambda:page.evaluate('fixture.controller.status')
    sent=lambda:page.evaluate('fixture.endpoint?.sent ?? []')
    check('initial disconnected status and zero MIDI requests',status()['state']=='disconnected' and page.evaluate('midiRequests')==0)
    page.click('#connect');check('connected fake and explicit arm required',status()['state']=='connected-disarmed' and not sent())
    page.click('#mod');check('disarmed send rejects visibly with no bytes','explicit arm required' in page.locator('#result').inner_text() and not sent())
    page.click('#arm')
    box=page.locator('#note').bounding_box();page.mouse.move(box['x']+10,box['y']+10);page.mouse.down()
    check('real pointer down encodes note',sent()==[[0x91,60,100]])
    page.mouse.move(1200,800);page.mouse.up();check('outside pointer release uses capture',sent()[-1]==[0x81,60,0] and not status()['held'])
    page.locator('#note').focus();page.keyboard.down('Space');page.keyboard.up('Space')
    check('keyboard note has matching on/off',sent()[-2:]==[[0x91,60,100],[0x81,60,0]])
    page.evaluate("fixture.controller.noteOn(62);document.querySelector('#channel').value='3';document.querySelector('#channel').dispatchEvent(new Event('change'))")
    check('channel change releases original channel and disarms',sent()[-1]==[0x81,62,0] and status()['channel']==3 and not status()['armed'])
    page.click('#arm');page.click('#mod');page.click('#pedal');page.click('#bend')
    check('actual control buttons emit selected-channel bytes',sent()[-3:]==[[0xB2,1,75],[0xB2,64,127],[0xE2,127,127]])
    page.click('#incoming');n=len(sent());check('incoming is observation only',len(status()['feedback'])==1 and len(sent())==n)
    for selector in ['#unsupported','#seek','#sysex']:
      page.click(selector);check(selector+' rejection has zero transmit','unsupported' in page.locator('#result').inner_text() and len(sent())==n)
    page.click('#disarm');check('release resets used sustain and bend and clears observations',sent()[-2:]==[[0xB2,64,0],[0xE2,0,64]] and not status()['feedback'])
    page.click('#arm');page.evaluate('fixture.controller.noteOn(65)');page.evaluate("window.dispatchEvent(new Event('blur'))")
    check('focus loss releases and disarms',sent()[-1]==[0x82,65,0] and not status()['armed'])
    page.click('#connect');page.click('#arm');page.evaluate('fixture.controller.noteOn(66)');page.evaluate("window.dispatchEvent(new Event('pagehide'))")
    check('pagehide remains bound after reconnect',sent()[-1]==[0x82,66,0] and not status()['armed'])
    page.click('#arm');page.evaluate('fixture.controller.noteOn(67)');page.click('#lose')
    check('abrupt loss shows disconnected and unconfirmed releases',status()['state']=='disconnected' and status()['releaseUnconfirmed'])
    page.click('#connect');page.click('#arm');check('replacement never replays stale obligations',not sent() and status()['releaseUnconfirmed'])
    page.click('#failure');page.click('#mod');check('ambiguous enqueue faults and disarms',status()['state']=='fault' and not status()['armed'] and status()['releaseUnconfirmed'])
    page.click('#connect');page.click('#arm');page.evaluate('fixture.controller.noteOn(68)');page.keyboard.press('Escape')
    check('Escape releases fake note',sent()[-1]==[0x82,68,0] and not status()['armed'])
    page.click('#arm');page.evaluate('fixture.controller.noteOn(69)');page.evaluate("document.querySelector('#note').dispatchEvent(new PointerEvent('pointercancel',{pointerId:2}))")
    check('pointer cancellation releases and disarms',sent()[-1]==[0x82,69,0] and not status()['held'] and not status()['armed'])
    page.click('#disarm')
    for width in [390,1280]:
      page.set_viewport_size({'width':width,'height':900})
      check(f'{width}px fixture is readable without horizontal page overflow',page.evaluate('document.documentElement.scrollWidth<=window.innerWidth'))
    check('no Web MIDI permission request throughout',page.evaluate('midiRequests')==0)
    check('no browser script errors',not errors)
    print(json.dumps({'browser':browser.version,'cases':results,'pass':len(results),'fail':0,'physical_midi':'NOT_RUN','permission_requests':0},indent=2))
    browser.close()
finally:
 server.shutdown();server.server_close()
