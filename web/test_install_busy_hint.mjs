// SPDX-License-Identifier: GPL-3.0-only
// Exercise actual gate/run functions with a pending install, without hardware.
import { readFileSync } from 'node:fs';
import { runInNewContext } from 'node:vm';
import assert from 'node:assert/strict';
const html = readFileSync(process.env.X0X_INSTALLER_HTML || new URL('x0x_installer.html', import.meta.url), 'utf8');
const gate = html.slice(html.indexOf('function isCatalogChecked('), html.indexOf('function buildBadge('));
const run = html.slice(html.indexOf('async function run('), html.indexOf('$("go").addEventListener("click"'));
assert.ok(gate.includes('function updateInstallGate()') && run.includes('writing = true;'));
const strings = Object.fromEntries(['hintbusy', 'hintready', 'hintack', 'hintblocked', 'hintverifying']
  .map(key => [key, [...html.matchAll(new RegExp(key + ': "([^"\\n]*)"', 'g'))].at(-1)[1]]));
let release;
const pending = new Promise(resolve => { release = resolve; });
const ui = new Proxy({}, { get(obj, id) { return obj[id] ||= { checked: true, disabled: false, textContent: '' }; } });
const api = runInNewContext(`
  let writing = false, image = new Uint8Array([1]), stock = null;
  // Eligibility is exercised separately by the retained public-eligibility suite.
  const approvedModels = () => ['pro'], renderConnectionAction = () => {}, refreshJourney = () => {};
  let connectionState = 'idle', connected = false;
  const chosenModel = 'pro', version = {status:'dev-checked',models:{pro:{sha256:'a'.repeat(64)}}};
  const currentVersion = () => version, $ = id => ui[id], t = key => strings[key];
  const midiAccess = async () => ({}), led = () => {}, log = () => {}, sayK = () => {}, sameKeyboard = () => true;
  const Updater = class {async find() {return null;}};
  const installKeepingSlots = () => pending, scan = async () => {};
  const lang = 'en', TEXT = {en:{}};
  ${gate}\n${run}
  ({run, updateInstallGate, writing:()=>writing});`, {ui, strings, pending});
api.updateInstallGate();
assert.match(ui.installhint.textContent, /Ready/);
let completed = false;
const active = api.run(new Uint8Array([1]), 'SMK-37 Pro_900').then(() => {completed = true;});
for (let i = 0; i < 8; i++) await Promise.resolve();
assert.equal(completed, false, 'install must remain pending during the hint assertion');
assert.equal(api.writing(), true);
assert.match(ui.installhint.textContent, /Keep USB connected/);
assert.equal(ui.go.disabled, true);
assert.equal(ui.stockgo.disabled, true);
ui.devagree.checked = false;
api.updateInstallGate();
assert.match(ui.installhint.textContent, /Keep USB connected/);
assert.equal(ui.go.disabled, true, 'changing consent cannot enable install during a write');
ui.devagree.checked = true;
release({});
await active;
assert.equal(api.writing(), false);
assert.match(ui.installhint.textContent, /Ready/);
assert.equal(ui.go.disabled, false);
ui.devagree.checked = false;
api.updateInstallGate();
assert.equal(ui.go.disabled, true, 'consent remains mandatory after completion');
console.log('PASS busy USB hint during pending write, ready after completion, consent and writer gates preserved');
