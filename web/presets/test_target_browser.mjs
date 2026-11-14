// SPDX-License-Identifier: GPL-3.0-only
// Focused target/provenance replay against the actual page, not the full W3 matrix.
// Run: DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright DNBOX_CHROMIUM=/usr/bin/chromium node web/presets/test_target_browser.mjs
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFileSync, mkdtempSync, mkdirSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";
import assert from "node:assert/strict";
import { encodeDnpr } from "./codec.mjs";

const require=createRequire(import.meta.url);
const { chromium }=require(process.env.DNBOX_PLAYWRIGHT_MODULE || "playwright");
const here=dirname(fileURLToPath(import.meta.url));
const evidence=process.env.DNBOX_TARGET_EVIDENCE || mkdtempSync(join(tmpdir(),"dnbox-target-"));
mkdirSync(evidence,{recursive:true});
const sha256=b=>createHash("sha256").update(b).digest("hex");
const sources=["index.html","app.mjs","codec.mjs","zip.mjs","test_target_browser.mjs"];
const server=createServer((req,res)=>{
  const path=new URL(req.url,"http://127.0.0.1").pathname;
  const name=path==="/presets/" ? "index.html" : path.slice("/presets/".length);
  if(path==="/favicon.ico"){res.writeHead(204);res.end();return;}
  if(!path.startsWith("/presets/") || !sources.slice(0,4).includes(name)){res.writeHead(404);res.end();return;}
  res.writeHead(200,{"Content-Type":name.endsWith(".mjs") ? "text/javascript" : "text/html","Cache-Control":"no-store"});
  res.end(readFileSync(join(here,name)));
});
await new Promise((resolve,reject)=>{server.once("error",reject);server.listen(0,"127.0.0.1",resolve);});
const base="http://127.0.0.1:"+server.address().port;
const errors=[], external=[], cases=[], screenshots=[];
let browser;
try {
  browser=await chromium.launch({executablePath:process.env.DNBOX_CHROMIUM || "/usr/bin/chromium",headless:true,args:["--no-sandbox"]});
  for (const width of [1365,390,320]) for (const [label,engine,originalEngine,params,wrong] of [
    ["P1","chicago",2,22,"miami"],["P2","miami",1,18,"chicago"]
  ]) {
    const page=await browser.newPage({viewport:{width,height:900}});
    page.on("pageerror",e=>errors.push(e.message));
    page.on("request",r=>{if(!r.url().startsWith(base+"/"))external.push(r.url());});
    await page.goto(base+"/presets/");
    const bytes=encodeDnpr({engine,originalEngine,origin:2,name:label,sourceName:"SYNTHETIC",
      params:Uint8Array.from({length:params},(_,i)=>i+3)});
    const file={name:label+".dnpr",mimeType:"application/octet-stream",buffer:Buffer.from(bytes)};
    const waitStatus=text=>page.waitForFunction(t=>document.getElementById("status").textContent.includes(t),text);
    await page.selectOption("#target",engine);
    await page.locator("#files").setInputFiles(file);
    await waitStatus("passed file checks");
    assert.equal(await page.locator("#session-list > button").count(),0);
    const warning=await page.locator("#preview-warnings").innerText();
    assert.match(warning,/Original engine differs from record engine/);
    assert.match(warning,/retained as provenance/);
    assert.match(warning,/actual record engine; no conversion is performed/);
    assert.doesNotMatch(warning,/blocked unless both agree/);
    assert.ok(await page.locator("#preview-warnings").isVisible());
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
    const name=label+"-"+width+"-warning.png", screenshot=await page.screenshot({path:join(evidence,name),fullPage:true});
    screenshots.push({name,sha256:sha256(screenshot)});
    await page.locator("#keep").click(); await waitStatus("Added 1 validated");
    assert.match(await page.locator("#selected-warnings").innerText(),/retained as provenance/);
    const downloaded=page.waitForEvent("download"); await page.locator("#save-file").click();
    const download=await downloaded;
    assert.deepEqual(readFileSync(await download.path()),Buffer.from(bytes));
    await page.selectOption("#target",wrong);
    await page.locator("#files").setInputFiles(file); await waitStatus("WRONG ENGINE");
    assert.match(await page.locator("#status").innerText(),/Target differs from actual record engine/);
    assert.equal(await page.locator("#preview").isVisible(),false);
    assert.equal(await page.locator("#session-list > button").count(),1);
    await page.selectOption("#target","");
    await page.locator("#files").setInputFiles(file); await waitStatus("passed file checks");
    assert.equal(await page.locator("#session-list > button").count(),1);
    cases.push({name:label+" at "+width+"px",status:"PASS",warning,fixture_sha256:sha256(bytes)});
    await page.close();
  }
  assert.deepEqual(errors,[]); assert.deepEqual(external,[]);
  const result={chromium:browser.version(),cases,screenshots,pageErrors:errors,externalRequests:external,
    source_sha256:Object.fromEntries(sources.map(name=>[name,sha256(readFileSync(join(here,name)))]))};
  writeFileSync(join(evidence,"result.json"),JSON.stringify(result,null,2)+"\n");
  console.log(JSON.stringify({passed:cases.length,failed:0,evidence,chromium:result.chromium}));
} finally {
  if(browser)await browser.close();
  await new Promise(resolve=>server.close(resolve));
}
