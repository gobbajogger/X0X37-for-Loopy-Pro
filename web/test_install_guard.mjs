// SPDX-License-Identifier: GPL-3.0-only
// Real run() code against fake MIDI: refuse duplicate writes, recover after MIDI denial.
// No keyboard, bootloader, firmware bytes or sample slots are accessed.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';

const html = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const anchor = 'async function run(img, product, opts = {}) {';
const start = html.indexOf(anchor), end = html.indexOf('\n}', start);
assert.ok(start >= 0 && end > start, 'production installer run() missing');
const run = html.slice(start, end + 2);

function makeCase(midiAccess, stock = null) {
  const ui = Object.fromEntries(['go', 'stockgo', 'stockconfirm', 'conn', 'bar']
    .map(id => [id, { disabled: false, checked: true, value: 0, textContent: '' }]));
  const state = { writes: [], messages: [], scans: [], gateUpdates: 0 };
  let ctx;
  const fake = {
    $: id => ui[id] || (ui[id] = {}),
    Blob, URL, setTimeout, renderConnectionAction() {}, connected: false,
    document: { createElement: () => ({ click() {}, remove() {} }), body: { appendChild() {} } },
    midiAccess,
    Updater: class { async find() { return null; } },
    installKeepingSlots: async (_up, _img, product) => {
      assert.equal(ctx.isWriting(), true, 'write lock not held during simulated install');
      state.writes.push(product);
      return { move: null };
    },
    scan: async allow => { state.scans.push({ allow, writing: ctx.isWriting() }); },
    sameKeyboard: () => true, t: key => key,
    sayK: key => state.messages.push(key), log() {}, led() {},
    updateInstallGate: () => { state.gateUpdates++; ui.go.disabled = false; },
    stock, TEXT: { en: {} }, lang: 'en',
  };
  ctx = createContext(fake);
  new Script('let writing = false, connectionState = "idle";\n' + run +
    '\nglobalThis.start = run; globalThis.isWriting = () => writing;').runInContext(ctx);
  return { ctx, ui, state };
}

// Different Install buttons were both enabled until an asynchronous MIDI permission request settled.
{
  let permit;
  const pending = new Promise(resolve => { permit = resolve; });
  const { ctx, ui, state } = makeCase(async () => pending);
  ui.go.disabled = true; // primary Install click handler
  const first = ctx.start(new Uint8Array(), 'SMK-37 Pro_900');
  assert.equal(ctx.isWriting(), true, 'MIDI await must not leave install lock open');
  assert.equal(ui.stockgo.disabled, true, 'other Install button still clickable');
  const second = ctx.start(new Uint8Array(), 'SMK-37 Elite_900', { foreign: true });
  permit({});
  await Promise.all([first, second]);
  assert.deepEqual(state.writes, ['SMK-37 Pro_900'], 'two installs overlapped');
  assert.deepEqual(state.scans, [{ allow: true, writing: true }], 'post-write scan lost the write lock');
  assert.equal(ctx.isWriting(), false, 'write lock leaked after reconnect');
  console.log('PASS install guard: one writer while MIDI access is pending');
}

for (const stock of [null, { image: new Uint8Array(), product: 'SMK-37 Pro_900' }]) {
  const { ctx, ui, state } = makeCase(async () => { throw Object.assign(new Error('denied'), { code: 'denied' }); }, stock);
  ui.go.disabled = ui.stockgo.disabled = true; // whichever action was clicked
  await ctx.start(new Uint8Array(), 'SMK-37 Pro_900', { foreign: !!stock });
  assert.equal(ctx.isWriting(), false, 'MIDI refusal left install lock held');
  assert.equal(state.writes.length, 0, 'firmware write attempted despite denial');
  assert.deepEqual(state.messages, ['denied']);
  assert.equal(state.gateUpdates, 1, 'verified-package retry not refreshed');
  assert.equal(ui.go.disabled, false, 'main Install button still disabled');
  assert.equal(ui.stockgo.disabled, !stock, 'local firmware retry state incorrect');
  console.log('PASS install guard: MIDI denial, ' + (stock ? 'local firmware' : 'catalog firmware') + ' retry');
}
console.log('PASS installer single-writer and permission recovery (3 mock scenarios)');
