// SPDX-License-Identifier: GPL-3.0-only
// Empty-catalog workflow regression. Simulated files are UI fixtures, not firmware packages.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';

const source = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const cut = (a, b) => {
  const start = source.indexOf(a), end = source.indexOf(b, start);
  assert(start >= 0 && end > start, `missing source block ${a}`);
  return source.slice(start, end);
};
assert.match(source, /id="localempty" hidden/);
assert.match(source, /<button[^>]+id="chooseLocal"[^>]+type="button"/);
assert.match(source, /id="stockfile" accept="\\.fwsc"/);
assert.match(cut('function fillVersions()', '\\nfunction confirmWrite('), /\\$\\("localempty"\\)\\.hidden = list\\.length > 0/);
assert.equal((source.match(/localchoose: "/g) || []).length, 2, 'English and Japanese translations');
assert.match(source, /\\.local-pick \\{[^}]+min-height: 51px/);
const sourceHandlers = cut('let localPickerFromEmptyState = false;', '$("stockconfirm").addEventListener("change"');
const events = {}, observations = [];
const elements = Object.fromEntries(['chooseLocal', 'stockfile', 'stock', 'stockconfirm', 'stockgo'].map(id => [id, {
  checked: false, disabled: true,
  addEventListener(name, fn) { events[`${id}:${name}`] = fn; },
  click() { observations.push(`${id}.click`); },
  scrollIntoView() { observations.push(`${id}.scroll`); },
  focus() { observations.push(`${id}.focus`); },
}]));
const context = createContext({ elements, events, observations });
new Script(`
const $ = id => elements[id];
let stock = null, writing = false;
async function selectStockFile(file) { stock = file.valid && !writing ? {product:'SMK-37 Pro_900'} : null; }
${sourceHandlers}
globalThis.checkStock=()=>stock;
globalThis.setWriting=v=>{ writing=v; };
`).runInContext(context);
assert.equal(Object.keys(events).sort().join(','), 'chooseLocal:click,stockfile:cancel,stockfile:change');
assert.equal(elements.stockgo.disabled, true, 'Install must initially remain disabled');

// New primary CTA reuses the existing local-file input; no write and no auto-confirm.
events['chooseLocal:click']();
assert.deepEqual(observations, ['stockfile.click']);
await events['stockfile:change']({target:{files:[{valid:true}],value:'local.fwsc'}});
assert.deepEqual(observations, ['stockfile.click','stock.scroll','stockconfirm.focus']);
assert.equal(elements.stockconfirm.checked, false, 'never pre-approve device identity');
assert.equal(elements.stockgo.disabled, true, 'never enable installation via entry CTA');
console.log('PASS primary local-file chooser scrolls to unchecked confirmation');

// Cancel, invalid file and direct stock chooser must not unexpectedly scroll or focus.
observations.length = 0;
events['chooseLocal:click']();
events['stockfile:cancel']();
await events['stockfile:change']({target:{files:[{valid:true}],value:'local.fwsc'}});
assert.deepEqual(observations, ['stockfile.click']);
console.log('PASS cancellation clears primary-flow navigation');
observations.length = 0;
events['chooseLocal:click']();
await events['stockfile:change']({target:{files:[{valid:false}],value:'broken.fwsc'}});
assert.deepEqual(observations, ['stockfile.click']);
assert.equal(context.checkStock(), null);
console.log('PASS invalid package does not advance to confirmation');
observations.length = 0;
context.setWriting(true);
events['chooseLocal:click']();
await events['stockfile:change']({target:{files:[{valid:true}],value:'valid.fwsc'}});
assert.deepEqual(observations, ['stockfile.click']);
assert.equal(context.checkStock(), null);
console.log('PASS writing lock cannot be bypassed by file CTA');
