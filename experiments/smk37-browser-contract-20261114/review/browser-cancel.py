"""Independent focused Chromium cancellation regression; fake endpoint only."""
import hashlib
import http.server
import json
import pathlib
import threading
from playwright.sync_api import sync_playwright

root = pathlib.Path(__file__).resolve().parents[1]
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(root), **kwargs)
    def end_headers(self):
        self.send_header('Permissions-Policy', 'midi=()')
        super().end_headers()
    def log_message(self, *args):
        pass

paths = ['transport.mjs', 'fixture.mjs', 'index.html', 'review/browser-cancel.py']
before = {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}
server = http.server.ThreadingHTTPServer(('127.0.0.1',0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
receipt = {'before':before, 'browser':'system Chromium', 'scope':'fake endpoint pointer cancellation only'}
try:
    with sync_playwright() as p:
        browser = p.chromium.launch(executable_path='/usr/bin/chromium', headless=True, args=['--no-sandbox'])
        page = browser.new_page()
        page.add_init_script('window.midiPermissionRequests=0; navigator.requestMIDIAccess=()=>{window.midiPermissionRequests++;throw Error("Forbidden in fake fixture");};')
        page.goto(f'http://127.0.0.1:{server.server_port}/index.html')
        page.locator('#connect').click()
        page.locator('#arm').click()
        page.locator('#pedal').click()
        page.locator('#bend').click()
        page.locator('[data-rt="start"]').click()
        page.locator('#note').hover()
        page.mouse.down()
        page.locator('#note').dispatch_event('pointercancel', {'pointerId':1, 'button':0})
        state = page.evaluate('({status:fixture.controller.status,sent:fixture.endpoint.sent,permissions:window.midiPermissionRequests})')
        receipt['observed'] = state
        assert state['status']['armed'] is False, 'pointer cancel must disarm'
        assert state['status']['held'] == []
        assert state['sent'][-4:] == [[0x81,60,0],[0xB1,64,0],[0xE1,0,64],[0xFC]], 'pointer cancel must release all local obligations'
        assert state['permissions'] == 0
        page.mouse.up()
        browser.close()
        receipt['result'] = 'PASS'
except Exception as error:
    receipt['result'] = 'FAIL'
    receipt['error'] = str(error)
finally:
    server.shutdown()
    receipt['after'] = {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}
    receipt['stable'] = receipt['before'] == receipt['after']
    print(json.dumps(receipt, indent=2))
    raise SystemExit(0 if receipt.get('result')=='PASS' and receipt['stable'] else 1)
