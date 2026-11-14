// SPDX-License-Identifier: GPL-3.0-only
// Focus and per-record warning checks against the real browser page.
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import assert from "node:assert/strict";
import { encodeDnpr } from "./codec.mjs";

const require = createRequire(import.meta.url);
const { chromium } = require(process.env.DNBOX_PLAYWRIGHT_MODULE || "playwright");
const dir = dirname(fileURLToPath(import.meta.url));
const server = createServer((req, res) => {
  const name = new URL(req.url, "http://localhost").pathname.split("/").pop() || "index.html";
  if (!["index.html", "app.mjs", "codec.mjs", "zip.mjs"].includes(name)) { res.writeHead(404); res.end(); return; }
  res.writeHead(200, { "Content-Type": name.endsWith(".mjs") ? "text/javascript" : "text/html" });
  res.end(readFileSync(join(dir, name)));
});
await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
const browser = await chromium.launch({ executablePath: process.env.DNBOX_CHROMIUM || "/usr/bin/chromium", headless:true, args:["--no-sandbox"] });
const failures = [];
try {
  const page = await browser.newPage();
  const bytes = (name, originalEngine) => Buffer.from(encodeDnpr({ engine:"chicago", name, originalEngine, origin:2, params:new Uint8Array(22) }));
  const files = [ {name:"clean.dnpr", mimeType:"application/octet-stream", buffer:bytes("Clean", 1)},
    {name:"warning.dnpr", mimeType:"application/octet-stream", buffer:bytes("Warning", 2)} ];
  const check = async (name, fn) => { try { await fn(); console.log("PASS", name); } catch (error) { failures.push(name + ": " + error.message); console.log("FAIL", failures.at(-1)); } };
  await page.goto(`http://127.0.0.1:${server.address().port}/presets/`);
  await page.locator("#files").setInputFiles(files);
  await page.waitForFunction(() => document.querySelector("#status").textContent.includes("passed file checks"));
  await check("warning belongs to its preview record", async () => {
    const records = page.locator("#preview-meta .preview-record");
    assert.equal(await records.count(), 2);
    assert.equal(await records.nth(0).getByText("Original engine differs", {exact:false}).count(), 0);
    assert.equal(await records.nth(1).getByText("Original engine differs", {exact:false}).count(), 1);
  });
  await page.locator("#keep").focus();
  await page.keyboard.press("Enter");
  await check("keeping preview leaves focus on selected session record", async () => {
    assert.equal(await page.evaluate(() => document.activeElement?.getAttribute("aria-pressed")), "true");
  });
  await page.locator("#files").setInputFiles(files[0]);
  await page.waitForFunction(() => document.querySelector("#status").textContent.includes("passed file checks"));
  await page.locator("#discard").focus();
  await page.keyboard.press("Enter");
  await check("discarding preview returns focus to file picker", async () => {
    assert.equal(await page.evaluate(() => document.activeElement?.id), "files");
  });
  page.once("dialog", dialog => dialog.accept());
  await page.locator("#clear").focus();
  await page.keyboard.press("Enter");
  await check("clearing session returns focus to file picker", async () => {
    assert.equal(await page.evaluate(() => document.activeElement?.id), "files");
  });
} finally { await browser.close(); await new Promise(resolve => server.close(resolve)); }
if (failures.length) process.exitCode = 1;
