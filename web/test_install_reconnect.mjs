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

for (const mode of ['normal', 'resume', 'missing', 'failure']) {
  const state = { connected: false, scans: 0, leds: [], finished: false };
  const ui = Object.fromEntries(['go', 'stockgo', 'stockconfirm', 'conn', 'bar']
    .map(id => [id, { disabled: false, checked: false, value: 0, textContent: '' }]));
  const link = { close() {} };
  const fake = {
    $: id => ui[id] || (ui[id] = {}),
    Blob, URL, setTimeout,
    document: { createElement: () => ({ click() {}, remove() {} }),
      body: { appendChild() {} } },
    midiAccess: async () => ({}),
    Updater: class {
      async find(predicate) {
        const ota = { model: 'ota-SMK-37 Pro', text: 'ota-SMK-37 Pro' };
        const app = { model: 'SMK-37 Pro', text: 'SMK-37 Pro' };
        if (state.finished) {
          assert.equal(ctx.isWriting(), true, mode + ': reconnect must retain the write lock');
          state.scans++;
          if (mode === 'missing') return null;
          return predicate(app) ? { id: app, name: 'Keyboard', link } : null;
        }
        if (mode === 'resume' && predicate(ota)) return { id: ota, link };
        return null;
      }
      async resume() { state.finished = true; return true; }
    },
    installKeepingSlots: async () => {
      if (mode === 'failure') throw Object.assign(new Error('test disconnect'), { code: 'lost' });
      state.finished = true;
      return { move: null };
    },
    t: key => key, sayK() {}, log() {},
    sameKeyboard: () => true,
    led: value => state.leds.push(value),
    setConnected: on => {
      state.connected = on;
      state.leds.push(on ? 'on' : 'off');
    },
    suggestModel() {}, updateInstallGate() { state.gateUpdated = true; },
    stock: null, TEXT: { en: { lost: 'Disconnected' } }, lang: 'en',
  };
  const ctx = createContext(fake);
  new Script('let writing = false, lastId = null, scanSerial = 0;\n' + scan + '\n' + run +
    '\nglobalThis.start = run; globalThis.isWriting = () => writing;')
    .runInContext(ctx);
  await ctx.start(new Uint8Array(0), 'SMK-37 Pro_900');
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
  console.log('PASS reconnect ' + mode);
}
console.log('PASS post-write installer connection checks (simulated MIDI, 4 cases)');
