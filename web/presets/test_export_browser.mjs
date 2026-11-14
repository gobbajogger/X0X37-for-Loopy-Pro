// SPDX-License-Identifier: GPL-3.0-only
// Browser export regression: DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright node --test web/presets/test_export_browser.mjs
import test, { before, after } from "node:test";
import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { encodeDnpr } from "./codec.mjs";
import { readDnprZip } from "./zip.mjs";

const { chromium } = createRequire(import.meta.url)(process.env.DNBOX_PLAYWRIGHT_MODULE || "playwright");
const here = dirname(fileURLToPath(import.meta.url));
const server = createServer((req, res) => {
  const path = new URL(req.url, "http://localhost").pathname;
  const name = path === "/presets/" ? "index.html" : path.slice("/presets/".length);
  if (!path.startsWith("/presets/") || !["index.html", "app.mjs", "codec.mjs", "zip.mjs"].includes(name)) {
    res.writeHead(404).end(); return;
  }
  res.writeHead(200, {"Content-Type": name.endsWith(".mjs") ? "text/javascript" : "text/html"});
  res.end(readFileSync(join(here, name)));
});
let browser, base;
before(async () => {
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  base = `http://127.0.0.1:${server.address().port}/presets/`;
  browser = await chromium.launch({executablePath:process.env.DNBOX_CHROMIUM || "/usr/bin/chromium", headless:true, args:["--no-sandbox"]});
});
after(async () => { if (browser) await browser.close(); await new Promise(resolve => server.close(resolve)); });
const preset = (name, origin = 2, factoryIndex = 0) => encodeDnpr({
  engine:"chicago", name, origin, factoryIndex, params:Uint8Array.from({length:22}, (_, i) => i)
});
async function stage(page, ...records) {
  await page.goto(base);
  await page.locator("#files").setInputFiles(records.map(({name,bytes}) => ({name:name+".dnpr", mimeType:"application/octet-stream", buffer:Buffer.from(bytes)})));
  await page.waitForFunction(() => document.querySelector("#status").textContent.includes("passed file checks"));
  await page.locator("#keep").click();
}

test("mixed ZIP exports only eligible original bytes and keeps every record", async () => {
  const page = await browser.newPage();
  try {
    const user = preset("User"), factory = preset("Factory",1,1), derived = preset("Derived",2,3);
    await stage(page, {name:"user",bytes:user}, {name:"factory",bytes:factory}, {name:"derived",bytes:derived});
    const downloadPromise = page.waitForEvent("download", {timeout:2000});
    await page.locator("#save-zip").click();
    const download = await downloadPromise;
    const files = await readDnprZip(new Uint8Array(readFileSync(await download.path())));
    assert.equal(files.length,1);
    assert.deepEqual(files[0].bytes,user);
    assert.equal(await page.locator("#session-list > button").count(),3);
    assert.match(await page.locator("#status").innerText(), /1.*(?:unchanged|eligible).*(?:2|two).*(?:excluded|skipped|blocked)/i);
  } finally { await page.close(); }
});

test("all-blocked ZIP reports no download and retains all records", async () => {
  const page = await browser.newPage(); let downloads = 0;
  page.on("download", () => downloads++);
  try {
    await stage(page, {name:"factory",bytes:preset("Factory",1,1)}, {name:"derived",bytes:preset("Derived",2,3)});
    await page.locator("#save-zip").click();
    assert.equal(downloads,0);
    assert.equal(await page.locator("#session-list > button").count(),2);
    assert.match(await page.locator("#status").innerText(), /(?:no|none).*(?:eligible|exportable)/i);
  } finally { await page.close(); }
});

test("download initiation failure releases URL and preserves session", async () => {
  const page = await browser.newPage();
  try {
    await stage(page,{name:"user",bytes:preset("User")});
    await page.evaluate(() => {
      window.revoked = [];
      const revoke = URL.revokeObjectURL.bind(URL);
      URL.revokeObjectURL = url => { window.revoked.push(url); revoke(url); };
      HTMLAnchorElement.prototype.click = function () { throw Error("download blocked"); };
    });
    await page.locator("#save-file").click();
    assert.equal(await page.locator("#session-list > button").count(),1);
    assert.match(await page.locator("#status").innerText(), /download blocked/i);
    assert.equal(await page.evaluate(() => window.revoked.length),1);
  } finally { await page.close(); }
});

test("anchor creation failure releases URL and preserves the session", async () => {
  const page = await browser.newPage();
  try {
    await stage(page,{name:"user",bytes:preset("User")});
    await page.evaluate(() => {
      window.revoked = [];
      const revoke = URL.revokeObjectURL.bind(URL);
      URL.revokeObjectURL = url => { window.revoked.push(url); revoke(url); };
      const create = document.createElement.bind(document);
      document.createElement = tag => { if (tag === "a") throw Error("anchor unavailable"); return create(tag); };
    });
    await page.locator("#save-file").click();
    assert.equal(await page.locator("#session-list > button").count(),1);
    assert.match(await page.locator("#status").innerText(), /EXPORT FAILED.*anchor unavailable/i);
    assert.equal(await page.evaluate(() => window.revoked.length),1);
  } finally { await page.close(); }
});

test("ZIP construction failure is labelled as export failure and retains the session", async () => {
  const page = await browser.newPage(); let downloads = 0;
  page.on("download", () => downloads++);
  try {
    await page.route("**/zip.mjs", async route => {
      const source = readFileSync(join(here,"zip.mjs"),"utf8").replace("export function makeDnprZip(","function originalMakeDnprZip(");
      await route.fulfill({status:200, contentType:"text/javascript", body:source + '\nexport function makeDnprZip() { throw new Error("ZIP construction failed"); }\n'});
    });
    await stage(page,{name:"user",bytes:preset("User")});
    await page.locator("#save-zip").click();
    assert.equal(downloads,0);
    assert.equal(await page.locator("#session-list > button").count(),1);
    assert.match(await page.locator("#status").innerText(), /EXPORT FAILED.*ZIP construction failed/i);
  } finally { await page.close(); }
});
