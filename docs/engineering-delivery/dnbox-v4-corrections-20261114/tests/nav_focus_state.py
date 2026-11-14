from pathlib import Path
import hashlib
import json

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
BASE = 'http://127.0.0.1:4321/'
results = []


def record(case, width, passed, **evidence):
    results.append(dict(case=case, width=width, passed=passed, **evidence))


with sync_playwright() as p:
    browser = p.chromium.launch(executable_path='/usr/bin/chromium', headless=True)
    for width in [1440, 390, 320]:
        context = browser.new_context(viewport={'width': width, 'height': 900})
        page = context.new_page()
        errors = []
        page.on('pageerror', lambda error: errors.append(str(error)))
        page.goto(BASE + '#home')
        page.evaluate("location.hash='learn/bass'")
        page.wait_for_timeout(100)
        page.go_back()
        page.wait_for_timeout(100)
        page.go_forward()
        page.wait_for_timeout(100)
        record('Forward restores intentional no-ID H1 focus', width,
               page.evaluate("document.activeElement.tagName==='H1'"))

        page.goto(BASE + '#home')
        page.evaluate("location.hash='learn/controls'")
        page.wait_for_timeout(100)
        page.locator('.track-choice').nth(4).focus()
        saved = page.evaluate('history.state.dnboxRoute.focus')
        page.go_back()
        page.wait_for_timeout(100)
        page.go_forward()
        page.wait_for_timeout(100)
        record('Forward restores no-ID native button without a link exit', width,
               page.evaluate("document.activeElement.dataset.track==='4'"), saved=saved)

        page.goto(BASE + '#home')
        repeated = page.locator('footer a[href="#files"]')
        repeated.focus()
        saved = page.evaluate('history.state.dnboxRoute.focus')
        page.evaluate("location.hash='learn/bass'")
        page.wait_for_timeout(100)
        page.go_back()
        page.wait_for_timeout(100)
        record('Back restores the exact repeated-href link', width,
               repeated.evaluate('(a)=>a===document.activeElement'), saved=saved)

        page.goto(BASE + '#learn/bass')
        page.locator('main details summary').click()
        page.wait_for_timeout(50)
        page.evaluate("location.hash='learn/movement'")
        page.wait_for_timeout(100)
        page.evaluate("location.hash='learn/bass'")
        page.wait_for_timeout(100)
        newest_closed = not page.locator('main details').evaluate('(d)=>d.open')
        page.go_back()
        page.wait_for_timeout(100)
        page.go_back()
        page.wait_for_timeout(100)
        old_open = page.locator('main details').evaluate('(d)=>d.open')
        old_summary_focused = page.locator('main details summary').evaluate('(s)=>s===document.activeElement')
        page.go_forward()
        page.wait_for_timeout(100)
        page.go_forward()
        page.wait_for_timeout(100)
        new_closed = not page.locator('main details').evaluate('(d)=>d.open')
        record('Repeated route entries keep independent disclosure state and summary focus', width,
               newest_closed and old_open and old_summary_focused and new_closed)

        page.goto(BASE + '#learn/bass')
        page.locator('main details summary').click()
        page.locator('footer a[href="#home"]').scroll_into_view_if_needed()
        before = page.evaluate('scrollY')
        page.locator('main details summary').evaluate('(s)=>s.focus({preventScroll:true})')
        page.goto(BASE + 'assets/key-locator.svg')
        page.go_back()
        page.wait_for_timeout(150)
        after = page.evaluate('scrollY')
        record('Same-origin full-document Back restores disclosure, summary focus and position', width,
               page.locator('main details').evaluate('(d)=>d.open')
               and page.locator('main details summary').evaluate('(s)=>s===document.activeElement')
               and abs(before - after) < 2,
               before=before, after=after,
               navigation_type=page.evaluate("performance.getEntriesByType('navigation')[0].type"))

        page.goto(BASE + '#home')
        if width < 760:
            page.locator('#menu').click()
        with context.expect_page() as opened:
            page.locator('#nav a[href="#learn"]').click(modifiers=['Control'])
        popup = opened.value
        popup.wait_for_load_state()
        record('Control-click retains native new-tab navigation', width,
               page.url.endswith('#home') and popup.url.endswith('#learn'))
        popup.close()
        record('No page errors', width, not errors, errors=errors)
        context.close()
    browser.close()

report = dict(navigation_sha256=hashlib.sha256((ROOT / 'site/navigation.js').read_bytes()).hexdigest(),
              checks=results)
(ROOT / 'evidence/nav-fix-focus-state-after.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
assert all(result['passed'] for result in results)
