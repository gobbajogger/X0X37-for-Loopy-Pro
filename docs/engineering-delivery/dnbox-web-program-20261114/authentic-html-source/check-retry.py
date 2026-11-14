"""Inject one local HTTP503 in original worklet fetch. No runtime bytes or permissions change."""
from pathlib import Path
from playwright.sync_api import sync_playwright
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
from threading import Thread
import json
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'evidence/delta';PACKET=Path('/workspace/website-v4-inputs/PR899-runtime-431d0db8-handoff')
receipt={'scope':'Unresolved boot/retry case only; previous9delta passes reused','worklet_requests':[]}
class FaultOnce(SimpleHTTPRequestHandler):
 def __init__(self,*a,**kw):super().__init__(*a,directory=str(ROOT/'site'),**kw)
 def log_message(self,*a):pass
 def do_GET(self):
  if self.path.split('?')[0]=='/firmware-runtime/worklet.js':
   receipt['worklet_requests'].append(self.path)
   if len(receipt['worklet_requests'])==1:
    self.send_response(503);self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(b'deliberate local transient503');return
  super().do_GET()
server=ThreadingHTTPServer(('127.0.0.1',8675),FaultOnce);Thread(target=server.serve_forever,daemon=True).start()
try:
 with sync_playwright() as p:
  b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True);pg=b.new_page()
  pg.route('**/*pro-dev.fwsc',lambda r:r.fulfill(path=str(PACKET/'runtime/fixture-pro.fwsc'),content_type='application/octet-stream',headers={'Access-Control-Allow-Origin':'*'}))
  pg.goto('http://127.0.0.1:8675/firmware-lesson/');pg.wait_for_function("()=>!document.querySelector('#boot').disabled")
  pg.locator('#boot').click();pg.wait_for_function("()=>!document.querySelector('#boot').disabled&&document.querySelector('#boot').textContent.startsWith('Retry')")
  assert not pg.locator('#boot').is_hidden()
  receipt['after_fault']={'button':pg.locator('#boot').inner_text(),'status':pg.locator('#runtime-state').inner_text(),'booted':pg.evaluate('firmwareLesson.state.booted')}
  assert receipt['after_fault']['booted'] is False
  pg.locator('#boot').click();pg.wait_for_function('()=>firmwareLesson.state.qualified',timeout=60000)
  receipt.update({'result':'PASS','browser':b.version,'after_retry_qualified':True});b.close()
except Exception as e:receipt.update({'result':'FAIL','error':str(e)});raise
finally:server.shutdown();server.server_close();(OUT/'retry-check.json').write_text(json.dumps(receipt,indent=2)+'\n')
