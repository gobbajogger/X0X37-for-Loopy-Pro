// SPDX-License-Identifier: GPL-3.0-only
// Browser-level W3 acceptance: real Chromium + native file upload/download.
// Run: DNBOX_PUPPETEER_MODULE=/path/to/puppeteer DNBOX_CHROMIUM=/usr/bin/chromium node web/presets/test_browser.mjs
// No external network, MIDI/USB, firmware, installation or deployment is used.
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { mkdtempSync, mkdirSync, readFileSync, readdirSync, writeFileSync, existsSync } from "node:fs";
import { join, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";
import { tmpdir } from "node:os";
import { createHash } from "node:crypto";
import assert from "node:assert/strict";
import { encodeDnpr, DNPR_ENGINES, crc32 } from "./codec.mjs";
import { makeDnprZip, readDnprZip } from "./zip.mjs";
const require = createRequire(import.meta.url);
const puppeteerModule = process.env.DNBOX_PUPPETEER_MODULE || "puppeteer";
const puppeteer = require(puppeteerModule);
const puppeteerVersion = require(puppeteerModule.startsWith("/") ? join(puppeteerModule,"package.json") : require.resolve("puppeteer/package.json")).version;
const here = dirname(fileURLToPath(import.meta.url));
const sha256 = b => createHash("sha256").update(b).digest("hex");
const sourceNames = ["index.html", "app.mjs", "codec.mjs", "zip.mjs"];
const mime = { ".html":"text/html; charset=utf-8", ".mjs":"text/javascript; charset=utf-8" };
const fixturesDir = mkdtempSync(join(tmpdir(), "dnbox-w3-fixtures-"));
const evidenceDir = process.env.DNBOX_BROWSER_EVIDENCE || mkdtempSync(join(tmpdir(), "dnbox-w3-evidence-"));
mkdirSync(evidenceDir, { recursive:true });
const record = (engine, overrides={}) => {
  const spec = Object.values(DNPR_ENGINES).find(x => x.token === engine);
  const refs = Array.from({length:overrides.refs ?? (spec.samples === -1 ? 0 : spec.samples)}, (_,i) =>
    ({source:1,index:i,crc:(0x22334400+i)>>>0,name:"S"+i}));
  return encodeDnpr({
    engine, origin:overrides.origin ?? 2, factoryIndex:overrides.factoryIndex ?? 0,
    name:overrides.name ?? ("Owner " + engine), sourceName:"OWNER", date:20261113,
    params:Uint8Array.from({length:spec.params},(_,i)=>(i*5+13)&255), sampleRefs:refs
  });
};
const file = (name, bytes) => { const path=join(fixturesDir,name); writeFileSync(path,bytes); return path; };
const fixture = {
  chicago:file("owner.chicago.dnpr",record("chicago")),
  miami:file("owner.miami.dnpr",record("miami")),
  juno:file("owner.juno.dnpr",record("juno")),
  drone:file("owner.drone.dnpr",record("drone")),
  drum:file("owner.808.dnpr",record("808",{refs:2})),
  kit:file("owner.909.dnpr",record("909")),
  brk:file("owner.break.dnpr",record("break")),
  factory:file("factory.miami.dnpr",record("miami",{origin:1,factoryIndex:1}))
};
const corrupt=Uint8Array.from(readFileSync(fixture.chicago)); corrupt[16]^=1;
fixture.bad=file("bad.chicago.dnpr",corrupt);
const newer=Uint8Array.from(readFileSync(fixture.chicago));
newer[8]=2;new DataView(newer.buffer).setUint32(16,crc32(newer,true),true);
fixture.newer=file("newer.chicago.dnpr",newer);
fixture.model=file("owner.elite.chicago.dnpr",readFileSync(fixture.chicago));
fixture.zip=file("owner.zip",makeDnprZip([
  {name:"one.juno.dnpr",bytes:new Uint8Array(readFileSync(fixture.juno))},
  {name:"two.drone.dnpr",bytes:new Uint8Array(readFileSync(fixture.drone))}
]));

const cases=[], errors=[], unexpectedRequests=[];
const run=async (name, fn)=>{
  try { await fn(); cases.push({name,status:"PASS"}); console.log("PASS",name); }
  catch(e) {cases.push({name,status:"FAIL",error:e.message}); console.error("FAIL",name,e.stack); }
};
const http=createServer((req,res)=>{
  const url=new URL(req.url,"http://127.0.0.1");
  const name=url.pathname==="/presets/" ? "index.html" : url.pathname.startsWith("/presets/") ? basename(url.pathname) : "";
  if(name==="favicon.ico"){res.writeHead(204);res.end();return;}
  if(!sourceNames.includes(name)){res.writeHead(404);res.end();return;}
  const contents=readFileSync(join(here,name));
  res.writeHead(200,{"Content-Type":mime[name.slice(name.lastIndexOf("."))],"Cache-Control":"no-store"});
  res.end(contents);
});
const listen=()=>new Promise((resolve,reject)=>{
  http.once("error",reject);http.listen(0,"127.0.0.1",()=>resolve(http.address().port));
});
const waitStatus=(page,text)=>page.waitForFunction(t=>document.querySelector("#status")?.textContent.includes(t),{timeout:6000},text);
const status=page=>page.$eval("#status",x=>x.textContent);
const count=page=>page.$$eval("#session-list > button",items=>items.length);
const upload=async(page,path)=>{await (await page.$("#files")).uploadFile(path);};
const screenshot=async(page,name)=>{
  const path=join(evidenceDir,name);
  await page.screenshot({path,fullPage:true});
  const b=readFileSync(path);
  return {name,bytes:b.length,sha256:sha256(b)};
};
const download=async (directory,name)=>{
  const path=join(directory,name);
  for(let i=0;i<80;i++){
    if(existsSync(path) && !readdirSync(directory).some(x=>x.endsWith(".crdownload")))return readFileSync(path);
    await new Promise(r=>setTimeout(r,50));
  }
  throw Error("Download was not committed: "+name+"; entries: "+readdirSync(directory).join(","));
};
let browser;
try {
  const port=await listen(), url="http://127.0.0.1:"+port+"/presets/";
  const profile=mkdtempSync(join(tmpdir(),"dnbox-w3-chromium-profile-"));
  browser=await puppeteer.launch({
    headless:true,executablePath:process.env.DNBOX_CHROMIUM || "/usr/bin/chromium",
    args:["--no-sandbox","--disable-dev-shm-usage","--no-first-run","--user-data-dir="+profile]
  });
  const version=await browser.version();
  const downloads=join(evidenceDir,"downloads");mkdirSync(downloads,{recursive:true});
  const bcdp=await browser.target().createCDPSession();
  await bcdp.send("Browser.setDownloadBehavior",{behavior:"allow",downloadPath:downloads,eventsEnabled:true});
  const makePage=async(viewport)=>{
    const p=await browser.newPage();await p.setViewport(viewport);
    p.on("pageerror",e=>errors.push(e.message));
    p.on("request",r=>{if(!r.url().startsWith("http://127.0.0.1:"+port+"/"))unexpectedRequests.push(r.url());});
    const response=await p.goto(url,{waitUntil:"networkidle0",timeout:12000});
    assert.equal(response.status(),200);
    await p.waitForSelector("#files");
    return p;
  };
  const desktop=await makePage({width:1365,height:900,deviceScaleFactor:1});
  const desktopShots=[];
  await run("Chromium desktop: dark-only layout, all controls, no horizontal overflow",async()=>{
    const info=await desktop.evaluate(()=>{
      const root=getComputedStyle(document.documentElement);
      const controls=["model","target","files","keep","discard","save-file","save-zip","clear"];
      return {title:document.title,scheme:root.colorScheme,theme:root.getPropertyValue("--bg").trim(),
        width:document.documentElement.scrollWidth,inner:innerWidth,missing:controls.filter(id=>!document.getElementById(id))};
    });
    assert.match(info.title,/DnBOX/);assert.equal(info.scheme,"dark");
    assert.equal(info.missing.length,0);assert.ok(info.width<=info.inner,JSON.stringify(info));
    desktopShots.push(await screenshot(desktop,"desktop-empty.png"));
  });
  await run("Native file picker: preview is non-mutating until explicit confirmation",async()=>{
    await upload(desktop,fixture.chicago);await waitStatus(desktop,"passed file checks");
    assert.equal(await count(desktop),0);
    assert.equal(await desktop.$eval("#preview",e=>e.hidden),false);
    assert.match(await desktop.$eval("#preview-meta",e=>e.textContent),/Chicago/);
    desktopShots.push(await screenshot(desktop,"desktop-chicago-preview.png"));
  });
  await run("Keyboard: reachable Keep action and unchanged .dnpr download",async()=>{
    await desktop.focus("#files");
    await desktop.keyboard.press("Tab");
    assert.equal(await desktop.evaluate(()=>document.activeElement.id),"keep");
    await desktop.keyboard.press("Enter"); await waitStatus(desktop,"Added 1 validated");
    assert.equal(await count(desktop),1);
    await desktop.click("#save-file");
    await waitStatus(desktop,"Download requested");
    const bytes=await download(downloads,"Owner-chicago.chicago.dnpr");
    assert.equal(Buffer.compare(bytes,readFileSync(fixture.chicago)),0,"Downloaded record must be byte identical");
    desktopShots.push(await screenshot(desktop,"desktop-kept.png"));
  });
  await run("Actual Chromium bad CRC rejects without disturbing existing session",async()=>{
    await upload(desktop,fixture.bad);await waitStatus(desktop,"BAD FILE");
    assert.equal(await count(desktop),1);
    assert.equal(await desktop.$eval("#preview",e=>e.hidden),true);
  });
  await run("Actual Chromium newer schema rejects with NEWER DATA",async()=>{
    await upload(desktop,fixture.newer);await waitStatus(desktop,"NEWER DATA");
    assert.equal(await count(desktop),1);
  });
  await run("Target engine mismatch shows WRONG ENGINE without write",async()=>{
    await desktop.select("#target","miami");await upload(desktop,fixture.chicago);await waitStatus(desktop,"WRONG ENGINE");
    assert.equal(await count(desktop),1);await desktop.select("#target","");
  });
  await run("Filename-model hint conflict rejected; not a claimed physical identity",async()=>{
    await desktop.select("#model","pro");await upload(desktop,fixture.model);await waitStatus(desktop,"WRONG MODEL");
    assert.equal(await count(desktop),1);await desktop.select("#model","");
  });
  await run("Real ZIP picker: preview/keep, ZIP download, both original records intact",async()=>{
    await upload(desktop,fixture.zip);await waitStatus(desktop,"All 2 record");
    assert.equal(await count(desktop),1);
    await desktop.click("#keep");await waitStatus(desktop,"Added 2 validated");assert.equal(await count(desktop),3);
    await desktop.click("#save-zip");await waitStatus(desktop,"ZIP download requested");
    const bytes=await download(downloads,"DnBOX-user-presets.dnpr.zip");
    const imported=await readDnprZip(new Uint8Array(bytes));
    assert.equal(imported.length,3);
    assert.deepEqual([...imported.map(x=>x.bytes)],[
      new Uint8Array(readFileSync(fixture.chicago)),
      new Uint8Array(readFileSync(fixture.juno)),
      new Uint8Array(readFileSync(fixture.drone))
    ]);
  });
  await run("Factory-derived preset preview but both export methods refuse",async()=>{
    await upload(desktop,fixture.factory);await waitStatus(desktop,"passed file checks");
    await desktop.click("#keep");await waitStatus(desktop,"Added 1 validated");
    const countBefore=readdirSync(downloads).length;
    await desktop.click("#save-file");await waitStatus(desktop,"FACTORY EXPORT OFF");
    await desktop.click("#save-zip");await waitStatus(desktop,"FACTORY EXPORT OFF");
    assert.equal(readdirSync(downloads).length,countBefore);
  });
  await run("909 sample references visibly warn when hardware inventory absent",async()=>{
    await upload(desktop,fixture.kit);await waitStatus(desktop,"passed file checks");
    assert.match(await desktop.$eval("#preview-warnings",e=>e.textContent),/not checked against keyboard/);
    desktopShots.push(await screenshot(desktop,"desktop-909-warning.png"));
    await desktop.click("#discard");await waitStatus(desktop,"Preview discarded");
  });
  await run("Chromium AX tree: status, label and button accessible names",async()=>{
    const session=await desktop.createCDPSession();
    const {nodes}=await session.send("Accessibility.getFullAXTree");
    const names=nodes.map(n=>n.name?.value||"").filter(Boolean).join("|");
    assert.match(names,/Intended hardware/);
    assert.match(names,/Target engine/);
    assert.match(names,/Download selected/);
    assert.match(names,/Choose.*dnpr/);
    assert.match(names,/Clear browser session/);
  });
  const mobile=await makePage({width:390,height:844,isMobile:true,hasTouch:true,deviceScaleFactor:2});
  const mobileShots=[];
  await run("Real mobile touchscreen: 390px layout and usable tap controls",async()=>{
    const state=await mobile.evaluate(()=>({
      vw:innerWidth,sw:document.documentElement.scrollWidth,
      bg:getComputedStyle(document.documentElement).getPropertyValue("--bg").trim(),
      buttons:["save-file","save-zip","clear"].map(id=>{
        const r=document.getElementById(id).getBoundingClientRect();
        return {id,width:r.width,height:r.height};
      })
    }));
    assert.ok(state.sw<=state.vw+1,JSON.stringify(state));
    assert.ok(state.buttons.every(x=>x.height>=44),JSON.stringify(state));
    mobileShots.push(await screenshot(mobile,"mobile-390-empty.png"));
    await upload(mobile,fixture.drum);await waitStatus(mobile,"passed file checks");
    assert.ok(await mobile.$eval("#keep",e=>e.getBoundingClientRect().height>=44),"Visible Keep target is under 44px");
    assert.match(await mobile.$eval("#preview-warnings",e=>e.textContent),/not checked against keyboard/);
    mobileShots.push(await screenshot(mobile,"mobile-390-808-preview.png"));
    await mobile.tap("#keep");await waitStatus(mobile,"Added 1 validated");
    assert.equal(await count(mobile),1);
    mobileShots.push(await screenshot(mobile,"mobile-390-kept.png"));
    await mobile.tap("#save-file");await waitStatus(mobile,"Download requested");
    assert.equal(Buffer.compare(await download(downloads,"Owner-808.808.dnpr"),readFileSync(fixture.drum)),0);
  });
  await run("Small mobile 320px: no clipping, accessible focus and touch targets",async()=>{
    await mobile.setViewport({width:320,height:720,isMobile:true,hasTouch:true,deviceScaleFactor:2});
    await mobile.reload({waitUntil:"networkidle0"});
    const info=await mobile.evaluate(()=>({
      sw:document.documentElement.scrollWidth,vw:innerWidth,
      file:document.querySelector("#files").getBoundingClientRect().width,
      panel:document.querySelector(".panel").getBoundingClientRect().width,
      vwCss:document.documentElement.clientWidth
    }));
    assert.ok(info.sw<=info.vw+1,JSON.stringify(info));
    assert.ok(info.file<=info.panel,JSON.stringify(info));
    mobileShots.push(await screenshot(mobile,"mobile-320-empty.png"));
  });
  await run("Real drag/drop: browser File and DataTransfer call same validation path",async()=>{
    const b=readFileSync(fixture.brk);
    const info=await mobile.evaluate(async a=>{
      const f=new File([Uint8Array.from(a)],"beats.break.dnpr",{type:"application/octet-stream"});
      const dt=new DataTransfer();dt.items.add(f);
      const element=document.getElementById("drop");
      element.dispatchEvent(new DragEvent("dragover",{bubbles:true,cancelable:true,dataTransfer:dt}));
      element.dispatchEvent(new DragEvent("drop",{bubbles:true,cancelable:true,dataTransfer:dt}));
      return true;
    },[...b]);
    assert.equal(info,true);await waitStatus(mobile,"passed file checks");
    assert.equal(await count(mobile),0);
    assert.equal(await mobile.$eval("#preview",e=>e.hidden),false);
  });
  await run("Isolated local server: no external requests or uncaught JS errors",async()=>{
    assert.deepEqual(unexpectedRequests,[]);
    assert.deepEqual(errors,[]);
  });
  const report={
    project:"DnBOX W3",pr:901,fixtureSource:process.env.DNBOX_SOURCE_SHA || "UNPINNED",
    node:process.version,chromium:version,puppeteer:puppeteerVersion,
    localOnly:true,deviceTransfer:false,server:"127.0.0.1 (ephemeral port)",
    cases,passed:cases.filter(x=>x.status==="PASS").length,failed:cases.filter(x=>x.status==="FAIL").length,
    pageErrors:errors,unexpectedRequests,desktopScreenshots:desktopShots,mobileScreenshots:mobileShots,
    evidenceDir
  };
  const evidence=join(evidenceDir,"chromium-w3-901.json");
  writeFileSync(evidence,JSON.stringify(report,null,2)+"\n");
  console.log("BROWSER_REPORT",JSON.stringify(report));
  console.log("BROWSER_EVIDENCE",evidence);
  if(report.failed) process.exitCode=1;
  await desktop.close();await mobile.close();
} catch(e){ console.error("BROWSER_FATAL",e.stack);process.exitCode=1; }
finally{
  if(browser) await browser.close();
  await new Promise(r=>http.close(r));
}