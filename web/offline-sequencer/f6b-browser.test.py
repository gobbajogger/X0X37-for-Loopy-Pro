# SPDX-License-Identifier: GPL-3.0-only
# Run on the Orchestrator's box, never in the Codex source-writing session.
from pathlib import Path
from urllib.parse import urlparse, unquote
from playwright.sync_api import sync_playwright
import os, tempfile, mimetypes, json, struct, zlib

ROOT = Path(__file__).resolve().parent
OUT = Path(os.environ.get('DNBOX_EDITOR_TEST_OUTPUT_DIR', str(Path(tempfile.gettempdir()) / 'dnbox-f6b')))

def project_fixture(pitch, opaque):
    raw = bytearray(36452)
    struct.pack_into('<8I', raw, 0, 0x50583058, 1, 36420, 0, 1, 13, 13, 0)
    struct.pack_into('<II', raw, 32, 0x50305830, 1)
    raw[50:52] = bytes([2, 13])
    for voice, kind in enumerate([1, 0]):
        cell, tag = 840 + voice * 32, 2760 + voice * 12
        raw[cell + 26:cell + 32] = bytes([79, 83, 2, kind, 0, 0])
        struct.pack_into('<IHBB', raw, tag, 0x4f4d4931 if kind else 0x4f434831, 2, 18 if kind else 22, 1)
    for pattern in range(32):
        at = 2784 + pattern * 656
        raw[at + 96] = raw[at + 196] = 16
        for part in [200, 408]:
            raw[at + part + 192] = 16
            raw[at + part + 195] = 24
            raw[at + part + 199] = 1
    raw[2984:2987] = bytes([pitch, 1, 90])
    raw[36451] = opaque
    struct.pack_into('<I', raw, 12, zlib.crc32(raw[32:]))
    return bytes(raw)

def run():
    OUT.mkdir(parents=True, exist_ok=True)
    receipts = []
    with sync_playwright() as p:
        opts = {'executable_path': os.environ['DNBOX_CHROMIUM_EXECUTABLE']} if os.environ.get('DNBOX_CHROMIUM_EXECUTABLE') else {}
        browser = p.chromium.launch(headless=True, **opts)
        for width, height in [(1280, 900), (390, 844), (844, 390)]:
            ctx = browser.new_context(viewport={'width': width, 'height': height})
            ctx.add_init_script('window.midiCalls=0;navigator.requestMIDIAccess=()=>{window.midiCalls++;throw Error("No device calls expected")};')
            page = ctx.new_page()
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            def serve(route):
                uri = urlparse(route.request.url)
                path = (ROOT / unquote(uri.path).lstrip('/')).resolve()
                if uri.path in ['/x0x_projects.js', '/x0x_breaks.js', '/device_session.js']:  # x0x_breaks.js imports ./device_session.js
                    path = ROOT.parent / uri.path.lstrip('/')
                    route.fulfill(body=path.read_bytes(), content_type='text/javascript')
                    return
                if uri.netloc != 'localhost' or not path.is_relative_to(ROOT) or not path.is_file():
                    route.abort()
                    return
                route.fulfill(body=path.read_bytes(), content_type=mimetypes.guess_type(str(path))[0] or 'application/octet-stream')
            page.route('**/*', serve)
            page.goto('http://localhost/index.html')
            page.wait_for_function('window.dnboxEditor')
            rev = lambda: page.evaluate('dnboxEditor.revision()')
            snap = lambda: page.evaluate('dnboxEditor.snapshot()')
            vp = page.locator('#viewport')
            vp.focus()
            vp.press('Enter')
            vp.press('ArrowRight')
            vp.press('ArrowRight')
            vp.press('Enter')
            assert len(page.evaluate('dnboxEditor.notes()')) == 2
            # Real marquee selects intersected notes; cancellation commits nothing.
            page.locator('button[data-tool="group"]').click()
            vp.evaluate('(v)=>{v.scrollLeft=0;v.scrollTop=32+(127-60)*28-100}')
            rect, top = vp.bounding_box(), vp.evaluate('(v)=>v.scrollTop')
            x, y = rect['x'] + 54, rect['y'] + 32 + (127-62)*28 - top + 4
            before, revision = snap(), rev()
            page.mouse.move(x, y); page.mouse.down()
            page.mouse.move(x + 120, y + 112, steps=5); page.mouse.up()
            assert '2 notes selected' in page.locator('#inspector').inner_text()
            page.mouse.move(x, y); page.mouse.down(); page.mouse.move(x + 60, y + 60, steps=3)
            vp.dispatch_event('pointercancel', {'pointerId': 1}); page.mouse.up()
            assert snap() == before and rev() == revision
            vp.focus()
            vp.press('ArrowLeft'); vp.press('ArrowLeft')
            vp.press('Alt+ArrowUp'); vp.press('Shift+ArrowRight'); vp.press('Delete')
            assert snap() == before and rev() == revision
            before, revision = snap(), rev()
            page.locator('#group-actions').click()
            page.locator('#select-all-notes').click()
            assert '2 selected' in page.locator('#group-count').inner_text()
            page.locator('#group-velocity').fill('79')
            page.locator('#group-set-velocity').click()
            assert rev() == revision  # preview is read-only
            page.locator('[data-group-apply]').click()
            assert rev() == revision + 1
            assert [n['velocity'] for n in page.evaluate('dnboxEditor.notes()')] == [79, 79]
            page.locator('#undo').click()
            assert snap() == before
            page.locator('#redo').click()
            assert [n['velocity'] for n in page.evaluate('dnboxEditor.notes()')] == [79, 79]
            # Leaving Group removes stale group state without changing music.
            page.locator('button[data-tool="group"]').click()
            page.locator('#group-actions').click()
            page.locator('#select-all-notes').click()
            page.locator('#close-dialog').click()
            page.locator('button[data-tool="select"]').click()
            assert 'notes selected' not in page.locator('#inspector').inner_text()
            # Context changes clear the old group; ghosts cannot become edit targets.
            page.locator('[data-part="1"]').click()
            page.locator('#ghost-toggle').check()
            assert page.locator('.ghost-note').count() == 2
            assert page.locator('.ghost-note[data-note]').count() == 0
            before, revision = snap(), rev()
            page.locator('#keyboard-toggle').click()
            page.locator('[data-key-pitch="60"]').click()
            assert snap() == before and rev() == revision
            page.locator('#keyboard-insert').click()
            assert rev() == revision + 1
            # Group tool ignores the single-note velocity editing gesture.
            page.locator('button[data-tool="group"]').click()
            before, revision = snap(), rev()
            assert page.locator('[data-velocity]').count() == 1
            page.locator('[data-velocity]').first.dispatch_event('pointerdown', {'pointerId': 9, 'clientY': 100})
            page.locator('#velocity').dispatch_event('pointermove', {'pointerId': 9, 'clientY': 20})
            page.locator('#velocity').dispatch_event('pointerup', {'pointerId': 9})
            assert snap() == before and rev() == revision
            # View switches never change music, and controls remain reachable.
            page.locator('#view-sound').click()
            assert page.locator('#sound').is_visible()
            assert not page.locator('.music').is_visible()
            page.locator('#view-editor').click()
            assert vp.is_visible()
            assert snap() == before
            page.locator('#library').click()
            page.get_by_role('heading', name='Community library').wait_for()
            page.locator('#close-dialog').click()
            # A late older File.text must not replace a newer library workflow.
            page.evaluate('''() => {
              const original=File.prototype.text;
              File.prototype.text=function(){
                if(this.name==='slow.json')return new Promise(resolve=>{window.releaseSlowDraft=()=>resolve(JSON.stringify(dnboxEditor.snapshot()));});
                return original.call(this);
              };
            }''')
            page.locator('#open-draft').set_input_files({'name':'slow.json','mimeType':'application/json','buffer':b'{}'})
            page.locator('#library').click()
            page.evaluate('async()=>{window.releaseSlowDraft();await new Promise(resolve=>setTimeout(resolve,0));}')
            assert page.locator('#dialog-title').inner_text() == 'Community library'
            page.locator('#close-dialog').click()
            # Undo/Redo must restore raw project origin, not just visible notes.
            a, b = project_fixture(60, 0xa1), project_fixture(62, 0xb2)
            def open_project(raw):
                page.locator('#project-file').click()
                page.get_by_label('Revision-13 project file').set_input_files({'name':'project.bin','mimeType':'application/octet-stream','buffer':raw})
                page.get_by_role('button',name='Open musical draft from project',exact=True).click()
            def exported():
                page.locator('#project-file').click()
                with page.expect_download() as event:
                    page.get_by_role('button',name='Export edited project binary',exact=True).click()
                data = Path(event.value.path()).read_bytes()
                page.locator('#close-dialog').click()
                return data
            open_project(a); open_project(b)
            page.locator('#undo').click(); assert exported() == a
            page.locator('#redo').click(); assert exported() == b
            # Loading unrelated JSON detaches export; Undo restores the origin.
            unrelated = snap(); unrelated['patterns'][0]['bass'][0]['length'] = 32
            page.locator('#open-draft').set_input_files({'name':'other.json','mimeType':'application/json','buffer':json.dumps(unrelated).encode()})
            page.get_by_role('button',name='Replace local draft',exact=True).click()
            page.locator('#project-file').click()
            assert page.get_by_role('button',name='Export edited project binary',exact=True).is_disabled()
            page.locator('#close-dialog').click()
            page.locator('#undo').click(); assert exported() == b
            assert page.evaluate('window.midiCalls') == 0
            assert not errors, errors
            page.screenshot(path=str(OUT / f'f6b-{width}x{height}.png'), full_page=True)
            receipts.append({'viewport': [width, height], 'result': 'PASS', 'device_calls': 0})
            ctx.close()
        browser.close()
    (OUT / 'f6b-browser-receipt.json').write_text(json.dumps(receipts, indent=2) + '\n')

if __name__ == '__main__':
    run()
