// SPDX-License-Identifier: GPL-3.0-only
// Run: node --test web/presets/test_page.mjs
// Source-driven DOM interaction tests. Real Chromium and device checks are separate.
import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import * as codec from "./codec.mjs";
import * as zip from "./zip.mjs";
import { checkPresetPage } from "./page-harness.mjs";

test("DnBOX W3 file-only page interactions", async () => {
  const appSource = readFileSync(new URL("./app.mjs", import.meta.url), "utf8");
  const htmlSource = readFileSync(new URL("./index.html", import.meta.url), "utf8");
  const result = await checkPresetPage({ appSource, htmlSource, codec, zip });
  assert.deepEqual(result.failures, [], result.failures.join("\n"));
  assert.equal(result.passed.length, 11, "Keep every interaction regression check active");
});
