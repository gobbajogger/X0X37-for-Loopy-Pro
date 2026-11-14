from playwright.sync_api import sync_playwright
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[1];out=[]
with sync_playwright() as p:
 b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
 for w in [1440,390,320]:
  pg=b.new_page(viewport={'width':w,'height':900})
  pg.goto('http://127.0.0.1:4321/#home');pg.keyboard.press('Tab');pg.evaluate("location.hash='learn/bass'");pg.wait_for_timeout(100)
  out.append(dict(case='direct hash initialized and heading focused',width=w,passed=pg.evaluate("!!history.state?.dnboxRoute && document.activeElement.tagName==='H1'")))
  pg.goto('http://127.0.0.1:4321/#home');pg.locator('footer a[href="#help/recover"]').click();pg.locator('#recovery').focus();pg.go_back();pg.wait_for_timeout(100);pg.go_forward();pg.wait_for_timeout(100)
  out.append(dict(case='focus-only Forward restores select',width=w,passed=pg.evaluate("document.activeElement.id==='recovery'")))
  pg.goto('http://127.0.0.1:4321/#learn/bass');pg.locator('main details summary').click();pg.locator('footer a[href="#home"]').scroll_into_view_if_needed();before=pg.evaluate('scrollY');pg.locator('footer a[href="#home"]').click();pg.go_back();pg.wait_for_timeout(150)
  after=pg.evaluate('scrollY');out.append(dict(case='expanded disclosure restores before scroll',width=w,before=before,after=after,passed=pg.locator('main details').evaluate('(d)=>d.open') and abs(before-after)<2))
  pg.goto('http://127.0.0.1:4321/#files');pg.locator('#sample-tool details summary').click();pg.locator('footer a[href="#home"]').scroll_into_view_if_needed();before=pg.evaluate('scrollY');pg.locator('footer a[href="#home"]').click();pg.go_back();pg.wait_for_timeout(150)
  after=pg.evaluate('scrollY');out.append(dict(case='sample disclosure restores before scroll',width=w,before=before,after=after,passed=pg.locator('#sample-tool details').evaluate('(d)=>d.open') and abs(before-after)<2));pg.close()
 b.close()
name=sys.argv[1] if len(sys.argv)>1 else 'history-edge-results.json';(ROOT/'evidence'/name).write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2));assert all(r['passed'] for r in out)
