// SPDX-License-Identifier: GPL-3.0-only
// Generated-site Chromium acceptance with MIDI removed or mocked. No real devices.
// X0X_PLAYWRIGHT_MODULE=<absolute playwright index.mjs> node web/test_workspace_browser.mjs
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { pathToFileURL } from 'node:url';
const module=process.env.X0X_PLAYWRIGHT_MODULE;
const {chromium}=await import(module ? pathToFileURL(module).href : 'playwright');
const base=process.env.X0X_SITE_URL || 'http://127.0.0.1:8765/';
const out=new URL('../build/web-site/',import.meta.url);mkdirSync(out,{recursive:true});
const browser=await chromium.launch({headless:true});
const results=[];
function wav(){const b=Buffer.alloc(44+44100*2);b.write('RIFF');b.writeUInt32LE(b.length-8,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);b.writeUInt16LE(1,20);b.writeUInt16LE(1,22);b.writeUInt32LE(44100,24);b.writeUInt32LE(88200,28);b.writeUInt16LE(2,32);b.writeUInt16LE(16,34);b.write('data',36);b.writeUInt32LE(b.length-44,40);for(let i=0;i<44100;i++)b.writeInt16LE(Math.round(12000*Math.sin(i/12)),44+i*2);return b}
try{
 for(const width of [320,390,1440]){
  const context=await browser.newContext({viewport:{width,height:900},reducedMotion:'reduce'});
  await context.addInitScript(()=>Object.defineProperty(navigator,'requestMIDIAccess',{value:undefined,configurable:true}));
  const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto(base);await page.locator('#browsernotice').waitFor({state:'visible'});await page.evaluate(()=>document.fonts.ready);
  assert.equal(await page.locator('#connbtn').isDisabled(),true);
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  for(const target of ['.site-nav a','.site-nav button','.workspace-nav a','#stockfile']){
    for(const node of await page.locator(target).all()){const b=await node.boundingBox();assert.ok(b.height>=44,`${width} ${target} target ${b.height}`)}
  }
  await page.locator('#stockfile').setInputFiles({name:'very-long-local-firmware-file-name-'.repeat(8)+'.fwsc',mimeType:'application/octet-stream',buffer:Buffer.from('bad')});
  await page.locator('#localstatus.inline-error').waitFor({state:'visible'});
  assert.equal(await page.locator('#stockgo').isDisabled(),true);assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  assert.equal(await page.locator('#localstatus').getAttribute('aria-live'),'polite');
  await page.locator('#lang').click();assert.equal(await page.locator('html').getAttribute('lang'),'ru');assert.match(await page.locator('#localstatus').innerText(),/файл/i);
  assert.equal(await page.locator('iframe').count(),2);
  await page.locator('[data-view="samples"]').click();await page.locator('#sampleprep').waitFor({state:'hidden'});assert.equal(await page.locator('#sampleprep').isVisible(),false);
  await page.frameLocator('#samplesframe').locator('#add').waitFor();
  await page.locator('[data-view="projects"]').click();await page.locator('#sampleprep').waitFor({state:'hidden'});assert.equal(await page.locator('#sampleprep').isVisible(),false);
  await page.frameLocator('#projectsframe').locator('#file').waitFor();
  await page.goBack();await page.locator('#samplesview').waitFor({state:'visible'});assert.equal(await page.locator('#samplesview').isVisible(),true);
  await page.locator('[data-view="install"]').click();await page.locator('#sampleprep').waitFor({state:'visible'});assert.equal(await page.locator('#sampleprep').isVisible(),true);
  await page.locator('#lang').click();await page.locator('#lang').click();
  assert.equal(await page.evaluate(()=>getComputedStyle(document.querySelector('#led')).animationName),'none');
  await page.keyboard.press('Tab');assert.ok(await page.evaluate(()=>document.activeElement!==document.body));
  await page.locator('#prepspeed').selectOption('2');await page.locator('#prepaudio').setInputFiles({name:'original-loop.wav',mimeType:'audio/wav',buffer:wav()});
  await page.locator('#preplist audio').waitFor();
  assert.match(await page.locator('#preplist').innerText(),/22050 Hz/);assert.match(await page.locator('#preplist').innerText(),/50% saved/);
  await page.locator('#preplist button').click();
  await page.frameLocator('#samplesframe').getByText('Prepared audio received.',{exact:false}).waitFor();
  const sampleFrame=page.frameLocator('#samplesframe');assert.equal(await sampleFrame.locator('#send').isDisabled(),true);
  assert.match(await sampleFrame.locator('#list').innerText(),/new/);
  assert.equal(await page.locator('#sampleprep').isVisible(),false);
  await page.locator('[data-view="install"]').click();
  await page.screenshot({path:new URL(`actual-${width}.png`,out).pathname,fullPage:true,timeout:15000});
  assert.deepEqual(errors,[]);results.push({width,result:'PASS',coverage:'layout, targets, unsupported, file error, RU, task/back navigation, reduced motion, offline preparation and exact sample handoff'});
  await context.close();
 }
 const context=await browser.newContext();await context.addInitScript(()=>Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>{throw new Error('synthetic permission refusal')}}));
 const page=await context.newPage();await page.goto(base);await page.locator('#connbtn').click();await page.getByRole('button',{name:'Try again',exact:true}).waitFor();
 assert.match(await page.locator('#connDetail').innerText(),/MIDI|permission|access|denied/i);results.push({result:'PASS',coverage:'mock permission refusal shows Try again; no real Web MIDI invocation'});await context.close();
 for(const path of ['manual/','new/','history/']){const page=await browser.newPage();await page.goto(base+path);assert.ok((await page.locator('h1').innerText()).length>3);assert.equal(await page.locator('link[href="../site.css"]').count(),1);await page.close()}
 const source={};for(const path of ['web/x0x_installer.html','web/x0x_breaks.html','web/x0x_projects.html','web/workspace.js','web/site.css','tools/make_pages.py'])source[path]=createHash('sha256').update(readFileSync(path)).digest('hex');
 writeFileSync(new URL('browser-acceptance.json',out),JSON.stringify({source,results,deviceAccess:'NONE: Web MIDI absent or stubbed',publication:'not performed'},null,2));console.log('PASS generated-site browser acceptance',results);
}finally{await browser.close()}
