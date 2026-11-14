// SPDX-License-Identifier: GPL-3.0-only
// Run with DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright node --test web/presets/test_preview_atomic.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {createServer} from 'node:http';
import {readFileSync} from 'node:fs';
import {dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {encodeDnpr} from './codec.mjs';

const {chromium} = createRequire(import.meta.url)(process.env.DNBOX_PLAYWRIGHT_MODULE || 'playwright');
const here = dirname(fileURLToPath(import.meta.url));
test('a rejected replacement preserves the existing validated preview and can be recovered', async () => {
  const server = createServer((req,res) => {
    const path = new URL(req.url,'http://localhost').pathname;
    const name = path === '/presets/' ? 'index.html' : path.slice('/presets/'.length);
    if (!path.startsWith('/presets/') || !['index.html','app.mjs','codec.mjs','zip.mjs'].includes(name)) {res.writeHead(404).end();return;}
    res.writeHead(200,{'Content-Type':name.endsWith('.mjs')?'text/javascript':'text/html'}).end(readFileSync(join(here,name)));
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  let browser;
  try {
    browser=await chromium.launch({executablePath:process.env.DNBOX_CHROMIUM || '/usr/bin/chromium',headless:true,args:['--no-sandbox']});
    const page=await browser.newPage();
    await page.goto(`http://127.0.0.1:${server.address().port}/presets/`);
    const bytes=encodeDnpr({engine:'chicago',origin:2,name:'Safe preview',params:new Uint8Array(22)});
    const bad=bytes.slice(); bad[16]^=1;
    const file=(name,buffer)=>({name,mimeType:'application/octet-stream',buffer:Buffer.from(buffer)});
    await page.locator('#files').setInputFiles(file('safe.dnpr',bytes));
    await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('passed file checks'));
    await page.locator('#files').setInputFiles(file('bad.dnpr',bad));
    await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('BAD FILE'));
    assert.equal(await page.locator('#preview').isVisible(),true);
    assert.match(await page.locator('#preview').innerText(),/Safe preview/);
    assert.match(await page.locator('#preview-status').innerText(),/bad\.dnpr.*previous validated preview/i);
    assert.equal(await page.locator('#session-list > button').count(),0);
    await page.locator('#keep').click();
    assert.equal(await page.locator('#session-list > button').count(),1);
    const result=page.waitForEvent('download');
    await page.locator('#save-file').click();
    assert.deepEqual(readFileSync(await (await result).path()),Buffer.from(bytes));
  } finally {await browser?.close(); await new Promise(resolve=>server.close(resolve));}
});
