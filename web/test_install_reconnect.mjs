// SPDX-License-Identifier: GPL-3.0-only
// Execute the installer's real scan/run functions against fake Web MIDI.
// No physical device, firmware image or sample data is touched.
import './test_install_guard.mjs';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';

const input = process.argv[2] || new URL('./x0x_installer.html', import.meta.url);
const source = readFileSync(input, 'utf8');
function extract(anchor) {
  const start = source.indexOf(anchor);
  assert.ok(start >= 0, 'missing production function ' + anchor);
  const end = source.indexOf('\n}', start);
  assert.ok(end > start, 'unterminated production function ' + anchor);
  return source.slice(start, end + 2);
}
const scan = extract('async function scan(allowPostWrite = false) {');
const run = extract('async function run(img, product, opts = {}) {');
assert.match(scan, /if \(writing && !allowPostWrite\) return;/,
  'ordinary MIDI changes must not scan while a write is active');

for (const mode of ['normal', 'resume', 'missing', 'failure', 'partial', 'stock']) {
  const state = { connected: false, scans: 0, leds: [], finished: false, messages: [], reloads: 0 };
  const ui = Object.fromEntries(['go', 'stockgo', 'stockconfirm', 'devagree', 'conn', 'bar']
    .map(id => [id, { disabled: false, checked: false, value: 0, textContent: '' }]));
  ui.devagree.checked = true;
  const link = { close() {} };
  const fake = {
    $: id => ui[id] || (ui[id] = {}),
    Blob, URL, setTimeout, renderConnectionAction() {}, refreshJourney() {}, connected: false,
    document: { querySelector: () => null, createElement: () => ({ click() {}, remove() {} }),
      body: { appendChild() {} } },
    midiAccess: async () => ({}),
    Updater: class {
      async find(predicate) {
        const ota = { model: 'ota-SMK-37 Pro', text: 'ota-SMK-37 Pro' };
        const app = { model: 'SMK-37 Pro', text: 'SMK-37 Pro' };
        if (state.finished) {
          assert.equal(ctx.isWriting(), !state.allowManual, mode + ': reconnect must retain the write lock');
          state.scans++;
          if (mode === 'missing') return null;
          return predicate(app) ? { id: app, name: 'Keyboard', link } : null;
        }
        if (mode === 'resume' && predicate(ota)) return { id: ota, link };
        return null;
      }
      async resume() { state.finished = true; return true; }
    },
    installKeepingSlots: async (_up, _img, _product, _board, opts) => {
      if (mode === 'failure') throw Object.assign(new Error('test disconnect'), { code: 'lost' });
      state.finished = true;
      opts.step("done");
      if (mode === 'partial') return {move: {newMap: [1], skipped: ['slot 3 too large'], name: 'saved-partial.x0xslots'}};
      if (mode === 'stock') return {move: {newMap: [], skipped: [], name: 'saved-stock.x0xslots'}};
      return { move: null };
    },
    t: key => key, sayK: (key, detail) => { state.messages.push({key, detail}); }, log() {},
    sameKeyboard: () => true,
    led: value => state.leds.push(value),
    setConnected: on => {
      state.connected = on;
      state.leds.push(on ? 'on' : 'off');
    },
    currentVersion: () => ({models: {pro: {}}}), availableModels: () => ['pro'],
    loadSelectedPackage: () => { state.reloads++; state.messages.push({key: 'package-reloaded'}); },
    updateInstallGate() { state.gateUpdated = true; ui.go.disabled = !ui.devagree.checked; },
    stock: null, TEXT: { en: { lost: 'Disconnected' } }, lang: 'en',
  };
  const ctx = createContext(fake);
  new Script('let writing = false, lastId = null, scanSerial = 0, connectionState = "idle", chosenModel = "pro";\n' + extract('function suggestModel(deviceModel) {') + '\n' + scan + '\n' + run +
    '\nglobalThis.start = run; globalThis.scan = scan; globalThis.isWriting = () => writing;')
    .runInContext(ctx);
  await ctx.start(new Uint8Array(0), 'SMK-37 Pro_900', {foreign: mode === 'stock'});
  assert.equal(ctx.isWriting(), false, mode + ': write lock not released');
  assert.equal(state.gateUpdated, true, mode + ': install gate not updated');
  if (mode === 'failure') {
    assert.equal(state.scans, 0, 'failed install must preserve its error');
    assert.equal(state.leds.at(-1), 'err', 'error LED must remain visible');
  } else {
    assert.equal(state.finished, true, mode + ': install did not complete');
    assert.ok(state.scans >= 1, mode + ': no post-write scan');
    assert.equal(state.connected, mode !== 'missing', mode + ': stale connection state');
    assert.equal(state.leds.at(-1), mode === 'missing' ? 'off' : 'on',
      mode + ': stale connection indicator');
  }
  if (mode !== 'failure' && mode !== 'missing') {
    assert.equal(state.reloads, 0, mode + ': owned reconnect reloaded package over install result');
    assert.equal(ui.devagree.checked, false, mode + ': next write needs renewed consent');
    assert.equal(ui.go.disabled, true, mode + ': next write must stay disabled without consent');
    assert.equal(ui.bar.value, 100, mode + ': completed progress was lost');
  }
  if (mode === 'partial') assert.deepEqual(state.messages.at(-1), {
    key: 'slotsdone', detail: 'slotskipslot 3 too large (saved-partial.x0xslots)'
  }, 'partial-restore warning and archive name must survive reconnect');
  if (mode === 'stock') assert.deepEqual(state.messages.at(-1), {
    key: 'slotstock', detail: 'saved-stock.x0xslots'
  }, 'stock archive instructions must survive reconnect');
  if (mode === 'partial' || mode === 'stock') {
    state.finished = false; // Manual scan has no write ownership and retains ordinary model selection.
    await ctx.scan();
    assert.equal(state.reloads, 0); // no device found in this fixture branch
    state.finished = true;
    // A standalone manual scan uses the same returned app, without a held writer.
    state.allowManual = true;
    await ctx.scan();
    assert.equal(state.reloads, 1, 'ordinary scan must still select/reload model');
  }
  console.log('PASS reconnect ' + mode);
}
console.log('PASS post-write installer connection checks (simulated MIDI, 6 cases plus ordinary manual scans)');
