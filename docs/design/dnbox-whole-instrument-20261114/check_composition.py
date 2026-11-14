"""Affected host composition/LOAD interactions only; never firmware execution."""
import asyncio
import base64
import hashlib
import json
from pathlib import Path
from playwright.async_api import async_playwright

HERE = Path(__file__).parent

async def main():
    payload = (HERE/'walkthrough.html').read_bytes()
    manifest = json.loads((HERE/'candidate-manifest.json').read_text())
    assert hashlib.sha256(payload).hexdigest() == manifest['candidate_html_sha256']
    result = {'candidate_sha256':manifest['candidate_html_sha256'], 'scope':'Affected embedded Settings navigation/lifecycle, six unchanged pair presentations and normal/clip LOAD hint/model semantics only', 'not_run':['historical emulator','firmware','audio','storage','physical hardware','unchanged whole152/103 and Settings105 broad suites'], 'not_qualified':['Real overlapping pointer/frame teardown: prior attempt hung in Playwright mouse.up after frame removal; interrupted exit130. Keyboard-owned teardown below is a distinct check.'], 'checks':[], 'errors':[], 'network_requests':[]}
    def check(name, condition, detail=None):
        result['checks'].append({'name':name, 'pass':bool(condition), 'detail':detail})
        (HERE/'affected-validation-progress.json').write_text(json.dumps(result,indent=2)+'\n')
        assert condition, name
    async with async_playwright() as pw:
        browser = await pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
        page = await browser.new_page(viewport={'width':1280,'height':1000},device_scale_factor=1)
        page.set_default_timeout(10000)
        page.on('pageerror', lambda e:result['errors'].append(str(e)))
        page.on('request', lambda r:result['network_requests'].append(r.url))
        await page.route('**/*', lambda route:route.abort())
        await page.set_content(payload.decode(), wait_until='load')
        await page.locator('[data-tab=settings]').click()
        data = await page.evaluate('window.__designData.settings')
        check('Original Settings component bytes preserved', hashlib.sha256(base64.b64decode(data['component_b64'])).hexdigest()==manifest['r2_component_sha256'])
        check('Six curated workflows',len(data['cases'])==6)
        for i,c in enumerate(data['cases']):
            await page.locator('#settingsChoice').select_option(str(i))
            text = await page.locator('#settingsPair').inner_text()
            check('Comparison boundary and tradeoff retained: '+c['id'],c['match'] in text and c['cost'] in text)
            geometry = await page.locator('#settingsPair img').evaluate_all('(xs)=>xs.map(e=>({width:e.getBoundingClientRect().width,height:e.getBoundingClientRect().height,nw:e.naturalWidth,nh:e.naturalHeight}))')
            check('Actual and proposal native geometry: '+c['id'],all(set(g.values())=={240} for g in geometry),geometry)
            await page.locator('#openSettings').click()
            await page.wait_for_function("document.querySelector('#settingsStage').textContent.startsWith('Original R2 inspection task:')")
            child = page.frame_locator('#settingsFrame')
            check('Task staged: '+c['id'],c['task'] in await page.locator('#settingsStage').inner_text())
            geometry = await child.locator('#lcd').evaluate('(e)=>({w:e.getBoundingClientRect().width,h:e.getBoundingClientRect().height})')
            check('Inspection native canvas geometry: '+c['id'],geometry=={'w':240,'h':240},geometry)
            await page.locator('#closeSettings').click()
            check('Closed inspection status is truthful: '+c['id'],'inspection closed' in await page.locator('#settingsStage').inner_text())
        # Host routes exact component references without broken relative URLs.
        await page.locator('#settingsChoice').select_option('1')
        await page.locator('#openSettings').click()
        await page.wait_for_function("document.querySelector('#settingsStage').textContent.startsWith('Original R2 inspection task:')")
        frame = next(f for f in page.frames if f != page.main_frame)
        child = page.frame_locator('#settingsFrame')
        for name in ['README.md','source-receipt.json','qa/interaction-results.json','emulator-evidence.json']:
            await child.locator('a').filter(has_text={'README.md':'Task cards, rationale and exact source links','source-receipt.json':'Verified input receipt','qa/interaction-results.json':'Executed model checks','emulator-evidence.json':'Separate historical emulator receipt'}[name]).click()
            check('Embedded reference route: '+name,await page.locator('#settingsReferenceText').inner_text()==data['references'][name])
        # Normal running LOAD cancellation sends stop; stopped cancellation sends start.
        await child.locator('[data-event="SELECT+"]').click()
        await child.locator('[data-event="SCALE"]').click()
        state = await frame.evaluate('study.state')
        check('Dirty LOAD question opened from PROJECT',state['modal']['act']=='load' and state['playing'] and state['dirty'])
        await child.locator('[data-event="SEQ PLAY"]').click()
        state = await frame.evaluate('study.state')
        check('Normal running PLAY cancels plus stop, preserves edits and no LOAD attempt',state['modal'] is None and not state['playing'] and state['dirty'] and state['counts']['load']==0)
        await child.locator('[data-event="SCALE"]').click()
        check('Stopped modal accessible hint names start','cancels and starts' in await child.locator('#lcd').get_attribute('aria-label'))
        await child.locator('[data-event="SEQ PLAY"]').click()
        state = await frame.evaluate('study.state')
        check('Normal stopped PLAY cancels plus start command',state['modal'] is None and state['playing'] and state['dirty'] and state['counts']['load']==0)
        # Clip fixture is deliberately enabled only after an open normal question.
        await child.locator('[data-event="SCALE"]').click()
        await child.locator('#clip').check()
        await child.locator('[data-event="SEQ PLAY"]').click()
        state = await frame.evaluate('study.state')
        check('Clip PLAY delegated with question retained',state['modal']['act']=='load' and state['playing'] and state['counts']['load']==0)
        await child.locator('[data-event="SCALE"]').click()
        state = await frame.evaluate('study.state')
        check('Clip physical SCALE masked, LOAD not activated',state['modal']['act']=='load' and state['counts']['load']==0 and state['ack']=='SCALE masked')
        await child.locator('[data-event="PATCH"]').click()
        state = await frame.evaluate('study.state')
        check('PATCH cancels clip modal without transport toggle',state['modal'] is None and state['playing'])
        # A host handler destroys the context with a keyboard-owned repeat.
        # The separate real-pointer/capture protocol diagnostic is unqualified.
        await child.locator('#clip').uncheck()
        arrow = child.locator('[data-arrow="1"]')
        await arrow.focus()
        await page.keyboard.down('Enter')
        owners = await frame.evaluate('study.repeatOwners')
        check('Keyboard held-arrow owner acquired before leaving',owners['inputs']>0,owners)
        await page.locator('[data-tab=compare]').evaluate('(button)=>button.click()')
        await asyncio.wait_for(page.keyboard.up('Enter'),timeout=5)
        check('Leaving Settings disposes frame',await page.locator('#settingsFrame').count()==0 and frame.is_detached())
        await page.locator('[data-tab=settings]').click()
        await page.locator('#openSettings').click()
        await page.wait_for_function("document.querySelector('#settingsStage').textContent.startsWith('Original R2 inspection task:')")
        frame = next(f for f in page.frames if f != page.main_frame)
        check('Reentry creates clean held-input ownership',(await frame.evaluate('study.repeatOwners'))=={'inputs':0,'timer':False})
        child = page.frame_locator('#settingsFrame')
        await child.locator('a[href="overview.html"]').first.click()
        await page.wait_for_function("!document.querySelector('#settingsFrame')")
        check('Overview route returns to outer curated selector with focus',await page.evaluate('document.activeElement.id')=='settingsChoice')
        # Inspect the actual new whole native LCD and readable source text.
        await page.locator('#settings > details > summary').click()
        await page.locator('#showLoad').click()
        check('LOAD inspection routes to whole native screen',await page.locator('#screenChoice').input_value()=='LOAD-QUESTION' and await page.locator('#compare').is_visible())
        await page.locator('#screenView summary').click()
        text = await page.locator('#screenView .textscreen').inner_text()
        check('Whole native readable LOAD consequence matches pixels','SEQ PLAY: cancel + start' in text and 'CLIP: transport; question stays' in text and 'DISCARD IF LOADED' in text)
        check('Exact sequencer source locator corrected','firmware/src/seq/sequencer.c' in await page.locator('#screenView').inner_text())
        await page.set_viewport_size({'width':390,'height':900})
        await page.locator('[data-tab=settings]').click()
        await page.locator('#openSettings').click()
        await page.wait_for_function("document.querySelector('#settingsStage').textContent.startsWith('Original R2 inspection task:')")
        child = page.frame_locator('#settingsFrame')
        geometry = await child.locator('#lcd').evaluate('(e)=>({w:e.getBoundingClientRect().width,h:e.getBoundingClientRect().height})')
        check('Narrow viewport retains unscaled native canvas',geometry=={'w':240,'h':240})
        await page.locator('#closeSettings').focus()
        await page.keyboard.press('Enter')
        check('Keyboard closes inspection and restores open-button focus',await page.locator('#settingsFrame').count()==0 and await page.evaluate('document.activeElement.id')=='openSettings')
        check('No browser errors',not result['errors'],result['errors'])
        check('No network requests',not result['network_requests'],result['network_requests'])
        await browser.close()
    (HERE/'affected-validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'candidate_sha256':result['candidate_sha256'],'passed':len(result['checks']),'failed':0,'scope':result['scope']}))

if __name__=='__main__':
    asyncio.run(main())
