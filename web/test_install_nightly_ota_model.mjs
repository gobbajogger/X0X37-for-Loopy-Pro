// SPDX-License-Identifier: GPL-3.0-only
// Exact Updater.install OTA target-family regression with mock MIDI discovery.
// Run: node web/test_install_nightly_ota_model.mjs [path-to-fm1ota.js]
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script } from "node:vm";
import { sameKeyboard } from "./fm1ota.js";

const path = process.argv[2] || new URL("./fm1ota.js", import.meta.url);
const text = readFileSync(path, "utf8");
const i = text.indexOf("  async install(image, product, onStep, { beforeUpgrade } = {}) {");
const j = text.indexOf("\n  // step 2 with the loader", i);
assert.ok(i >= 0 && j > i, "Updater.install source anchors absent");
const method = text.slice(i, j).trim();
const install = new Script("({" + method + "}).install").runInNewContext({
  UPGRADE: [0xf0, 0x22, 0x24, 0x35, 0x7f, 0xf7],
  sleep: async () => {},
  IS_OTA: (id) => /^ota-/i.test(id.model),
  sameKeyboard,
  fail: (code, message, detail = "") => Object.assign(new Error(message), { code, detail }),
});
assert.equal(typeof install, "function");

async function simulate(spec) {
  const model = spec.model || "SMK-37 Pro";
  const product = model + "_900";
  const state = { writes: [], filters: [], rebootFilters: [], closed: 0, upgradeCalls: 0 };
  const dev = (m) => ({
    id: { model: m, text: m + "_900" },
    name: m,
    link: { send() { state.upgradeCalls++; return true; }, close() { state.closed++; } },
  });
  let loaderSearch = true;
  const updater = {
    async find(filter) {
      const d = dev(spec.running || model);
      return filter(d.id) ? d : null;
    },
    // Nightly transition wait snapshots prior matching ports; mock has none.
    async matchingPorts() { return []; },
    async serve() { return { finished: true, served: 1 }; },
    async waitFor(filter) {
      if (loaderSearch) {
        loaderSearch = false;
        for (const name of spec.loaders) {
          const d = dev(name);
          const eligible = filter(d.id);
          state.filters.push([name, eligible]);
          if (eligible || spec.ignoreFilter) return d;
        }
        return null;
      }
      // Reboot discovery must skip another plugged-in keyboard's running app.
      for (const name of (spec.reboots || [model])) {
        const d = dev(name);
        const eligible = filter(d.id);
        state.rebootFilters.push([name, eligible]);
        if (eligible || spec.ignoreRebootFilter) return d;
      }
      return null;
    },
    async write(d) {
      state.writes.push(d.id.model);
      return { finished: true };
    },
  };
  try { state.returned = await install.call(updater, new Uint8Array([1]), product, () => {}); }
  catch (e) { state.error = e.code || e.message; }
  return state;
}

const cases = [
  { name: "Pro ignores unrelated FM-1 after loader write", loaders: ["ota-SMK-37 Pro"], reboots: ["FM-1", "SMK-37 Pro"], rebootAllowed: [false, true], write: "ota-SMK-37 Pro" },
  { name: "Pro wrong-family-only reboot is not success", loaders: ["ota-SMK-37 Pro"], reboots: ["FM-1"], rebootAllowed: [false], wrote: "ota-SMK-37 Pro", error: "noreturn" },
  { name: "FM-1 ignores unrelated Pro after loader write", model: "FM-1", loaders: ["ota-FM-1"], reboots: ["SMK-37 Pro", "FM-1"], rebootAllowed: [false, true], write: "ota-FM-1" },
  { name: "wrong-family reboot discovery bypass still rejected", loaders: ["ota-SMK-37 Pro"], reboots: ["FM-1"], ignoreRebootFilter: true, rebootAllowed: [false], wrote: "ota-SMK-37 Pro", error: "noreturn" },
  { name: "legitimate same-family Elite reboot keeps mismatch report", loaders: ["ota-SMK-37 Pro"], reboots: ["SMK-37 Elite"], rebootAllowed: [true], wrote: "ota-SMK-37 Pro", error: "mismatch" },
  { name: "Pro skips unrelated FM-1 loader", loaders: ["ota-FM-1", "ota-SMK-37 Pro"], write: "ota-SMK-37 Pro" },
  { name: "Pro never writes FM-1-only loader", loaders: ["ota-FM-1"], error: "noloader" },
  { name: "FM-1 never writes Pro-only loader", model: "FM-1", loaders: ["ota-SMK-37 Pro"], error: "noloader" },
  { name: "FM-1 skips unrelated Pro loader", model: "FM-1", loaders: ["ota-SMK-37 Pro", "ota-FM-1"], write: "ota-FM-1" },
  { name: "Elite loader alias accepted for Pro", loaders: ["ota-SMK-37 Elite"], write: "ota-SMK-37 Elite" },
  { name: "MKE loader alias accepted for Pro", loaders: ["ota-MKE-P37"], write: "ota-MKE-P37" },
  { name: "Starrykey loader alias accepted for Pro", loaders: ["ota-STARRYKEY-37 Play"], write: "ota-STARRYKEY-37 Play" },
  { name: "independent check rejects wrong loader from discovery", loaders: ["ota-FM-1"], ignoreFilter: true, error: "model" },
  { name: "independent check rejects non-OTA port from discovery", loaders: ["SMK-37 Pro"], ignoreFilter: true, error: "model" },
  { name: "wrong running keyboard rejected", running: "FM-1", loaders: ["ota-SMK-37 Pro"], error: "model" },
  { name: "FM-1 normal OTA supported", model: "FM-1", loaders: ["ota-FM-1"], write: "ota-FM-1" },
  { name: "ordinary port skipped during OTA discovery", loaders: ["SMK-37 Pro", "ota-SMK-37 Pro"], write: "ota-SMK-37 Pro" },
  { name: "case-insensitive SMK alias supported", loaders: ["OTA-smk-37 elite"], write: "OTA-smk-37 elite" },
];
let passed = 0;
for (const c of cases) {
  const r = await simulate(c);
  if (c.write) {
    assert.deepEqual(r.writes, [c.write], c.name + " write target");
    assert.equal(r.error, undefined, c.name + " unexpected error");
    assert.equal(r.returned, (c.model || "SMK-37 Pro") + "_900", c.name + " final identity");
  } else {
    assert.equal(r.error, c.error, c.name + " expected error");
    assert.deepEqual(r.writes, c.wrote ? [c.wrote] : [], c.name + " expected write disposition");
  }
  if (c.rebootAllowed) assert.deepEqual(r.rebootFilters.map((x) => x[1]), c.rebootAllowed, c.name + " reboot family filter");
  passed++;
  console.log("PASS " + c.name);
}
console.log("PASS " + passed + " OTA-loader source-exact mock regressions; no device used");
