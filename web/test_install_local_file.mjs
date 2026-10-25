// SPDX-License-Identifier: GPL-3.0-only
// Host-only BF-001 regression: local .fwsc / ctl.img selection generations.
// No Web MIDI, no device write. Newest selection wins; stale completions no-op.
import { readFileSync } from 'node:fs';
import { runInNewContext } from 'node:vm';
import assert from 'node:assert/strict';

const html = readFileSync(process.env.X0X_INSTALLER_HTML || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const start = html.indexOf('let stockLoadSerial = 0;');
const stop = html.indexOf('async function run(img, product, opts = {})', start);
assert.ok(start > 0 && stop > start, 'production local stock selection block must exist');
const ctlStart = html.indexOf('let ctlLoadSerial = 0;');
const ctlEnd = html.indexOf('window.addEventListener("beforeunload"', ctlStart);
assert.ok(ctlStart > 0 && ctlEnd > ctlStart, 'controller-image file selection block must exist');
const source = html.slice(ctlStart, ctlEnd) + '\n' + html.slice(start, stop);

function picker({ digest } = {}) {
  const nodes = {};
  const node = () => ({
    checked: false, disabled: false, value: 'previous', textContent: '', handlers: {},
    addEventListener(event, fn) { this.handlers[event] = fn; },
    classList: { add() {}, remove() {} },
  });
  const dom = new Proxy(nodes, { get(t, p) { return (t[p] ||= node()); } });
  const events = [];
  const api = runInNewContext(`(() => {
    let stock = null, ctl = null, writing = false, lastId = null;
    const $ = id => dom[id];
    const sayK = (code, suffix) => events.push({ code, suffix });
    const STOCK_SHA256_FM1 = 'stock-reference';
    const productOf = raw => raw[0] === 1 ? 'SMK-37 Pro_9001000'
      : raw[0] === 2 ? 'SMK-37 Elite_9001000'
      : raw[0] === 3 ? 'FM-1_9001000' : 'Unrelated_9001000';
    const sha256Hex = raw => digest(raw);
    const sameKeyboard = (from, to) => (/^FM-1/i.test(from) === /^FM-1/i.test(to));
    const logicalImage = raw => raw;
    const ctlProblem = raw => raw[0] === 8 || raw[0] === 9 ? null : "not-controller-image";
    ${source}
    return {
      stock: () => stock, ctl: () => ctl, state: () => ({ writing, lastId }),
      connect: model => { lastId = { model, text: model }; },
      writing: value => { writing = value; },
    };
  })()`, { dom, events, digest: digest || (async () => 'not-stock') });
  const input = file => {
    dom.stockfile.value = 'selected.fwsc';
    return dom.stockfile.handlers.change({
      target: {
        files: [file],
        get value() { return dom.stockfile.value; },
        set value(v) { dom.stockfile.value = v; },
      },
    });
  };
  const ctlInput = file => {
    dom.ctlfile.value = 'selected.img';
    return dom.ctlfile.handlers.change({
      target: {
        files: [file],
        get value() { return dom.ctlfile.value; },
        set value(v) { dom.ctlfile.value = v; },
      },
    });
  };
  const drop = file => dom.stockdrop.handlers.drop({
    preventDefault() {},
    dataTransfer: { files: [file] },
  });
  return { dom, api, events, input, ctlInput, drop };
}

const file = id => ({ arrayBuffer: async () => Uint8Array.of(id).buffer });
function deferred() {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return { promise, resolve, reject };
}
async function caseRun(name, fn) {
  await fn();
  console.log('PASS', name);
}

await caseRun('last selected file wins when an earlier read resolves late', async () => {
  const p = picker();
  const slow = deferred();
  const a = p.input({ arrayBuffer: () => slow.promise });
  const b = p.input(file(2));
  await b;
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
  slow.resolve(Uint8Array.of(1).buffer);
  await a;
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
});

await caseRun('drag/drop wins over an earlier file chooser', async () => {
  const p = picker();
  const slow = deferred();
  const a = p.input({ arrayBuffer: () => slow.promise });
  await p.drop(file(2));
  slow.resolve(Uint8Array.of(1).buffer);
  await a;
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
});

await caseRun('late SHA calculation cannot replace latest selection', async () => {
  const slow = deferred();
  const p = picker({
    digest: raw => new Uint8Array(raw)[0] === 1 ? slow.promise : Promise.resolve('not-stock'),
  });
  const a = p.input(file(1));
  await Promise.resolve();
  const b = p.input(file(2));
  await b;
  slow.resolve('not-stock');
  await a;
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
});

await caseRun('new selection immediately clears consent and disables Install', async () => {
  const p = picker();
  await p.input(file(1));
  p.dom.stockconfirm.checked = true;
  p.dom.stockconfirm.handlers.change();
  assert.equal(p.dom.stockgo.disabled, false);
  const pending = deferred();
  const first = p.input({ arrayBuffer: () => pending.promise });
  assert.equal(p.api.stock(), null);
  assert.equal(p.dom.stockconfirm.checked, false);
  assert.equal(p.dom.stockgo.disabled, true);
  pending.resolve(Uint8Array.of(2).buffer);
  await first;
  assert.equal(p.dom.stockgo.disabled, true, 'new build must require fresh consent');
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
  assert.equal(p.dom.stockfile.value, '', 'allow choosing same file again');
});

await caseRun('stale read rejection does not affect later valid selection', async () => {
  const p = picker();
  const slow = deferred();
  const a = p.input({ arrayBuffer: () => slow.promise });
  await p.input(file(2));
  slow.reject(new Error('delayed failed read'));
  await a;
  assert.equal(p.api.stock()?.product, 'SMK-37 Elite_9001000');
  assert.equal(p.events.at(-1)?.code, 'stockok');
});

await caseRun('active read rejection is handled and reported', async () => {
  const p = picker();
  await p.input({ arrayBuffer: () => Promise.reject(new Error('cannot read')) });
  assert.equal(p.api.stock(), null);
  assert.equal(p.dom.stockgo.disabled, true);
  assert.equal(p.events.at(-1)?.code, 'stockloadfail');
});

await caseRun('invalid and wrong-family packages cannot be installed', async () => {
  const p = picker();
  await p.input(file(1));
  p.dom.stockconfirm.checked = true;
  p.dom.stockconfirm.handlers.change();
  await p.input(file(0));
  assert.equal(p.api.stock(), null);
  assert.equal(p.dom.stockgo.disabled, true);
  p.api.connect('FM-1');
  await p.input(file(1));
  assert.equal(p.api.stock(), null);
  assert.equal(p.dom.stockgo.disabled, true);
  assert.equal(p.events.at(-1)?.code, 'model');
});

await caseRun('last controller image wins with overlapping reads', async () => {
  const p = picker();
  const slow = deferred();
  const a = p.ctlInput({ arrayBuffer: () => slow.promise });
  await p.ctlInput(file(9));
  assert.equal(p.api.ctl()?.[0], 9);
  slow.resolve(Uint8Array.of(8).buffer);
  await a;
  assert.equal(p.api.ctl()?.[0], 9);
  assert.equal(p.dom.ctlfile.value, '', 'allow reselecting same controller image');
});

await caseRun('new controller choice invalidates earlier controller', async () => {
  const p = picker();
  await p.ctlInput(file(8));
  await p.ctlInput(file(0));
  assert.equal(p.api.ctl(), null);
  assert.equal(p.events.at(-1)?.code, 'ctlbad');
});

await caseRun('failed controller read is handled, not an unhandled rejection', async () => {
  const p = picker();
  await p.ctlInput({ arrayBuffer: () => Promise.reject(new Error('cannot read')) });
  assert.equal(p.api.ctl(), null);
  assert.equal(p.events.at(-1)?.code, 'ctlreadfail');
});

assert.ok(html.includes('ctlreading:'), 'must provide controller loading message');
assert.ok(html.includes('ctlreadfail:'), 'must provide controller read failure message');
assert.ok(html.includes('stockloading:'), 'must provide readable loading message in both language dictionaries');
assert.ok(html.includes('stockloadfail:'), 'must provide readable read failure message');
console.log('local .fwsc selection regressions passed');
