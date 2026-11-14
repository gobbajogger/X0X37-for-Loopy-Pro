// SPDX-License-Identifier: GPL-3.0-only
// User-workflow regressions on the real page. Run with:
// DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright node --test web/presets/test_workflow_browser.mjs
import test, { before, after } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { createServer } from 'node:http';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { encodeDnpr } from './codec.mjs';
import { makeDnprZip, readDnprZip } from './zip.mjs';

const require = createRequire(import.meta.url);
const { chromium } = require(process.env.DNBOX_PLAYWRIGHT_MODULE || 'playwright');
const here = dirname(fileURLToPath(import.meta.url));
const valid = encodeDnpr({engine:'chicago', origin:2, name:'User One', params:new Uint8Array(22)});
const factory = encodeDnpr({engine:'miami', origin:1, factoryIndex:1, name:'Factory Two', params:new Uint8Array(18)});
const drum = encodeDnpr({engine:'909', origin:2, name:'Drum Three', params:new Uint8Array(132),
  sampleRefs:Array.from({length:4},(_,i)=>({source:1,index:i,crc:i+1,name:'S'+i}))});
const asFile = (name, bytes) => ({name, mimeType:'application/octet-stream', buffer:Buffer.from(bytes)});
const files = [asFile('user.dnpr',valid),asFile('factory.dnpr',factory)];
let server, browser, base;
before(async () => {
  server = createServer((req,res) => {
    const path = new URL(req.url,'http://localhost').pathname;
    const name = path === '/presets/' ? 'index.html' : path.slice('/presets/'.length);
    if (!path.startsWith('/presets/') || !['index.html','app.mjs','codec.mjs','zip.mjs'].includes(name)) { res.writeHead(404); res.end(); return; }
    res.writeHead(200,{'Content-Type':name.endsWith('.mjs')?'text/javascript':'text/html','Cache-Control':'no-store'});
    res.end(readFileSync(join(here,name)));
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  base = `http://127.0.0.1:${server.address().port}/presets/`;
  browser = await chromium.launch({executablePath:process.env.DNBOX_CHROMIUM || '/usr/bin/chromium',headless:true,args:['--no-sandbox']});
});
after(async () => { await browser?.close(); await new Promise(resolve=>server?.close(resolve)); });
async function fresh(t, width=1365) {
  const context = await browser.newContext({viewport:{width,height:900},acceptDownloads:true});
  t.after(()=>context.close());
  const page=await context.newPage();
  const errors=[]; page.on('pageerror', e=>errors.push(e.message));
  t.after(()=>assert.deepEqual(errors,[]));
  await page.goto(base);
  return page;
}
async function choose(page, selected) {
  await page.locator('#files').setInputFiles(selected);
  await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('passed file checks'));
}
async function keep(page) { await page.locator('#keep').click(); await page.waitForFunction(()=>document.querySelector('#session-list').children.length > 0); }

test('mixed eligible and factory records: ZIP downloads only eligible unchanged bytes without removing records', async t=>{
  const page=await fresh(t);
  await choose(page,files); await keep(page);
  const downloadPromise=page.waitForEvent('download',{timeout:3000});
  await page.locator('#save-zip').click();
  const download=await downloadPromise;
  const archive=await readDnprZip(new Uint8Array(readFileSync(await download.path())));
  assert.equal(archive.length,1);
  assert.deepEqual(archive[0].bytes,valid);
  assert.equal(await page.locator('#session-list > button').count(),2);
  assert.match(await page.locator('#status').innerText(),/1/);
});

test('multi-record warnings appear within the affected record, at desktop and mobile widths', async t=>{
  for (const width of [1365,320]) {
    const page=await fresh(t,width);
    await choose(page,[asFile('user.dnpr',valid),asFile('factory.dnpr',factory),asFile('drum.dnpr',drum)]);
    const result=await page.locator('#preview').evaluate(root=>{
      const groups=[...root.querySelectorAll('.preview-record')];
      return groups.map(group=>group.textContent);
    });
    assert.equal(result.length,3);
    assert.doesNotMatch(result[0],/FACTORY EXPORT OFF|not checked against keyboard/);
    assert.match(result[1],/FACTORY EXPORT OFF/);
    assert.match(result[2],/not checked against keyboard/);
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth+1),false);
  }
});

test('keyboard focus has a visible useful destination after Keep and Discard', async t=>{
  const page=await fresh(t);
  await choose(page,asFile('user.dnpr',valid));
  await page.locator('#keep').focus(); await page.keyboard.press('Enter');
  await page.waitForFunction(()=>document.querySelector('#session-list').children.length===1);
  let focus=await page.evaluate(()=>({id:document.activeElement.id,tag:document.activeElement.tagName,visible:!!document.activeElement.getClientRects().length}));
  assert.ok(focus.visible && focus.tag!=='BODY',`Keep stranded focus: ${JSON.stringify(focus)}`);
  await choose(page,asFile('user.dnpr',valid));
  await page.locator('#discard').focus(); await page.keyboard.press('Enter');
  focus=await page.evaluate(()=>({id:document.activeElement.id,tag:document.activeElement.tagName,visible:!!document.activeElement.getClientRects().length}));
  assert.ok(focus.visible && focus.tag!=='BODY',`Discard stranded focus: ${JSON.stringify(focus)}`);
});

test('failed download reports failure and preserves session, selected record and source bytes', async t=>{
  const page=await fresh(t);
  await choose(page,asFile('user.dnpr',valid)); await keep(page);
  await page.evaluate(()=>{ window.originalCreateObjectURL=URL.createObjectURL; URL.createObjectURL=()=>{throw Error('synthetic download failure');}; });
  await page.locator('#save-file').click();
  assert.match(await page.locator('#status').innerText(),/synthetic download failure/);
  assert.equal(await page.locator('#session-list > button').count(),1);
  assert.equal(await page.locator('#session-list > button').getAttribute('aria-pressed'),'true');
  await page.evaluate(()=>{URL.createObjectURL=window.originalCreateObjectURL;});
  const promise=page.waitForEvent('download'); await page.locator('#save-file').click();
  assert.deepEqual(readFileSync(await (await promise).path()),Buffer.from(valid));
});

test('bad member in a ZIP cannot add partial records or alter an existing download', async t=>{
  const page=await fresh(t);
  await choose(page,asFile('user.dnpr',valid)); await keep(page);
  const bad=valid.slice(); bad[16]^=1;
  const archive=makeDnprZip([{name:'ok.dnpr',bytes:valid},{name:'bad.dnpr',bytes:bad}]);
  await page.locator('#files').setInputFiles(asFile('mixed.zip',archive));
  await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('BAD FILE'));
  assert.equal(await page.locator('#session-list > button').count(),1);
  assert.equal(await page.locator('#preview').isVisible(),false);
  const promise=page.waitForEvent('download'); await page.locator('#save-file').click();
  assert.deepEqual(readFileSync(await (await promise).path()),Buffer.from(valid));
});


test('invalid replacement preserves earlier validated preview and allows it to be kept', async t=>{
  const page=await fresh(t);
  await choose(page,asFile('user.dnpr',valid));
  const bad=valid.slice(); bad[16]^=1;
  await page.locator('#files').setInputFiles(asFile('bad.dnpr',bad));
  await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('BAD FILE'));
  assert.equal(await page.locator('#preview').isVisible(),true,'A failed replacement must leave the original validated preview available');
  assert.match(await page.locator('#preview').innerText(),/User One/);
  await keep(page);
  const promise=page.waitForEvent('download'); await page.locator('#save-file').click();
  assert.deepEqual(readFileSync(await (await promise).path()),Buffer.from(valid));
});

test('changing the target while a file read is pending invalidates stale previews and results', async t=>{
  const page=await fresh(t);
  await choose(page,asFile('user.dnpr',valid));
  await page.evaluate(bytes=>{
    let resolveRead;
    window.releaseRead=()=>resolveRead(Uint8Array.from(bytes).buffer);
    const delayed=new File([Uint8Array.from(bytes)],'slow.dnpr');
    delayed.arrayBuffer=()=>new Promise(resolve=>{resolveRead=resolve;});
    const transfer=new DataTransfer(); transfer.items.add(delayed);
    document.getElementById('drop').dispatchEvent(new DragEvent('drop',{bubbles:true,cancelable:true,dataTransfer:transfer}));
  },[...valid]);
  await page.selectOption('#target','miami');
  await page.evaluate(()=>window.releaseRead());
  await page.waitForTimeout(100);
  assert.equal(await page.locator('#preview').isVisible(),false);
  assert.equal(await page.locator('#session-list > button').count(),0);
  assert.doesNotMatch(await page.locator('#status').innerText(),/passed file checks/);
});

test('failed ZIP download leaves records intact and a retry returns unchanged eligible bytes', async t=>{
  const page=await fresh(t);
  await choose(page,files); await keep(page);
  await page.evaluate(()=>{window.originalCreateObjectURL=URL.createObjectURL; URL.createObjectURL=()=>{throw Error('synthetic ZIP failure');};});
  await page.locator('#save-zip').click();
  assert.match(await page.locator('#status').innerText(),/synthetic ZIP failure/);
  assert.equal(await page.locator('#session-list > button').count(),2);
  await page.evaluate(()=>{URL.createObjectURL=window.originalCreateObjectURL;});
  const promise=page.waitForEvent('download'); await page.locator('#save-zip').click();
  const result=await readDnprZip(new Uint8Array(readFileSync(await (await promise).path())));
  assert.equal(result.length,1);
  assert.deepEqual(result[0].bytes,valid);
  assert.equal(await page.locator('#session-list > button').count(),2);
});
